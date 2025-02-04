#include <vector>
#include "ConfigServer.h"

ConfigServer::ConfigServer() {
  m_changed_network       = false;
  m_changed_dmx_config    = false;
  m_changed_strobe_config = false;
  m_is_connected_to_wifi  = false;

  // Start with config reset, except for WiFi
  this->ResetESP32PinsToDefault();
  this->ResetArtnet2DMXToDefault();
  this->ResetChannelModsToDefault();
  this->ResetStrobeConfigToDefault();

  // No DMX output until config loaded
  m_dmx_enabled = false;
}

ConfigServer::~ConfigServer() {
}

void ConfigServer::Init() {

  if( !this->SettingsLoad() ) {
    Serial.println( "Settings failed to load - Resetting to default." );
    this->ResetConfigToDefault();
    this->SettingsSave();
  }

  m_changed_network      = true;
  m_changed_dmx_config   = true;
}

void ConfigServer::ResetConfigToDefault() {

  // Remove existing settings file.
  if( LittleFS.begin( false ) ) {
    LittleFS.remove( CONFIG_ADAPTER );
    LittleFS.remove( CONFIG_MODS );
  }

  // Reset all config
  this->ResetWiFiToDefault();
  this->ResetESP32PinsToDefault();
  this->ResetArtnet2DMXToDefault();
  this->ResetChannelModsToDefault();
  this->ResetStrobeConfigToDefault();

  // Disable DMX output
  m_dmx_enabled = false;
}

void ConfigServer::ResetWiFiToDefault() {
  m_is_connected_to_wifi = false;

  // No wifi setup on default, force AP
  m_wifi_ssid   = "";
  m_wifi_pass   = "";
  m_wifi_ip     = "";
  m_wifi_subnet = "";
}

void ConfigServer::ResetESP32PinsToDefault() {
  // DMX settings
  m_gpio_enable      = 21;  // Connect to DE & RE on MAX485.
  m_gpio_transmit    = 33;  // Connected to DI on MAX485.
  m_gpio_receive     = 38;  // Ensure pin is not connected to anything.
}

void ConfigServer::ResetArtnet2DMXToDefault() {
  m_artnet_source_ip       = "255.255.255.255";  // Any IP source is fine.
  m_artnet_universe        = 1;                  // Universe to listen for, all other universes are ignored.
  m_artnet_timeout_ms      = 3000;               // Artnet timeout
}

void ConfigServer::ResetChannelModsToDefault() {
  m_channel_mods_copy_artnet_to_dmx = true;
  m_ChannelModsHandler.Clear();
}

void ConfigServer::ResetStrobeConfigToDefault() {
  m_strobe_listening_channel  = 0;       // 0 - No strobe
  m_strobe_all_channels       = false;
  m_strobe_all_channels_value = 126;
  for( int i = 0; i < 5; i++ ) {
    m_strobe_value[ i ] = i;
    m_strobe_delay[ i ] = 0;
    m_strobe_duration[ i ] = 0;
  }
  memset( m_strobe_buffer_on, 0, sizeof( m_strobe_buffer_on ) );
  memset( m_strobe_buffer_off, 0, sizeof( m_strobe_buffer_off ) );
}

void ConfigServer::SettingsSave() {
  // Start LittleFS
  if( !LittleFS.begin( false ) ) {
    Serial.println( "LittleFS failed.  Attempting format." );
    if( !LittleFS.begin( true ) ) {
      Serial.println( "LittleFS failed format. Config saving aborted." );
      return;     
    } else {
      Serial.println( "LittleFS: Formatted" );
    }
  }

  // Adapter config
  DynamicJsonDocument doc( 32768 );
  doc[ "wifi_ssid" ]              = m_wifi_ssid;
  doc[ "wifi_pass" ]              = m_wifi_pass;
  doc[ "wifi_ip" ]                = m_wifi_ip;
  doc[ "wifi_subnet" ]            = m_wifi_subnet;
  doc[ "gpio_enable" ]            = m_gpio_enable;
  doc[ "gpio_transmit" ]          = m_gpio_transmit;
  doc[ "gpio_receive" ]           = m_gpio_receive;
  doc[ "artnet_source_ip" ]       = m_artnet_source_ip;
  doc[ "artnet_universe" ]        = m_artnet_universe;
  doc[ "artnet_timeout_ms" ]      = m_artnet_timeout_ms;
  doc[ "dmx_enabled" ]            = m_dmx_enabled;
  
  // Strobe
  doc[ "strobe_listening_channel" ]  = m_strobe_listening_channel;
  doc[ "strobe_all_channels" ]       = m_strobe_all_channels;
  doc[ "strobe_all_channels_value" ] = m_strobe_all_channels_value;
  doc[ "strobe_value_0" ]            = m_strobe_value[ 0 ];
  doc[ "strobe_delay_0" ]            = m_strobe_delay[ 0 ];
  doc[ "strobe_duration_0" ]         = m_strobe_duration[ 0 ];
  doc[ "strobe_value_1" ]            = m_strobe_value[ 1 ];
  doc[ "strobe_delay_1" ]            = m_strobe_delay[ 1 ];
  doc[ "strobe_duration_1" ]         = m_strobe_duration[ 1 ];
  doc[ "strobe_value_2" ]            = m_strobe_value[ 2 ];
  doc[ "strobe_delay_2" ]            = m_strobe_delay[ 2 ];
  doc[ "strobe_duration_2" ]         = m_strobe_duration[ 2 ];
  doc[ "strobe_value_3" ]            = m_strobe_value[ 3 ];
  doc[ "strobe_delay_3" ]            = m_strobe_delay[ 3 ];
  doc[ "strobe_duration_3" ]         = m_strobe_duration[ 3 ];
  doc[ "strobe_value_4" ]            = m_strobe_value[ 4 ];
  doc[ "strobe_delay_4" ]            = m_strobe_delay[ 4 ];
  doc[ "strobe_duration_4" ]         = m_strobe_duration[ 4 ];

  // Strobe array
  JsonArray array = doc.createNestedArray( "strobe_data" );
  for( int i = 0; i < 513; ++i ) {
    array.add( m_strobe_buffer_on[ i ] );
  }

  File config_adapter = LittleFS.open( CONFIG_ADAPTER, "w" );
  serializeJson( doc, config_adapter );
  config_adapter.close();

  // Clear out json
  doc.clear();

  doc[ "copy_artnet_to_dmx" ] = m_channel_mods_copy_artnet_to_dmx;

  // Mods config
  JsonArray array_channelmods = doc.createNestedArray( "channel_mods" );

  for( const ChannelMod& mod : m_ChannelModsHandler.GetModsVector() ) {
    JsonObject obj     = array_channelmods.createNestedObject();
    obj[ "sequence" ]  = mod.m_sequence;
    obj[ "channel" ]   = mod.m_channel;
    obj[ "mod_type" ]  = mod.m_mod_type;
    obj[ "mod_value" ] = mod.m_mod_value;
  }

  File config_mods = LittleFS.open( CONFIG_MODS, "w" );
  serializeJson( doc, config_mods );
  config_mods.close();
}

bool ConfigServer::SettingsLoad() {
  if( !LittleFS.begin( false ) ) {
    // Failed to start LittleFS, probably no save.
    Serial.println( "Failed to start LittleFS" );
    return false;
  }

  DynamicJsonDocument doc( 32768 );
  File config_adapter = LittleFS.open( CONFIG_ADAPTER, "r" );

  if( !config_adapter ) {
    // File not exist.
    Serial.println( "Failed to load adapter config file." );
    return false;
  }

  // Adapter config
  deserializeJson( doc, config_adapter );
  config_adapter.close();

  m_wifi_ssid              = doc[ "wifi_ssid" ].as<String>();
  m_wifi_pass              = doc[ "wifi_pass" ].as<String>();
  m_wifi_ip                = doc[ "wifi_ip" ].as<String>();
  m_wifi_subnet            = doc[ "wifi_subnet" ].as<String>();
  m_gpio_enable            = doc[ "gpio_enable" ];
  m_gpio_transmit          = doc[ "gpio_transmit" ];
  m_gpio_receive           = doc[ "gpio_receive" ];
  m_artnet_source_ip       = doc[ "artnet_source_ip" ].as<String>();
  m_artnet_universe        = doc[ "artnet_universe" ];
  m_artnet_timeout_ms      = doc[ "artnet_timeout_ms" ];
  m_dmx_enabled            = doc[ "dmx_enabled" ];

  // Strobe
  m_strobe_listening_channel  = doc[ "strobe_listening_channel" ];
  m_strobe_all_channels       = doc[ "strobe_all_channels" ];
  m_strobe_all_channels_value = doc[ "strobe_all_channels_value" ];
  m_strobe_value[ 0 ]         = doc[ "strobe_value_0" ];
  m_strobe_delay[ 0 ]         = doc[ "strobe_delay_0" ];
  m_strobe_duration[ 0 ]      = doc[ "strobe_duration_0" ];
  m_strobe_value[ 1 ]         = doc[ "strobe_value_1" ];
  m_strobe_delay[ 1 ]         = doc[ "strobe_delay_1" ];
  m_strobe_duration[ 1 ]      = doc[ "strobe_duration_1" ];
  m_strobe_value[ 2 ]         = doc[ "strobe_value_2" ];
  m_strobe_delay[ 2 ]         = doc[ "strobe_delay_2" ];
  m_strobe_duration[ 2 ]      = doc[ "strobe_duration_2" ];
  m_strobe_value[ 3 ]         = doc[ "strobe_value_3" ];
  m_strobe_delay[ 3 ]         = doc[ "strobe_delay_3" ];
  m_strobe_duration[ 3 ]      = doc[ "strobe_duration_3" ];
  m_strobe_value[ 4 ]         = doc[ "strobe_value_4" ];
  m_strobe_delay[ 4 ]         = doc[ "strobe_delay_4" ];
  m_strobe_duration[ 4 ]      = doc[ "strobe_duration_4" ];

  // Strobe array
  JsonArray array = doc[ "strobe_data" ];
  for( int i = 0; i < 513; ++i ) {
    m_strobe_buffer_on[ i ] = array[ i ];
  }

  // Clear out json
  doc.clear();

  // Mods config
  File config_mods = LittleFS.open( CONFIG_MODS, "r" );

  if( !config_mods ) {
    // File not exist.
    Serial.println( "Failed to load adapter config file." );
    return false;
  }

  deserializeJson( doc, config_mods );
  config_mods.close();

  m_channel_mods_copy_artnet_to_dmx = doc[ "copy_artnet_to_dmx" ];

  m_ChannelModsHandler.Clear();

  JsonArray array_channelmods = doc[ "channel_mods" ];
  for( const JsonObject& obj : array_channelmods ) {
    ChannelMod mod;
    mod.m_sequence  = obj[ "sequence" ];
    mod.m_channel   = obj[ "channel" ];
    mod.m_mod_type  = obj[ "mod_type" ];
    mod.m_mod_value = obj[ "mod_value" ];
    m_ChannelModsHandler.AddMod( mod );
  }

  return true;
}

bool ConfigServer::ConnectToWiFi() {
  if( m_is_connected_to_wifi ) {
    WiFi.disconnect();
    m_is_connected_to_wifi = false;
  }

  WiFi.begin();
  m_mac_address = WiFi.macAddress();

  IPAddress ip( 192, 168, 1, 1 );
  IPAddress subnet( 255, 255, 255, 0 );

  if( m_wifi_ssid.length() != 0 ) {
    WiFi.mode( WIFI_STA );
    WiFi.begin( m_wifi_ssid, m_wifi_pass );
  
    int timeout = 20;
    Serial.print( "Connecting to WiFi" );
    while( WiFi.status() != WL_CONNECTED && timeout-- > 0 ) {
      delay( 1000 );
      Serial.print( "." );
    }

    if( timeout > 0 ) {
      if( m_wifi_ip.length() > 0 ) {
        ip.fromString( m_wifi_ip );
        subnet.fromString( m_wifi_subnet );

        WiFi.config( ip, ip, subnet );
      }
      Serial.printf( "\nConnected to WiFi. IP = " );
      Serial.println( WiFi.localIP() );
      m_is_connected_to_wifi = true;
      return true;
    }
  }

  // Ensure any old wifi ssid gets removed.
  m_wifi_ssid = "";

  Serial.println( "No config found or WiFi timeout." );
  WiFi.mode( WIFI_AP );
  WiFi.softAP( HOTSPOT_SSID, HOTSPOT_PASS );  
  WiFi.softAPConfig( ip, ip, subnet );
  
  Serial.printf( "WiFi started in AP mode with IP = " );
  Serial.println( ip );
  
  m_is_connected_to_wifi = false;

  return false;
}

bool ConfigServer::IsConnectedToWiFi() {
  return m_is_connected_to_wifi;
}

void ConfigServer::StartWebServer() {
  m_WebServer.onNotFound( std::bind( &ConfigServer::HandleWebServerDataOnNotFound, this ) );

  m_WebServer.on( "/", HTTP_GET, std::bind( &ConfigServer::SendSetupMenuPage, this ) );

  m_WebServer.on( "/reset_all", HTTP_GET, std::bind( &ConfigServer::HandleResetAll, this ) );
  m_WebServer.on( "/reset_wifi", HTTP_GET, std::bind( &ConfigServer::HandleResetWiFi, this ) );
  m_WebServer.on( "/reset_esp32pins", HTTP_GET, std::bind( &ConfigServer::HandleResetESP32Pins, this ) );
  m_WebServer.on( "/reset_artnew2dmx", HTTP_GET, std::bind( &ConfigServer::HandleResetArtnet2DMX, this ) );
  m_WebServer.on( "/reset_channelmods", HTTP_GET, std::bind( &ConfigServer::HandleResetChannelMods, this ) );
  m_WebServer.on( "/reset_strobeconfig", HTTP_GET, std::bind( &ConfigServer::HandleResetStrobeConfig, this ) );

  m_WebServer.on( "/settings_wifi", HTTP_GET, std::bind( &ConfigServer::SendWiFiSetupPage, this ) );
  m_WebServer.on( "/settings_esp32pins", HTTP_GET, std::bind( &ConfigServer::SendESP32PinsSetupPage, this ) );
  m_WebServer.on( "/settings_artnet2dmx", HTTP_GET, std::bind( &ConfigServer::SendArtnet2DMXSetupPage, this ) );
  m_WebServer.on( "/settings_strobeconfig", HTTP_GET, std::bind( &ConfigServer::SendStrobeConfigSetupPage, this ) );
  m_WebServer.on( "/settings_channelmods", HTTP_GET, std::bind( &ConfigServer::SendChannelModsSetupPage, this ) );
  m_WebServer.on( "/download", HTTP_GET, std::bind( &ConfigServer::SendModConfigFile, this ) ); // There's only 1 download, so ignoring filename.

  m_WebServer.on( "/upload", HTTP_POST, std::bind( &ConfigServer::Send200Response, this ), std::bind( &ConfigServer::HandleFileUpload, this ) );
  m_WebServer.on( "/dmx_enable", HTTP_POST, std::bind( &ConfigServer::HandleDMXEnable, this ) );
  m_WebServer.on( "/dmx_disable", HTTP_POST, std::bind( &ConfigServer::HandleDMXDisable, this ) );
  m_WebServer.on( "/copy_artnet_enable", HTTP_POST, std::bind( &ConfigServer::HandleCopyArtnetToDMXEnable, this ) );
  m_WebServer.on( "/copy_artnet_disable", HTTP_POST, std::bind( &ConfigServer::HandleCopyArtnetToDMXDisable, this ) );

  m_WebServer.on( "/setup_wifi", HTTP_POST, std::bind( &ConfigServer::HandleSetupWiFi, this ) );
  m_WebServer.on( "/setup_esp32pins", HTTP_POST, std::bind( &ConfigServer::HandleSetupESP32Pins, this ) );
  m_WebServer.on( "/setup_artnet2dmx", HTTP_POST, std::bind( &ConfigServer::HandleSetupArtnet2DMX, this ) );
  m_WebServer.on( "/setup_channelmods", HTTP_POST, std::bind( &ConfigServer::HandleSetupChannelMods, this ) );
  m_WebServer.on( "/setup_strobeconfig", HTTP_POST, std::bind( &ConfigServer::HandleSetupStrobeConfig, this ) );
  m_WebServer.on( "/setup_strobechannels", HTTP_POST, std::bind( &ConfigServer::HandleSetupStrobeChannels, this ) );
  
  m_WebServer.on( UriBraces("/setup_channelmodsfor/{}"), HTTP_POST, std::bind( &ConfigServer::HandleSetupChannelModsForChannel, this ) );
  m_WebServer.on( UriBraces("/mods_editfor/{}"), HTTP_POST, std::bind( &ConfigServer::HandleChannelModsEditFor, this ) );
  m_WebServer.on( UriBraces("/mods_removefor/{}"), HTTP_POST, std::bind( &ConfigServer::HandleChannelModsRemoveFor, this ) );
  m_WebServer.on( UriBraces("/mods_addfor/{}"), HTTP_POST, std::bind( &ConfigServer::HandleChannelModsAddFor, this ) );
  m_WebServer.on( UriBraces("/mods_delfor/{}/{}"), HTTP_POST, std::bind( &ConfigServer::HandleChannelModsDelFor, this ) );

  m_WebServer.begin();
}

bool ConfigServer::Update() {

  m_changed_network       = false;
  m_changed_dmx_config    = false;
  m_changed_strobe_config = false;

  m_WebServer.handleClient();

  if( m_changed_network | m_changed_dmx_config | m_changed_strobe_config ) {
    return true;
  }

  return false;
}

bool ConfigServer::ChangedNetwork() {
  return m_changed_network;
}

bool ConfigServer::ChangedDMXConfig() {
  return m_changed_dmx_config;
}

bool ConfigServer::ChangedStrobeConfig() {
  return m_changed_strobe_config;
}

const std::vector< ChannelMod >& ConfigServer::GetModsVector() const {
  return m_ChannelModsHandler.GetModsVector();
}

void ConfigServer::SendSetupMenuPage() {

  m_WebpageBuilder.StartPage();
  m_WebpageBuilder.AddTitle( "Artnet2DMX Setup Page" );
  m_WebpageBuilder.StartBody();
  m_WebpageBuilder.StartCenter();
  m_WebpageBuilder.AddHeading( "Artnet2DMX Setup Page" );

  // WiFi settings
  m_WebpageBuilder.AddBreak( 2 );
  m_WebpageBuilder.AddButtonActionForm( "settings_wifi", "WiFi" );
  m_WebpageBuilder.AddBreak( 2 );
  m_WebpageBuilder.AddButtonActionForm( "settings_esp32pins", "ESP32 Pins" );
  m_WebpageBuilder.AddBreak( 2 );
  m_WebpageBuilder.AddButtonActionForm( "settings_artnet2dmx", "Art-Net 2 DMX" );
  m_WebpageBuilder.AddBreak( 2 );
  m_WebpageBuilder.AddButtonActionForm( "settings_strobeconfig", "Strobe Config" );
  m_WebpageBuilder.AddBreak( 2 );
  m_WebpageBuilder.AddButtonActionForm( "settings_channelmods", "Channel Mods" );

  m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddLabel( "note", "It's advisable to disable DMX output during setup." );
  m_WebpageBuilder.AddBreak( 2 );
  m_WebpageBuilder.AddLabel( "dmx_status", "DMX Status" );
  m_WebpageBuilder.AddBreak( 1 );
  if( m_dmx_enabled ) {
    m_WebpageBuilder.AddButtonActionFormPost( "dmx_disable", "ENABLED" );
  } else {
    m_WebpageBuilder.AddButtonActionFormPost( "dmx_enable", "DISABLED" );
  }

  // Reset button
  m_WebpageBuilder.AddBreak( 4 );
  m_WebpageBuilder.AddButtonActionForm( "reset_all", "RESET ALL SETTINGS TO DEFAULT" );

  m_WebpageBuilder.EndCenter();
  m_WebpageBuilder.EndBody();
  m_WebpageBuilder.EndPage();

  m_WebServer.send( 200, "text/html", m_WebpageBuilder.m_html );
}

void ConfigServer::SendWiFiSetupPage() {
  m_WebpageBuilder.StartPage();
  m_WebpageBuilder.AddTitle( "Artnet2DMX Setup Page" );
  m_WebpageBuilder.StartBody();
  m_WebpageBuilder.StartCenter();
  m_WebpageBuilder.AddHeading( "WiFi Setup" );

  // WiFi settings
  m_WebpageBuilder.AddText( "Device MAC = " + m_mac_address );
  m_WebpageBuilder.AddBreak( 2 );
  m_WebpageBuilder.AddFormAction( "/setup_wifi", "POST" );
  m_WebpageBuilder.AddLabel( "wifi_ssid", "WiFi ssid : " );
  m_WebpageBuilder.AddInputType( "text", "wifi_ssid", "wifi_ssid", m_wifi_ssid, "", true );
  m_WebpageBuilder.AddBreak( 2 );
  m_WebpageBuilder.AddLabel( "wifi_pass", "Password : " );
  m_WebpageBuilder.AddInputType( "password", "wifi_pass", "wifi_pass", "", "", true );
  m_WebpageBuilder.AddBreak( 2 );
  m_WebpageBuilder.AddLabel( "ip", "IP (Leave blank if DHCP assigned) : " );
  m_WebpageBuilder.AddInputType( "text", "ip", "ip", "", String( "xxx.xxx.xxx.xxx" ), false );
  m_WebpageBuilder.AddBreak( 2 );
  m_WebpageBuilder.AddLabel( "subnet", "Subnet (Leave blank if DHCP assigned) : " );
  m_WebpageBuilder.AddInputType( "text", "subnet", "subnet", "", String( "xxx.xxx.xxx.xxx" ), false );
  m_WebpageBuilder.AddBreak( 3 );

  // Submit button
  m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddButton( "submit", "SUBMIT & SAVE" );
  m_WebpageBuilder.EndFormAction();

  // Cancel button
  m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddButtonActionForm( "/", "RETURN TO MAIN MENU" );

  // Reset button
  m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddButtonActionForm( "reset_wifi", "RESET WiFi BACK TO HOTSPOT" );

  m_WebpageBuilder.EndCenter();
  m_WebpageBuilder.EndBody();
  m_WebpageBuilder.EndPage();

  m_WebServer.send( 200, "text/html", m_WebpageBuilder.m_html );
}

void ConfigServer::SendESP32PinsSetupPage() {
  String mac_address = "Device MAC = " + WiFi.macAddress();

  m_WebpageBuilder.StartPage();
  m_WebpageBuilder.AddTitle( "Artnet2DMX Setup Page" );
  m_WebpageBuilder.StartBody();
  m_WebpageBuilder.StartCenter();
  m_WebpageBuilder.AddHeading( "ESP32 Pin Setup" );

  // ESP32 & Art-Net settings
  m_WebpageBuilder.AddFormAction( "/setup_esp32pins", "POST" );
  m_WebpageBuilder.AddLabel( "gpio_enable", "GPIO - Enable : Connnect to DE & RE on MAX485." );
  m_WebpageBuilder.AddBreak( 1 );
  m_WebpageBuilder.AddInputType( "number", "GPIO Enable", "gpio_enable", String( m_gpio_enable ), "", true );
  m_WebpageBuilder.AddBreak( 2 );
  m_WebpageBuilder.AddLabel( "gpio_transmit", "GPIO - Transmit : Connnect to DI on MAX485." );
  m_WebpageBuilder.AddBreak( 1 );
  m_WebpageBuilder.AddInputType( "number", "GPIO Transmit", "gpio_transmit", String( m_gpio_transmit ), "", true );
  m_WebpageBuilder.AddBreak( 2 );
  m_WebpageBuilder.AddLabel( "gpio_receive", "GPIO - Receive : Ensure GPIO is not connected." );
  m_WebpageBuilder.AddBreak( 1 );
  m_WebpageBuilder.AddInputType( "number", "GPIO Receive", "gpio_receive", String( m_gpio_receive ), "", true );

  // Submit button
  m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddButton( "submit", "SUBMIT & SAVE" );
  m_WebpageBuilder.EndFormAction();

  // Cancel button
  m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddButtonActionForm( "/", "RETURN TO MAIN MENU" );

  // Reset button
  m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddButtonActionForm( "reset_esp32pins", "RESET ALL ESP32 PINS TO DEFAULT" );

  m_WebpageBuilder.EndCenter();
  m_WebpageBuilder.EndBody();
  m_WebpageBuilder.EndPage();

  m_WebServer.send( 200, "text/html", m_WebpageBuilder.m_html );
}

void ConfigServer::SendArtnet2DMXSetupPage() {
  m_WebpageBuilder.StartPage();
  m_WebpageBuilder.AddTitle( "Artnet2DMX Setup Page" );
  m_WebpageBuilder.StartBody();
  m_WebpageBuilder.StartCenter();
  m_WebpageBuilder.AddHeading( "Art-Net to DMX Setup" );

  m_WebpageBuilder.AddFormAction( "/setup_artnet2dmx", "POST" );
  m_WebpageBuilder.AddBreak( 2 );
  m_WebpageBuilder.AddLabel( "source ip", "Source IP that will send Art-Net packets. Use 255.255.255.255 if any." );
  m_WebpageBuilder.AddBreak( 1 );
  m_WebpageBuilder.AddInputType( "text", "source ip", "artnet_source_ip", String( m_artnet_source_ip ), "xxx.xxx.xxx.xxx", true );
  m_WebpageBuilder.AddBreak( 2 );
  m_WebpageBuilder.AddLabel( "Art-Net Universe", "Art-Net Universe : The Art-Net universe to translate into DMX. All other universes are ignored." );
  m_WebpageBuilder.AddBreak( 1 );
  m_WebpageBuilder.AddInputType( "number", "Art-Net universe", "artnet_universe", String( m_artnet_universe ), "", true );
  m_WebpageBuilder.AddBreak( 2 );
  m_WebpageBuilder.AddLabel( "Art-Net timeout in ms", "Art-Net timeout in ms.  If no data received after this time then everything is turned off.  Use 0 to disable." );
  m_WebpageBuilder.AddBreak( 1 );
  m_WebpageBuilder.AddInputType( "number", "Art-Net timeout in ms", "artnet_timeout_ms", String( m_artnet_timeout_ms ), "", true );
  m_WebpageBuilder.AddBreak( 2 );
  
  // Submit button
  m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddButton( "submit", "SUBMIT & SAVE" );
  m_WebpageBuilder.EndFormAction();

  // Cancel button
  m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddButtonActionForm( "/", "RETURN TO MAIN MENU" );

  // Reset button
  m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddButtonActionForm( "reset_artnew2dmx", "RESET ALL ART-NET TO DMX SETTINGS TO DEFAULT" );

  m_WebpageBuilder.EndCenter();
  m_WebpageBuilder.EndBody();
  m_WebpageBuilder.EndPage();

  m_WebServer.send( 200, "text/html", m_WebpageBuilder.m_html );
}

void ConfigServer::SendChannelModsSetupPage() {
  m_WebpageBuilder.StartPage();
  m_WebpageBuilder.AddTitle( "Channel Mods Setup Page" );
  m_WebpageBuilder.StartBody();
  m_WebpageBuilder.StartCenter();
  m_WebpageBuilder.AddHeading( "Channel Mods Setup" );
  m_WebpageBuilder.AddBreak( 3 );
  
  m_WebpageBuilder.AddLabel( "copy_artnet_status", "Copy Artnet to DMX" );
  m_WebpageBuilder.AddBreak( 1 );
  if( m_channel_mods_copy_artnet_to_dmx ) {
    m_WebpageBuilder.AddButtonActionFormPost( "copy_artnet_disable", "ENABLED" );
  } else {
    m_WebpageBuilder.AddButtonActionFormPost( "copy_artnet_enable", "DISABLED" );
  }

  m_WebpageBuilder.AddFormAction( "/setup_channelmods", "POST" );
  m_WebpageBuilder.AddBreak( 2 );
  m_WebpageBuilder.AddLabel( "Channel Mods", "Select channel to setup." );
  m_WebpageBuilder.AddBreak( 1 );
  m_WebpageBuilder.AddSelectorNumberList( "channel", "Select channel", 1, 512, 1, 1 );

  // Submit button
  m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddButton( "submit", "ADD NEW" );
  m_WebpageBuilder.EndFormAction();

  m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddFileDownloadLink( "config_mods.json", "Click to download config_mods.json" );

  m_WebpageBuilder.AddBreak( 2 );
  m_WebpageBuilder.AddFileUpload();

  // Cancel button
  m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddButtonActionForm( "/", "RETURN TO MAIN MENU" );

  // Reset button
  m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddButtonActionForm( "/reset_channelmods", "RESET ALL CHANNEL MODS TO DEFAULT" );

  // Edit & Remove buttons for existing channels with mods
  m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddGridStyle( "grid-container", 3 );
  m_WebpageBuilder.AddFormAction( "/channelmods_er", "POST" );
  m_WebpageBuilder.StartDivClass( "grid-container" );

  m_WebpageBuilder.AddGridCellText( "Channel" );
  m_WebpageBuilder.AddGridCellText( "" );
  m_WebpageBuilder.AddGridCellText( "" );

  bool channel_has_mods[ 513 ];
  for(int i = 1; i < 512; ++i) {
    channel_has_mods[ i ] = false;
  }

  for( const ChannelMod& mod : m_ChannelModsHandler.GetModsVector() ) {
    channel_has_mods[ mod.m_channel ] = true;
  }

  for(int i = 1; i < 512; ++i) {
    if( channel_has_mods[ i ] ) {
      m_WebpageBuilder.AddGridCellText( String( i ) );
      m_WebpageBuilder.AddButtonAction( "/mods_editfor/" + String( i ), "Edit" );
      m_WebpageBuilder.AddButtonAction( "/mods_removefor/" + String( i ), "Remove" );
    }
  }

  m_WebpageBuilder.EndFormAction();
  m_WebpageBuilder.EndDiv();
  m_WebpageBuilder.EndCenter();
  m_WebpageBuilder.EndBody();
  m_WebpageBuilder.EndPage();

  m_WebServer.send( 200, "text/html", m_WebpageBuilder.m_html );
}

void ConfigServer::SendChannelModsForChannelSetupPage( int channel_number ) {
  m_WebpageBuilder.StartPage();
  m_WebpageBuilder.AddStandardViewportScale();

  m_WebpageBuilder.AddTitle( "Channel Mods Setup Page" );
  m_WebpageBuilder.StartBody();
  m_WebpageBuilder.StartCenter();
  m_WebpageBuilder.AddHeading( "Channel " + String( channel_number ) + " Mods Setup" );
  m_WebpageBuilder.AddBreak( 3 );
  
  m_WebpageBuilder.AddGridStyle( "grid-container", 3 );
  m_WebpageBuilder.AddFormAction( "/", "POST" );
  m_WebpageBuilder.StartDivClass( "grid-container" );

  m_WebpageBuilder.AddGridCellText( "Modifier" );
  m_WebpageBuilder.AddGridCellText( "Value" );
  m_WebpageBuilder.AddButtonAction( "/mods_addfor/" + String( channel_number ), "Add Mod" );

  // Itr for each mod to find the ones for this channel
  int mod_position = 0;

  for( const ChannelMod& mod : m_ChannelModsHandler.GetModsVector() ) {
    if( mod.m_channel == channel_number ) {
      // Add mod type
      m_WebpageBuilder.m_html += "<select name=\"mod_type_" + String( mod.m_sequence ) + "\" id=\"Mod Type\">";
      for( int mod_type = 0; mod_type <= CHANNELMODTYPE::MAX; mod_type++ )
      {
        m_WebpageBuilder.m_html +=  "<option value=\"" + String( mod_type ) + "\"";
        if( mod.m_mod_type == mod_type ) {
          m_WebpageBuilder.m_html += " selected";
        }
        m_WebpageBuilder.m_html += ">" + String( ModTypeAsString( mod_type ) ) + "</option>";
      }
      m_WebpageBuilder.m_html += "</select>";
      // Add value
      m_WebpageBuilder.AddGridEntryNumberCell( "mod_value_" + String( mod.m_sequence ), mod.m_mod_value, 0, 512, true );
      m_WebpageBuilder.AddButtonAction( "/mods_delfor/" + String( channel_number ) + "/" + String( mod.m_sequence ), "Remove Mod" );
    }
    mod_position++;
  }

  // Gap
  m_WebpageBuilder.AddGridCellText( "" );
  m_WebpageBuilder.AddGridCellText( "" );
  m_WebpageBuilder.AddGridCellText( "" );

  // Submit button
  m_WebpageBuilder.AddGridCellText( "" );
  m_WebpageBuilder.AddButtonAction( "/setup_channelmodsfor/" + String( channel_number ), "SAVE" );
  m_WebpageBuilder.AddGridCellText( "" );

  m_WebpageBuilder.EndFormAction();

  m_WebpageBuilder.EndDiv();

  // Cancel button
  m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddButtonActionForm( "/settings_channelmods", "RETURN TO CHANNEL MODS SETUP" );

  m_WebpageBuilder.EndCenter();
  m_WebpageBuilder.EndBody();
  m_WebpageBuilder.EndPage();

  m_WebServer.send( 200, "text/html", m_WebpageBuilder.m_html );

}

void ConfigServer::SendModConfigFile() {
  String filename = CONFIG_MODS;

  if( LittleFS.exists( filename ) ) {
    String filenameonly = filename;
    int last_slash_position = filename.lastIndexOf( '/' );
    if( last_slash_position != -1 ) {
        filenameonly = filename.substring( last_slash_position + 1 );
    } else {
        filenameonly = filename;
    }
    File file = LittleFS.open( filename, "r" );
    m_WebServer.sendHeader( "Content-Disposition", "attachment; filename=\"" + filenameonly + "\"" );
    m_WebServer.streamFile( file, "application/octet-stream" );
    file.close();

    this->SendChannelModsSetupPage();
  } else {
    m_WebServer.send( 200, "text/plain", "Not found!" );
    delay( 4000 );
    Serial.printf("File not sent.\n");
  }  
}

void ConfigServer::SendStrobeConfigSetupPage() {
  m_WebpageBuilder.StartPage();
  m_WebpageBuilder.AddTitle( "Artnet2DMX Setup Page" );
  m_WebpageBuilder.StartBody();
  m_WebpageBuilder.StartCenter();
  m_WebpageBuilder.AddHeading( "Strobe Config Setup" );

  m_WebpageBuilder.AddBreak( 2 );
  m_WebpageBuilder.AddLabel( "info", "Note: Very limited strobe effect by switching DMX512 channels on & off." );
  m_WebpageBuilder.AddBreak( 1 );
  m_WebpageBuilder.AddLabel( "info2", "Since each DMX512 update is ~23ms, then timings can only be done in ~23ms intervals." );
  m_WebpageBuilder.AddBreak( 1 );
  m_WebpageBuilder.AddLabel( "info3", "Set Channel In to 0 to disable this strobe effect." );
  m_WebpageBuilder.AddBreak( 3 );

  m_WebpageBuilder.AddGridStyle( "grid-container4", 4 );
  m_WebpageBuilder.AddFormAction( "/strobe_config", "POST" );
  m_WebpageBuilder.StartDivClass( "grid-container4" );

  // 
  m_WebpageBuilder.AddGridCellText( "ArtNet Channel In" );
  m_WebpageBuilder.AddGridCellText( "Strobe All DMX Channels?" );
  m_WebpageBuilder.AddGridCellText( "DMX Value Out" );
  m_WebpageBuilder.AddGridCellText( "" );

  m_WebpageBuilder.AddGridEntryNumberCell( "channel", m_strobe_listening_channel, 0, 512, true );
  m_WebpageBuilder.AddSelector2Items( "all","","No", "Yes", !m_strobe_all_channels );
  m_WebpageBuilder.AddGridEntryNumberCell( "value", m_strobe_all_channels_value, 1, 255, true );
  m_WebpageBuilder.AddGridCellText( "" );

  // Gap
  m_WebpageBuilder.AddGridCellText( "" );
  m_WebpageBuilder.AddGridCellText( "" );
  m_WebpageBuilder.AddGridCellText( "" );
  m_WebpageBuilder.AddGridCellText( "" );

  //
  m_WebpageBuilder.AddGridCellText( "Strobe" );
  m_WebpageBuilder.AddGridCellText( "ArtNet Value In" );
  m_WebpageBuilder.AddGridCellText( "Delay" );
  m_WebpageBuilder.AddGridCellText( "Duration" );

  for( int i = 1; i < 5; ++i ) {
    m_WebpageBuilder.AddGridCellText( String( i ) );
    m_WebpageBuilder.AddGridEntryNumberCell( "value_" + String( i ), m_strobe_value[ i ], 1, 255, true );
    m_WebpageBuilder.AddSelectorNumberList( "delay_" + String( i ), "delay", 23, 989, 23, m_strobe_delay[ i ] * 23 );
    m_WebpageBuilder.AddSelectorNumberList( "duration_" + String( i ), "duration", 23, 989, 23, m_strobe_duration[ i ] * 23 );
  }

  // Gap
  m_WebpageBuilder.AddGridCellText( "" );
  m_WebpageBuilder.AddGridCellText( "" );
  m_WebpageBuilder.AddGridCellText( "" );
  m_WebpageBuilder.AddGridCellText( "" );

  // Submit button
  m_WebpageBuilder.AddButtonAction( "/setup_strobeconfig", "SAVE" );
  m_WebpageBuilder.AddGridCellText( "" );
  m_WebpageBuilder.AddGridCellText( "" );
  m_WebpageBuilder.AddGridCellText( "" );

  m_WebpageBuilder.EndFormAction();
  m_WebpageBuilder.EndDiv();

  // Cancel button
  m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddButtonActionForm( "/", "RETURN TO MAIN MENU" );

  // Reset button
  m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddButtonActionForm( "/reset_strobeconfig", "RESET STROBE CONFIG TO DEFAULT" );

  // Info
  m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddLabel( "info4", "Below each channel can be assigned different values on strobe." );
  m_WebpageBuilder.AddBreak( 1 );
  m_WebpageBuilder.AddLabel( "info5", "Note: Click SAVE at the very bottom of the page if you change values." );
  m_WebpageBuilder.AddBreak( 3 );

  // Individual channel data
  m_WebpageBuilder.AddGridStyle( "grid-container2", 2 );
  m_WebpageBuilder.AddFormAction( "/strobe_channels", "POST" );
  m_WebpageBuilder.StartDivClass( "grid-container2" );

  m_WebpageBuilder.AddGridCellText( "DMX Channel Out" );
  m_WebpageBuilder.AddGridCellText( "DMX Value Out" );

  for( int i = 1; i < 513; ++i ) {
    m_WebpageBuilder.AddGridCellText( String( i ) );
    m_WebpageBuilder.AddGridEntryNumberCell( "scv_" + String( i ), m_strobe_buffer_on[ i ], 1, 255, true );
  }
  m_WebpageBuilder.AddButtonAction( "/setup_strobechannels", "SAVE" );
  m_WebpageBuilder.AddGridCellText( "" );

  m_WebpageBuilder.EndFormAction();
  m_WebpageBuilder.EndDiv();

  m_WebpageBuilder.AddBreak( 3 );

  m_WebpageBuilder.EndCenter();
  m_WebpageBuilder.EndBody();
  m_WebpageBuilder.EndPage();

  m_WebServer.send( 200, "text/html", m_WebpageBuilder.m_html );
}

void ConfigServer::Send200Response() {
  m_WebServer.send( 200 );
}

void ConfigServer::HandleResetAll() {
  m_WebServer.send( 200, "text/plain", "Resetting everything to defaults - Reconnect to hotspot to setup WiFi." );
  delay( 4000 );
  this->ResetConfigToDefault();
  this->SettingsSave();
  this->ConnectToWiFi();
  m_changed_network       = true;
  m_changed_dmx_config    = true;
  m_changed_strobe_config = true;
}

void ConfigServer::HandleResetWiFi() {
  m_WebServer.send( 200, "text/plain", "Resetting to WiFi defaults - Reconnect to hotspot to setup WiFi." );
  delay( 4000 );
  this->ResetWiFiToDefault();
  this->SettingsSave();
  this->ConnectToWiFi();
  m_changed_network = true;
}

void ConfigServer::HandleResetESP32Pins() {
  this->ResetESP32PinsToDefault();
  this->SettingsSave();
  this->SendESP32PinsSetupPage();
  m_changed_dmx_config = true;
}

void ConfigServer::HandleResetArtnet2DMX() {
  this->ResetArtnet2DMXToDefault();
  this->SettingsSave();
  this->SendArtnet2DMXSetupPage();
}

void ConfigServer::HandleResetChannelMods() {
  this->ResetChannelModsToDefault();
  this->SettingsSave();
  this->SendChannelModsSetupPage();
}

void ConfigServer::HandleResetStrobeConfig() {
  this->ResetStrobeConfigToDefault();
  this->SettingsSave();
  this->SendStrobeConfigSetupPage();
}

void ConfigServer::HandleDMXEnable() {
  m_dmx_enabled = true;
  this->SettingsSave();
  this->SendSetupMenuPage();
}

void ConfigServer::HandleDMXDisable() {
  m_dmx_enabled = false;
  this->SettingsSave();
  this->SendSetupMenuPage();
}

void ConfigServer::HandleCopyArtnetToDMXEnable() {
  m_channel_mods_copy_artnet_to_dmx = true;
  this->SettingsSave();
  this->SendChannelModsSetupPage();
}

void ConfigServer::HandleCopyArtnetToDMXDisable() {
  m_channel_mods_copy_artnet_to_dmx = false;
  this->SettingsSave();
  this->SendChannelModsSetupPage();
}

void ConfigServer::HandleSetupWiFi() {  
  String wifi_ssid_new   = "";
  String wifi_pass_new   = "";
  String wifi_ip_new     = "";
  String wifi_subnet_new = "";

  for( int i = 0; i < m_WebServer.args(); i++ ) {
    if( m_WebServer.argName( i ) == "wifi_ssid" ) {
      wifi_ssid_new = m_WebServer.arg( i );
    } else if( m_WebServer.argName( i ) == "wifi_pass" ) {
      wifi_pass_new = m_WebServer.arg( i );
    } else if( m_WebServer.argName( i ) == "ip" ) {
      wifi_ip_new = m_WebServer.arg( i );
    } else if( m_WebServer.argName( i ) == "subnet" ) {
      wifi_subnet_new = m_WebServer.arg( i );
    }
  }

  if( wifi_ssid_new.length() > 0 && wifi_pass_new.length() > 0 ) {
    m_wifi_ssid = wifi_ssid_new;
    m_wifi_pass = wifi_pass_new;
    m_wifi_ip   = wifi_ip_new;
    if( m_wifi_ip.length() > 0 && wifi_subnet_new.length() == 0 ) {
      m_wifi_subnet = "255.255.255.0";
    } else {
      m_wifi_subnet = wifi_subnet_new;
    }

    m_WebServer.send( 200, "text/plain", "Attempting to connect to WiFi. On failure hotspot will re-appear." );
     
    if( this->ConnectToWiFi() ) {
      this->SettingsSave();
    }
  }
  m_changed_network = true;
}

void ConfigServer::HandleSetupESP32Pins() {  

  for( int i = 0; i < m_WebServer.args(); i++ ) {
    if( m_WebServer.argName( i ) == "gpio_enable" ) {
      m_gpio_enable = m_WebServer.arg( i ).toInt();
    } else if( m_WebServer.argName( i ) == "gpio_transmit" ) {
      m_gpio_transmit = m_WebServer.arg( i ).toInt();
    } else if( m_WebServer.argName( i ) == "gpio_receive" ) {
      m_gpio_receive = m_WebServer.arg( i ).toInt();
    }
  }

  this->SettingsSave();
  this->SendSetupMenuPage();
  m_changed_dmx_config  = true;
}

void ConfigServer::HandleSetupArtnet2DMX() {  
  for( int i = 0; i < m_WebServer.args(); i++ ) {
    if( m_WebServer.argName( i ) == "artnet_source_ip" ) {
      m_artnet_source_ip = m_WebServer.arg( i );
    } else if( m_WebServer.argName( i ) == "artnet_universe" ) {
      m_artnet_universe = m_WebServer.arg( i ).toInt();
    } else if( m_WebServer.argName( i ) == "artnet_timeout_ms" ) {
      m_artnet_timeout_ms = m_WebServer.arg( i ).toInt();
    }
  }

  this->SettingsSave();
  this->SendSetupMenuPage();
}

void ConfigServer::HandleSetupChannelMods() {  
  for( int i = 0; i < m_WebServer.args(); i++ ) {
    if( m_WebServer.argName( i ) == "channel" ) {
      this->SendChannelModsForChannelSetupPage( m_WebServer.arg( i ).toInt() );
      break;
    }
  }

  this->SendSetupMenuPage();
}

void ConfigServer::HandleSetupChannelModsForChannel() {
  int sequence_number;
  for( int i = 0; i < m_WebServer.args(); i++ ) {
    if( m_WebServer.argName( i ).startsWith( "mod_type_" ) ) {
      sequence_number = m_WebServer.argName( i ).substring( 9 ).toInt();
      m_ChannelModsHandler.UpdateForModType( sequence_number, m_WebServer.arg( i ).toInt() );
    } else if( m_WebServer.argName( i ).startsWith( "mod_value_" ) ) {
      sequence_number = m_WebServer.argName( i ).substring( 10 ).toInt();
      m_ChannelModsHandler.UpdateForModValue( sequence_number, m_WebServer.arg( i ).toInt() );
    }
  }

  this->SettingsSave();
  this->SendChannelModsForChannelSetupPage( m_WebServer.pathArg(0).toInt() );
}

void ConfigServer::HandleChannelModsEditFor() {
  this->SendChannelModsForChannelSetupPage( m_WebServer.pathArg(0).toInt() );
}

void ConfigServer::HandleChannelModsRemoveFor() {
  m_ChannelModsHandler.RemoveAllForChannel( m_WebServer.pathArg(0).toInt() );

  this->SettingsSave();
  this->SendChannelModsSetupPage();
}

void ConfigServer::HandleChannelModsAddFor() {
  unsigned int channel = m_WebServer.pathArg(0).toInt();
  m_ChannelModsHandler.AddMod( channel, CHANNELMODTYPE::NOTHING, 0 );
  this->SettingsSave();
  this->SendChannelModsForChannelSetupPage( channel );
}

void ConfigServer::HandleChannelModsDelFor() {
  unsigned int channel         = m_WebServer.pathArg(0).toInt();
  unsigned int sequence_number = m_WebServer.pathArg(1).toInt();

  m_ChannelModsHandler.RemoveMod( sequence_number );
  this->SettingsSave();
  this->SendChannelModsForChannelSetupPage( channel );
}

void ConfigServer::HandleWebServerDataOnNotFound() {
  // Unhandled
  m_WebServer.send( 200, "text/plain", "Not found!" );
}

void ConfigServer::HandleFileUpload() {
  HTTPUpload& upload = m_WebServer.upload();

  switch( upload.status ) {
    case UPLOAD_FILE_START: {
      String filename = "/" + upload.filename;
      if( !filename.startsWith( "/" ) ) {
        filename = "/" + filename;
      }
      Serial.printf( "File upload : Filename being received = '%s'\n", filename.c_str() );
      m_file_being_uploaded = LittleFS.open( CONFIG_MODS, "w" ); //filename, "w" );
      break;
    }
    case UPLOAD_FILE_WRITE: {
      if( m_file_being_uploaded ) {
        m_file_being_uploaded.write( upload.buf, upload.currentSize );
      }
      break;
    }
    case UPLOAD_FILE_END: {
      if( m_file_being_uploaded ) {
        m_file_being_uploaded.close();
        this->SettingsLoad();
        this->SendChannelModsSetupPage();
      }
      break;
    }
    case UPLOAD_FILE_ABORTED: {
      if( m_file_being_uploaded ) {
        m_file_being_uploaded.close();
        LittleFS.remove( CONFIG_MODS );
        Serial.printf( "File upload : Aborted. Mod config file deleted.\n" );
      }
      break;
    }
    default: {
      Serial.printf( "File upload : Unknown status = %i\n", upload.status );
      break;
    }
  }
}

void ConfigServer::HandleSetupStrobeConfig() {
  bool has_changed = false;
  int strobe_rate;
  int tmp_i;
  for( int i = 0; i < m_WebServer.args(); i++ ) {
    if( m_WebServer.argName( i ) == "channel" ) {
      tmp_i = m_WebServer.arg( i ).toInt();
      if( tmp_i != m_strobe_listening_channel ) {
        m_strobe_listening_channel = tmp_i;
        has_changed = true;
      }
    } else if ( m_WebServer.argName( i ) == "all" ) {
      if( m_WebServer.arg( i ) == "Yes" ) {
        if( !m_strobe_all_channels ) {
          memset( m_strobe_buffer_on, m_strobe_all_channels_value, sizeof( m_strobe_buffer_on ) );
          m_strobe_buffer_on[ 0 ] = 0;
          m_strobe_all_channels = true;
          has_changed = true;
        }
      } else if( m_strobe_all_channels ) {
        memset( m_strobe_buffer_on, 0, sizeof( m_strobe_buffer_on ) );
        m_strobe_all_channels = false;
        has_changed = true;
      }
    } else if ( m_WebServer.argName( i ) == "value" ) {
      tmp_i = m_WebServer.arg( i ).toInt();
      if( tmp_i != m_strobe_all_channels_value ) {
        m_strobe_all_channels_value = tmp_i;
        if( m_strobe_all_channels ) {
          memset( m_strobe_buffer_on, m_strobe_all_channels_value, sizeof( m_strobe_buffer_on ) );
          m_strobe_buffer_on[ 0 ] = 0;
        }
        has_changed = true;
      }
    } else if( m_WebServer.argName( i ).startsWith( "value_" ) ) {
      strobe_rate = m_WebServer.argName( i ).substring( 6 ).toInt();
      tmp_i = m_WebServer.arg( i ).toInt();
      if( m_strobe_value[ strobe_rate ] != tmp_i ) {
        m_strobe_value[ strobe_rate] = tmp_i;
        has_changed = true;
      }
    } else if( m_WebServer.argName( i ).startsWith( "delay_" ) ) {
      strobe_rate = m_WebServer.argName( i ).substring( 6 ).toInt();
      tmp_i = m_WebServer.arg( i ).toInt() / 23;
      if( m_strobe_delay[ strobe_rate ] != tmp_i ) {
        m_strobe_delay[ strobe_rate] = tmp_i;
        has_changed = true;
      }
    } else if( m_WebServer.argName( i ).startsWith( "duration_" ) ) {
      strobe_rate = m_WebServer.argName( i ).substring( 9 ).toInt();
      tmp_i = m_WebServer.arg( i ).toInt() / 23;
      if( m_strobe_duration[ strobe_rate ] != tmp_i ) {
        m_strobe_duration[ strobe_rate] = tmp_i;
        has_changed = true;
      }
    }
  }

  if( has_changed ) {
    this->SettingsSave();
    m_changed_strobe_config = true;
  }

  this->SendStrobeConfigSetupPage();
}

void ConfigServer::HandleSetupStrobeChannels() {
  bool has_changed = false;
  int channel_number;
  int channel_value;
  for( int i = 0; i < m_WebServer.args(); i++ ) {
    if( m_WebServer.argName( i ).startsWith( "scv_" ) ) {
      channel_number = m_WebServer.argName( i ).substring( 4 ).toInt();
      if( channel_number > 0 && channel_number < 513 ) {
        channel_value =  m_WebServer.arg( i ).toInt();      
        if( channel_value < 256 && m_strobe_buffer_on[ channel_number ] != channel_value ) {
           m_strobe_buffer_on[ channel_number ] = channel_value;
           has_changed = true;
        }
      }
    }
  }

  if( has_changed ) {    
    this->SettingsSave();
  }

  this->SendStrobeConfigSetupPage();
}
