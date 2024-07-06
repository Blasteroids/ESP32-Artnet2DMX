#ifndef _CONFIGSERVER_H_
#define _CONFIGSERVER_H_

#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "FS.h"
#include "WebpageBuilder.h"

const String HOTSPOT_SSID = "ESP32_ArtNet2DMX";
const String HOTSPOT_PASS = "1234567890";  // Has to be minimum 10 digits?

const String CONFIG_FILENAME = "/config.json";

class ConfigServer {
public:
  ConfigServer();

  ~ConfigServer();
  
  void Init();

  // Returns true if connected to WiFi, false if gone into WiFi setup AP.
  bool ConnectToWiFi();
  
  bool IsConnectedToWiFi();

  void StartWebServer( WebServer* ptr_WebServer );

  // Returns true if settings have changed.
  bool Update();
  
  void HandleWebServerData();

  // WiFi settings
  String m_wifi_ssid;
  String m_wifi_pass;
  String m_wifi_ip;
  String m_wifi_subnet;

  // ESP32 settings
  int m_gpio_enable;       // Connect to DE & RE on MAX485.  Default = 21
  int m_gpio_transmit;     // Connected to DI on MAX485.  Default = 33
  int m_gpio_receive;      // Ensure pin is not connected to anything.  Default = 38

  // Artnet 2 DMX settings
  String m_artnet_source_ip;        // The IP that we're expecting data from.  Use 255.255.255.255 for any.
  int    m_artnet_universe;         // Universe to listen for, all other universes are ignored.  Default = 1
  long   m_artnet_timeout_ms;       // When no artnet data has been received by this amount of ms then turn off all dmx.  Default = 2000.  Use -1 for no timeout.
  long   m_dmx_update_interval_ms;  // The interval between updating the dmx line in ms.  Default = 23

private:
  void ResetConfigToDefault();
  void ResetWiFiToDefault();
  void ResetESP32PinsToDefault();
  void ResetArtnet2DMXToDefault();  

  void SettingsSave();
  bool SettingsLoad();
  
  void SendSetupMenuPage();
  void SendWiFiSetupPage();
  void SendESP32PinsSetupPage();
  void SendArtnet2DMXSetupPage();

  bool HandleWebGet();
  bool HandleWebPost();

  bool HandleSetupWiFi();
  bool HandleSetupESP32Pins();
  bool HandleSetupArtnet2DMX();

  WebServer*     m_ptr_WebServer;
  WebpageBuilder m_WebpageBuilder;
  
  bool m_settings_changed;
  bool m_is_connected_to_wifi;
};

#endif
