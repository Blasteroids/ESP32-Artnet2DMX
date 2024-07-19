#include "Print.h"

#include "ESP32Artnet2DMX.h"

ESP32Artnet2DMX::ESP32Artnet2DMX() {
  memset( m_dmx_buffer, 0, sizeof( m_dmx_buffer ) );

  m_artnet_source_ipaddress_any.fromString( "255.255.255.255" );

  m_is_started = false;
}

ESP32Artnet2DMX::~ESP32Artnet2DMX() {
  this->Stop();
}

void ESP32Artnet2DMX::Init( WebServer* ptr_WebServer ) {

  // Init must be called because class constructor is not called by default on global var.
  m_ConfigServer.Init();

  // Attempt to connect to WiFi.  On failure will create a hotspot.
  m_ConfigServer.ConnectToWiFi();

  // Startup the webserver.
  m_ConfigServer.StartWebServer( ptr_WebServer );

  m_is_started = false;
}

bool ESP32Artnet2DMX::Start() {

  // Only writing out DMX so no need for personalities
  dmx_config_t config = DMX_CONFIG_DEFAULT;
  dmx_personality_t personalities[] = {};
  int personality_count = 0;

  dmx_driver_install( DMX_NUM_1, &config, personalities, personality_count );

  dmx_set_pin( DMX_NUM_1, m_ConfigServer.m_gpio_transmit, m_ConfigServer.m_gpio_receive, m_ConfigServer.m_gpio_enable );

  if( !m_WiFiUDP.begin( ARTNET_UDP_PORT ) ) {
    Serial.print("Failed to create Art-Net network socket on UDP port 6464\n");
    return false;
  }

  // Store expected source IP for artnet packets.
  m_artnet_source_ipaddress.fromString( m_ConfigServer.m_artnet_source_ip );

  m_dmx_update_time_next_ms = millis();

  if( m_ConfigServer.m_artnet_timeout_ms == 0 ) {
    m_artnet_timeout_next_ms = 0;
  } else {
    m_artnet_timeout_next_ms = m_dmx_update_time_next_ms + m_ConfigServer.m_artnet_timeout_ms;
  }

  m_is_started = true;

  return m_is_started;
}

void ESP32Artnet2DMX::Stop() {
  if( dmx_driver_is_installed( DMX_NUM_1 ) ) {
    dmx_driver_delete( DMX_NUM_1 ) ;
  }

  m_WiFiUDP.stop();

  m_is_started = false;
  return;
}

bool ESP32Artnet2DMX::IsStarted() {
  return m_is_started;
}

void ESP32Artnet2DMX::Update() {

  if( m_ConfigServer.Update() ) {
    // Settings have changed.
    this->Stop();
    this->Start();
  }

  this->CheckForArtNetData();

  // Target 23ms for sending updates.
  if( millis() >= m_dmx_update_time_next_ms ) {
    this->SendDMX();
  }

  if( ( m_artnet_timeout_next_ms != 0 ) && ( millis() >= m_artnet_timeout_next_ms ) ) {
    m_artnet_timeout_next_ms = 0;
    memset( m_dmx_buffer, 0, sizeof( m_dmx_buffer ) );
    this->SendDMX();
  }
}

void ESP32Artnet2DMX::HandleWebServerData() {
  m_ConfigServer.HandleWebServerData();
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
      //Serial.printf( "GOT : DMX Packet\n" );
      this->HandleArtNetDMX( (ArtNetPacketDMX*)&m_data_buffer[ ARTNET_PACKET_PAYLOAD_START ] );
      break;
    }
    case ARTNET_OPCODE_POLL: {
      //Serial.printf( "GOT : POLL Packet\n" );
      break;
    }
    case ARTNET_OPCODE_POLLREPLY: {
      //Serial.printf( "GOT : POLLREPLY Packet\n" );
      break;
    }
    default: {
      Serial.printf( "Unhandled OpCode %i\n", ptr_header->m_OpCode );
      break;
    }
  }
}

void ESP32Artnet2DMX::HandleArtNetDMX( ArtNetPacketDMX* ptr_packetdmx )
{
  uint16_t protocol = ptr_packetdmx->m_ProtocolLo | ptr_packetdmx->m_ProtocolHi << 8;
  uint16_t universe_in = ptr_packetdmx->m_SubUni | ptr_packetdmx->m_Net << 8;
  uint16_t number_of_channels = ptr_packetdmx->m_Length | ptr_packetdmx->m_LengthHi << 8;
/*
  Serial.printf(" Target protocol = %i\n", ARTNET_VERSION );
  Serial.printf(" Protocol = %i  Universe = %i  Sequence = %i  Nof channels = %i\n", protocol, universe_in, ptr_packetdmx->m_Sequence, number_of_channels );

  for( int i = 0; i < number_of_channels; i++ ) {
    Serial.print( ptr_packetdmx->m_Data[ i ], HEX );
    Serial.print( " " );
  }
  Serial.print( "\n");
*/
  // Set new artnet network timeout
  if( m_ConfigServer.m_artnet_timeout_ms != 0 ) {
    m_artnet_timeout_next_ms = millis() + m_ConfigServer.m_artnet_timeout_ms;
  }

  // Is this the universe we are looking for?
  if( universe_in != m_ConfigServer.m_artnet_universe ) {
    return;
  }

  // Note: m_dmx_buffer[ 0 ] must be 0x00 which is DMX null start code.  Actual dmx channel data will start at m_dmx_buffer[ 1 ]
  //       ptr_packetdmx->m_Data[ 0 ] relates to first channel data, so the array needs to be adjusted.
  memcpy( &m_dmx_buffer[ 1 ], ptr_packetdmx->m_Data, number_of_channels * sizeof( uint8_t ) );

  // Process any channel mods
  for( auto& mod: m_ConfigServer.m_channel_mods_vector ) {
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
      }
    }
  }

  // DMX data will be sent on m_dmx_update_time_next_ms
}

void ESP32Artnet2DMX::SendDMX()
{
  if( !m_ConfigServer.m_dmx_enabled ) {
    return;
  }

  dmx_write( DMX_NUM_1, m_dmx_buffer, DMX_PACKET_SIZE );
  dmx_send_num( DMX_NUM_1, DMX_PACKET_SIZE );
  dmx_wait_sent( DMX_NUM_1, DMX_TIMEOUT_TICK );
  m_dmx_update_time_next_ms += m_ConfigServer.m_dmx_update_interval_ms;
}
