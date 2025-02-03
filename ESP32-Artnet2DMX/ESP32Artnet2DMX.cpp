#include "Print.h"

#include "ESP32Artnet2DMX.h"

ESP32Artnet2DMX::ESP32Artnet2DMX() {
  memset( m_dmx_buffer, 0, sizeof( m_dmx_buffer ) );

  m_artnet_source_ipaddress_any.fromString( "255.255.255.255" );
  
  this->StrobeOff();

  m_strobe_all_channels = false;
  m_is_started          = false;
}

ESP32Artnet2DMX::~ESP32Artnet2DMX() {
  this->Stop();
}

void ESP32Artnet2DMX::Init() {

  // Init must be called because class constructor is not called by default on global var.
  m_ConfigServer.Init();

  // Attempt to connect to WiFi.  On failure will create a hotspot.
  m_ConfigServer.ConnectToWiFi();

  // Startup the webserver.
  m_ConfigServer.StartWebServer();

  // Create strobe buffer.
  this->BuildStrobeBuffer();

  m_is_started = false;
}

bool ESP32Artnet2DMX::Start() {

  this->StartDMX();

  if( !m_WiFiUDP.begin( ARTNET_UDP_PORT ) ) {
    Serial.print("Failed to create Art-Net network socket on UDP port 6464\n");
    return false;
  }

  // Store expected source IP for artnet packets.
  m_artnet_source_ipaddress.fromString( m_ConfigServer.m_artnet_source_ip );

  if( m_ConfigServer.m_artnet_timeout_ms == 0 ) {
    m_artnet_timeout_next_ms = 0;
  } else {
    m_artnet_timeout_next_ms = millis() + m_ConfigServer.m_artnet_timeout_ms;
  }

  m_is_started = true;

  return m_is_started;
}

void ESP32Artnet2DMX::Stop() {

  this->StopDMX();

  m_WiFiUDP.stop();

  this->StrobeOff();

  m_is_started          = false;

  return;
}

bool ESP32Artnet2DMX::IsStarted() {
  return m_is_started;
}

void ESP32Artnet2DMX::Update() {

  if( m_ConfigServer.Update() ) {
    
    // Network changes, so restart everything.
    if( m_ConfigServer.ChangedNetwork() ) {
      this->Stop();
      this->Start();
    }
    
    // DMX config changed, relating to IO.
    if( m_ConfigServer.ChangedDMXConfig() ) {
      this->StopDMX();
      this->StartDMX();
    }

    // Strobe config changed, so build the changed strobe buffer.
    if( m_ConfigServer.ChangedStrobeConfig() ) {
      this->BuildStrobeBuffer();
    }
  }

  this->CheckForArtNetData();

  // No data from artnet, so clean out dmx data buffer.
  if( ( m_artnet_timeout_next_ms != 0 ) && ( millis() >= m_artnet_timeout_next_ms ) ) {
    m_artnet_timeout_next_ms = 0;
    memset( m_dmx_buffer, 0, sizeof( m_dmx_buffer ) );
  }

  this->SendDMX();

}

void ESP32Artnet2DMX::CheckForArtNetData() {
  int packet_size_in_bytes = m_WiFiUDP.parsePacket();

  if( packet_size_in_bytes == 0 ) {
    return;
  }

  // Read data to clean out socket.
  m_WiFiUDP.read( m_data_buffer, ARTNET_PACKET_MAXSIZE );

  if( packet_size_in_bytes < ARTNET_PACKET_MINSIZE_HEADER ) {
    // Ignore anything that's smaller than expected
    if( packet_size_in_bytes != 0 ) {
      Serial.printf( "Packet ignored with data length = %i\n", packet_size_in_bytes );
    }
    return;
  }

  // Check source of packet here & discard if not from expected source.
  if( m_artnet_source_ipaddress != m_artnet_source_ipaddress_any ) {
    if( m_artnet_source_ipaddress != m_WiFiUDP.remoteIP() ) {
      Serial.printf( "Packet ignored from unexpected source IP.\n" );
      return;
    }
  }

  ArtNetPacketHeader* ptr_header = (ArtNetPacketHeader*)&m_data_buffer[ 0 ];

  // Test for correct packet starting data
  String art_net = String( (char*)ptr_header->m_ID );
  if( !art_net.equals( ARTNET_HEADER_ID ) ) {
    Serial.printf( "Header ID failed = %i\n", packet_size_in_bytes );
    return;
  }

  switch( ptr_header->m_OpCode ) {
    case ARTNET_OPCODE_DMX: {
      this->HandleArtNetDMX( (ArtNetPacketDMX*)&m_data_buffer[ ARTNET_PACKET_PAYLOAD_START ] );
      break;
    }
    case ARTNET_OPCODE_POLL: {
      break;
    }
    case ARTNET_OPCODE_POLLREPLY: {
      break;
    }
    default: {
      Serial.printf( "Unhandled OpCode %i\n", ptr_header->m_OpCode );
      break;
    }
  }
}

void ESP32Artnet2DMX::HandleArtNetDMX( ArtNetPacketDMX* ptr_packet_artnet )
{
  uint16_t protocol = ptr_packet_artnet->m_ProtocolLo | ptr_packet_artnet->m_ProtocolHi << 8;
  uint16_t universe_in = ptr_packet_artnet->m_SubUni | ptr_packet_artnet->m_Net << 8;
  uint16_t number_of_channels = ptr_packet_artnet->m_Length | ptr_packet_artnet->m_LengthHi << 8;

  // Set new artnet network timeout
  if( m_ConfigServer.m_artnet_timeout_ms != 0 ) {
    m_artnet_timeout_next_ms = millis() + m_ConfigServer.m_artnet_timeout_ms;
  }

  // Is this the universe we are looking for?
  if( universe_in != m_ConfigServer.m_artnet_universe ) {
    return;
  }

  // Handle strobe from artnet packet.
  if( m_ConfigServer.m_strobe_listening_channel > 0 ) {
    if( ptr_packet_artnet->m_Data[ m_ConfigServer.m_strobe_listening_channel - 1 ] == 0 ) {
      if( m_strobe_on ) {
        this->StrobeOff();
      }
    } else {
      for( int strobe_rate = 0; strobe_rate < 5; strobe_rate++ ) {
        if( m_strobe_rate_current != strobe_rate &&
            ptr_packet_artnet->m_Data[ m_ConfigServer.m_strobe_listening_channel - 1 ] == m_ConfigServer.m_strobe_value[ strobe_rate ] ) {
          m_strobe_on                 = true;
          m_strobe_rate_current       = strobe_rate;
          m_strobe_value              = m_ConfigServer.m_strobe_value[ strobe_rate ];
          m_strobe_delay              = m_ConfigServer.m_strobe_delay[ strobe_rate ];
          m_strobe_duration           = m_ConfigServer.m_strobe_duration[ strobe_rate ];
          m_strobe_delay_current      = 1;
          m_strobe_duration_current   = 0;
        }
      }
    }
  }

  // Note: m_dmx_buffer[ 0 ] must be 0x00 which is DMX null start code.  Actual dmx channel data will start at m_dmx_buffer[ 1 ]
  //       ptr_packet_artnet->m_Data[ 0 ] relates to first channel data, so the array needs to be adjusted.
  if( m_ConfigServer.m_channel_mods_copy_artnet_to_dmx ) {
    memcpy( &m_dmx_buffer[ 1 ], ptr_packet_artnet->m_Data, number_of_channels * sizeof( uint8_t ) );
  } else {
    memset( m_dmx_buffer, 0, sizeof( m_dmx_buffer ) );
  }

  // Process any channel mods
  for( const ChannelMod& mod : m_ConfigServer.GetModsVector() ) {
    if( mod.m_channel < 513 ) {
      switch( mod.m_mod_type ) {
        case CHANNELMODTYPE::EQUALS_VALUE: {
          m_dmx_buffer[ mod.m_channel ] = mod.m_mod_value;
          break;
        }
        case CHANNELMODTYPE::ADD_VALUE: {
          if( m_dmx_buffer[ mod.m_channel ] > 255 - mod.m_mod_value ) {
            m_dmx_buffer[ mod.m_channel ] = 255;
          } else {
            m_dmx_buffer[ mod.m_channel ] += (uint8_t) mod.m_mod_value;
          }
          break;
        }
        case CHANNELMODTYPE::MINUS_VALUE: {
          if( m_dmx_buffer[ mod.m_channel ] < mod.m_mod_value ) {
            m_dmx_buffer[ mod.m_channel ] = 0;
          } else {
            m_dmx_buffer[ mod.m_channel ] -= (uint8_t) mod.m_mod_value;
          }
          break;
        }
        case CHANNELMODTYPE::COPY_FROM_CHANNEL: {
          m_dmx_buffer[ mod.m_channel ] = m_dmx_buffer[ mod.m_mod_value ];
          break;
        }
        case CHANNELMODTYPE::ADD_FROM_CHANNEL: {
          if( m_dmx_buffer[ mod.m_channel ] > 255 - m_dmx_buffer[ mod.m_mod_value ] ) {
            m_dmx_buffer[ mod.m_channel ] = 255;
          } else {
            m_dmx_buffer[ mod.m_channel ] += m_dmx_buffer[ mod.m_mod_value ];
          }
          break;
        }
        case CHANNELMODTYPE::MINUS_FROM_CHANNEL: {
          if( m_dmx_buffer[ mod.m_channel ] < m_dmx_buffer[ mod.m_mod_value ] ) {
            m_dmx_buffer[ mod.m_channel ] = 0;
          } else {
            m_dmx_buffer[ mod.m_channel ] -= m_dmx_buffer[ mod.m_mod_value ];
          }
          break;
        }
        case CHANNELMODTYPE::ABOVE_0_ADD_VALUE: {
          if( m_dmx_buffer[ mod.m_channel ] > 0 ) {
            if( m_dmx_buffer[ mod.m_channel ] > 255 - mod.m_mod_value ) {
              m_dmx_buffer[ mod.m_channel ] = 255;
            } else {
              m_dmx_buffer[ mod.m_channel ] += (uint8_t) mod.m_mod_value;
            }
          }
          break;
        }
        case CHANNELMODTYPE::ABOVE_0_MINUS_VALUE: {
          if( m_dmx_buffer[ mod.m_channel ] > 0 ) {
            if( m_dmx_buffer[ mod.m_channel ] < mod.m_mod_value ) {
              m_dmx_buffer[ mod.m_channel ] = 0;
            } else {
              m_dmx_buffer[ mod.m_channel ] -= (uint8_t) mod.m_mod_value;
            }
          }
          break;
        }
        case CHANNELMODTYPE::COPY_FROM_ARTNET: {
          m_dmx_buffer[ mod.m_channel ] = ptr_packet_artnet->m_Data[ mod.m_mod_value - 1 ];
          break;
        }
        case CHANNELMODTYPE::ADD_FROM_ARTNET: {
          if( m_dmx_buffer[ mod.m_channel ] > 255 - ptr_packet_artnet->m_Data[ mod.m_mod_value - 1 ] ) {
            m_dmx_buffer[ mod.m_channel ] = 255;
          } else {
            m_dmx_buffer[ mod.m_channel ] += ptr_packet_artnet->m_Data[ mod.m_mod_value - 1 ];
          }
          break;
        }
        case CHANNELMODTYPE::MINUS_FROM_ARTNET: {
          if( m_dmx_buffer[ mod.m_channel ] < ptr_packet_artnet->m_Data[ mod.m_mod_value - 1 ] ) {
            m_dmx_buffer[ mod.m_channel ] = 0;
          } else {
            m_dmx_buffer[ mod.m_channel ] -= ptr_packet_artnet->m_Data[ mod.m_mod_value - 1 ];
          }
          break;
        }
        case CHANNELMODTYPE::IF_0_ADD_FROM_ARTNET: {
          if( m_dmx_buffer[ mod.m_channel ] == 0 ) {
            m_dmx_buffer[ mod.m_channel ] = ptr_packet_artnet->m_Data[ mod.m_mod_value - 1 ];
          }
          break;
        }
      }
    }
  }
}

void ESP32Artnet2DMX::StartDMX() {
  // Only writing out DMX so no need for personalities
  dmx_config_t config = DMX_CONFIG_DEFAULT;
  dmx_personality_t personalities[] = {};
  int personality_count = 0;

  dmx_driver_install( DMX_NUM_1, &config, personalities, personality_count );

  dmx_set_pin( DMX_NUM_1, m_ConfigServer.m_gpio_transmit, m_ConfigServer.m_gpio_receive, m_ConfigServer.m_gpio_enable );
}
  
void ESP32Artnet2DMX::StopDMX() {
  if( dmx_driver_is_installed( DMX_NUM_1 ) ) {
    dmx_driver_delete( DMX_NUM_1 ) ;
  }
}

void ESP32Artnet2DMX::SendDMX() {
  if( !m_ConfigServer.m_dmx_enabled ) {
    return;
  }

  if( dmx_wait_sent( DMX_NUM_1, 0 ) ) {
    if( m_strobe_on ) {
      if( --m_strobe_delay_current == 0 ) {
        m_strobe_delay_current    = m_strobe_delay;
        m_strobe_duration_current = m_strobe_duration;
      }
      if( m_strobe_duration_current > 0 ) {
        dmx_write( DMX_NUM_1, m_ConfigServer.m_strobe_buffer_on, DMX_PACKET_SIZE );
        --m_strobe_duration_current;
      } else {
        dmx_write( DMX_NUM_1, m_ConfigServer.m_strobe_buffer_off, DMX_PACKET_SIZE );
      }
    } else {
      dmx_write( DMX_NUM_1, m_dmx_buffer, DMX_PACKET_SIZE );
    }
    dmx_send_num( DMX_NUM_1, DMX_PACKET_SIZE );
  }
}

void ESP32Artnet2DMX::StrobeOff() {
  m_strobe_on           = false;
  m_strobe_rate_current = 5;
  m_strobe_value        = 0;
  m_strobe_delay        = 0;
  m_strobe_duration     = 0;
}

void ESP32Artnet2DMX::BuildStrobeBuffer() {
  if( m_strobe_on ) {
    if( m_ConfigServer.m_strobe_listening_channel == 0 ) {
      this->StrobeOff();
    } else {
      if( m_strobe_value != m_ConfigServer.m_strobe_value[ m_strobe_rate_current ] || 
          m_strobe_delay != m_ConfigServer.m_strobe_delay[ m_strobe_rate_current ] || 
          m_strobe_duration != m_ConfigServer.m_strobe_duration[ m_strobe_rate_current ] ) {
        // The current on strobing effect has been changed.
        m_strobe_value              = m_ConfigServer.m_strobe_value[ m_strobe_rate_current ];
        m_strobe_delay              = m_ConfigServer.m_strobe_delay[ m_strobe_rate_current ];
        m_strobe_duration           = m_ConfigServer.m_strobe_duration[ m_strobe_rate_current ];
        m_strobe_delay_current      = m_strobe_delay;
        m_strobe_duration_current   = m_strobe_duration;
      }
    }
  }
}
