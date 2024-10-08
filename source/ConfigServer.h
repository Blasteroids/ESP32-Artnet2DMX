#ifndef _CONFIGSERVER_H_
#define _CONFIGSERVER_H_

#include <vector>
#include <WiFi.h>
#include <WebServer.h>
#include <uri/UriBraces.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "FS.h"
#include "WebpageBuilder.h"
#include "ChannelModsHandler.h"

const String HOTSPOT_SSID = "ESP32_ArtNet2DMX";
const String HOTSPOT_PASS = "1234567890";  // Has to be minimum 10 digits?

const String CONFIG_ADAPTER = "/config_adapter.json";
const String CONFIG_MODS    = "/config_mods.json";

class ConfigServer {
public:
  ConfigServer();

  ~ConfigServer();
  
  void Init();

  // Returns true if connected to WiFi, false if gone into WiFi setup AP.
  bool ConnectToWiFi();
  
  bool IsConnectedToWiFi();

  void StartWebServer();

  // Returns true if settings have changed.
  bool Update();
  
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
  String          m_artnet_source_ip;        // The IP that we're expecting data from.  Use 255.255.255.255 for any.
  int             m_artnet_universe;         // Universe to listen for, all other universes are ignored.  Default = 1
  unsigned long   m_artnet_timeout_ms;       // When no artnet data has been received by this amount of ms then turn off all dmx.  Default = 2000.  Use -1 for no timeout.
  unsigned long   m_dmx_update_interval_ms;  // The interval between updating the dmx line in ms.  Default = 23
  bool            m_dmx_enabled;             // Enable/Disable dmx output.

  // DMX channel mods
  bool m_channel_mods_copy_artnet_to_dmx;

  const std::vector< ChannelMod >& GetModsVector() const;


private:
  void ResetConfigToDefault();
  void ResetWiFiToDefault();
  void ResetESP32PinsToDefault();
  void ResetArtnet2DMXToDefault();  
  void ResetChannelModsToDefault();

  void SettingsSave();
  bool SettingsLoad();
  
  void SendSetupMenuPage();
  void SendWiFiSetupPage();
  void SendESP32PinsSetupPage();
  void SendArtnet2DMXSetupPage();
  void SendChannelModsSetupPage();
  void SendChannelModsForChannelSetupPage( int channel_number );
  void SendModConfigFile();
  void Send200Response();

  void HandleResetAll();
  void HandleResetWiFi();
  void HandleResetESP32Pins();
  void HandleResetArtnet2DMX();
  void HandleResetChannelMods();

  void HandleDMXEnable();
  void HandleDMXDisable();
  void HandleCopyArtnetToDMXEnable();
  void HandleCopyArtnetToDMXDisable();

  void HandleSetupWiFi();
  void HandleSetupESP32Pins();
  void HandleSetupArtnet2DMX();
  void HandleSetupChannelMods();
  void HandleSetupChannelModsForChannel();
  void HandleChannelModsEditFor();
  void HandleChannelModsRemoveFor();
  void HandleChannelModsAddFor();
  void HandleChannelModsDelFor();

  void HandleWebServerDataOnNotFound();

  void HandleFileUpload();

  WebServer          m_WebServer;
  WebpageBuilder     m_WebpageBuilder;
  String             m_mac_address;
  bool               m_settings_changed;
  bool               m_is_connected_to_wifi;
  File               m_file_being_uploaded;
  ChannelModsHandler m_ChannelModsHandler;
};

#endif
