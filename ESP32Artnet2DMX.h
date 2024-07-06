#ifndef _ESP32ARTNET2DMX_
#define _ESP32ARTNET2DMX_

//
#include <iostream>
#include <string>
//
#include <WiFi.h>   // WiFi Ref. : https://www.arduino.cc/reference/en/libraries/wifi/
#include <WebServer.h>
#include <esp_dmx.h>
//
#include "ConfigServer.h"
#include "ArtNet_Spec.h"

class ESP32Artnet2DMX {
public:
  ESP32Artnet2DMX();

  ~ESP32Artnet2DMX();

  void Init( WebServer* ptr_WebServer );

  bool Start();

  bool IsStarted();

  void Update();

  void Stop();

  void HandleWebServerData();

private:  
  void SendDMX();

  void CheckForArtNetData();

  void HandleArtNetDMX( ArtNetPacketDMX* ptr_packetdmx );

  bool          m_is_started;
  
  long          m_artnet_timeout_next_ms;

  long          m_dmx_update_time_next_ms;

  uint8_t       m_data_buffer[ ARTNET_PACKET_MAXSIZE ];

  uint8_t       m_dmx_buffer[ 513 ];

  WiFiUDP       m_WiFiUDP;

  // Config
  ConfigServer  m_ConfigServer;

  IPAddress     m_artnet_source_ipaddress;
  IPAddress     m_artnet_source_ipaddress_any;
};

#endif

