#include <vector>
#include "ConfigServer.h"

ConfigServer::ConfigServer() {
  m_settings_changed     = false;
  m_is_connected_to_wifi = false;
}

ConfigServer::~ConfigServer() {
}

void ConfigServer::Init() {

  if( !this->SettingsLoad() ) {
    Serial.println( "Settings failed to load - Resetting to default." );
    this->ResetConfigToDefault();
  }

  m_settings_changed = true;
}

void ConfigServer::ResetConfigToDefault() {
  // Remove existing settings file.
  if( LittleFS.begin( false ) ) {
    LittleFS.remove( CONFIG_FILENAME );
  }

  // Reset all config
  this->ResetWiFiToDefault();
  this->ResetESP32PinsToDefault();
  this->ResetArtnet2DMXToDefault();
  this->ResetChannelModsToDefault();

  // Shows on first page
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

void ConfigServer::ResetChannelModsToDefault() {
  m_channel_mods_vector.clear();
}

void ConfigServer::ResetArtnet2DMXToDefault() {
  m_artnet_source_ip       = "255.255.255.255";  // Any IP source is fine.
  m_artnet_universe        = 1;                  // Universe to listen for, all other universes are ignored.
  m_artnet_timeout_ms      = 3000;               // Artnet timeout
  m_dmx_update_interval_ms = 23;                 // Roughly 4hz
}

void ConfigServer::SettingsSave() {
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
  doc[ "dmx_update_interval_ms" ] = m_dmx_update_interval_ms;
  doc[ "dmx_enabled" ]            = m_dmx_enabled;

  JsonArray array_channelmods = doc.createNestedArray( "channel_mods" );
  for( ChannelMod& mod : m_channel_mods_vector ) {
    JsonObject obj     = array_channelmods.createNestedObject();
    obj[ "sequence" ]  = mod.m_sequence;
    obj[ "channel" ]   = mod.m_channel;
    obj[ "mod_type" ]  = mod.m_mod_type;
    obj[ "mod_value" ] = mod.m_mod_value;
  }

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

  File config_file = LittleFS.open( CONFIG_FILENAME, "w" );
  serializeJson( doc, config_file );
  config_file.close();

  m_settings_changed = true;
}

bool ConfigServer::SettingsLoad() {
  if( !LittleFS.begin( false ) ) {
    // Failed to start LittleFS, probably no save.
    Serial.println( "Failed to start LittleFS" );
    return false;
  }

  DynamicJsonDocument doc( 32768 );
  File config_file = LittleFS.open( CONFIG_FILENAME, "r" );

  if( !config_file ) {
    // File not exist.
    Serial.println( "Failed to load file" );
    return false;
  }

  deserializeJson( doc, config_file );
  config_file.close();

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
  m_dmx_update_interval_ms = doc[ "dmx_update_interval_ms" ];
  m_dmx_enabled            = doc[ "dmx_enabled" ];

  m_channel_mods_vector.clear();
  JsonArray array_channelmods = doc[ "channel_mods" ];
  for( const JsonObject& obj : array_channelmods ) {
    ChannelMod mod;
    mod.m_sequence  = obj[ "sequence" ];
    mod.m_channel   = obj[ "channel" ];
    mod.m_mod_type  = obj[ "mod_type" ];
    mod.m_mod_value = obj[ "mod_value" ];
    m_channel_mods_vector.push_back( mod );
  }

  return true;
}

void ConfigServer::StartWebServer( WebServer* ptr_WebServer ) {
  m_ptr_WebServer = ptr_WebServer;
  m_ptr_WebServer->begin();
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

bool ConfigServer::Update() {

  m_ptr_WebServer->handleClient();

  if( m_settings_changed ) {
    m_settings_changed = false;
    return true;
  }

  return false;
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
  m_WebpageBuilder.AddButtonActionForm( "settings_channelmods", "Channel Mods" );

  m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddLabel( "note", "It's advisable to disable DMX during setup." );
  m_WebpageBuilder.AddBreak( 2 );
  m_WebpageBuilder.AddLabel( "dmx_status", "DMX Status" );
  m_WebpageBuilder.AddBreak( 2 );
  if( m_dmx_enabled ) {
    m_WebpageBuilder.AddButtonActionForm( "dmx_disable", "ENABLED" );
  } else {
    m_WebpageBuilder.AddButtonActionForm( "dmx_enable", "DISABLED" );
  }

  // Reset button
  m_WebpageBuilder.AddBreak( 4 );
  m_WebpageBuilder.AddButtonActionForm( "reset_all", "RESET ALL SETTINGS TO DEFAULT" );

  m_WebpageBuilder.EndCenter();
  m_WebpageBuilder.EndBody();
  m_WebpageBuilder.EndPage();

  m_ptr_WebServer->send( 200, "text/html", m_WebpageBuilder.m_html );
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
  m_WebpageBuilder.AddInputType( "text", "wifi_ssid", "wifi_ssid", "", "", true );
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

  m_ptr_WebServer->send( 200, "text/html", m_WebpageBuilder.m_html );
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

  m_ptr_WebServer->send( 200, "text/html", m_WebpageBuilder.m_html );
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
  m_WebpageBuilder.AddLabel( "DMX update interval in ms", "DMX interval update in milliseconds.  Only change this if you know what you're doing." );
  m_WebpageBuilder.AddBreak( 1 );
  m_WebpageBuilder.AddInputType( "number", "DMX update interval in ms", "dmx_update_ms", String( m_dmx_update_interval_ms ), "", true );

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

  m_ptr_WebServer->send( 200, "text/html", m_WebpageBuilder.m_html );
}

void ConfigServer::SendChannelModsSetupPage() {
  m_WebpageBuilder.StartPage();
  m_WebpageBuilder.AddTitle( "Channel Mods Setup Page" );
  m_WebpageBuilder.StartBody();
  m_WebpageBuilder.StartCenter();
  m_WebpageBuilder.AddHeading( "Channel Mods Setup" );
  m_WebpageBuilder.AddBreak( 3 );
  
  m_WebpageBuilder.AddFormAction( "/setup_channelmods", "POST" );

  m_WebpageBuilder.AddLabel( "Channel Mods", "Select channel to setup." );
  m_WebpageBuilder.AddBreak( 1 );
  m_WebpageBuilder.AddSelectorNumberList( "channel", "Select channel", 1, 512, 1 );

  // Submit button
  m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddButton( "submit", "ADD NEW" );
  m_WebpageBuilder.EndFormAction();

  // Cancel button
  m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddButtonActionForm( "/", "RETURN TO MAIN MENU" );

  // Reset button
  m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddButtonActionForm( "reset_channelmods", "RESET ALL CHANNEL MODS TO DEFAULT" );

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

  for( std::vector<ChannelMod>::iterator ptr_mod = m_channel_mods_vector.begin(); ptr_mod != m_channel_mods_vector.end(); ++ptr_mod ) {
    channel_has_mods[ ptr_mod->m_channel ] = true;
  }

  for(int i = 1; i < 512; ++i) {
    if( channel_has_mods[ i ] ) {
      m_WebpageBuilder.AddGridCellText( String( i ) );
      m_WebpageBuilder.AddButtonAction( "channelmods_edit" + String( i ), "Edit" );
      m_WebpageBuilder.AddButtonAction( "channelmods_remove" + String( i ), "Remove" );
    }
  }

  m_WebpageBuilder.EndFormAction();
  m_WebpageBuilder.EndDiv();
  m_WebpageBuilder.EndCenter();
  m_WebpageBuilder.EndBody();
  m_WebpageBuilder.EndPage();

  m_ptr_WebServer->send( 200, "text/html", m_WebpageBuilder.m_html );
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
  m_WebpageBuilder.AddFormAction( "/setup_channelmodsfor" + String( channel_number), "POST" );
  m_WebpageBuilder.StartDivClass( "grid-container" );

  m_WebpageBuilder.AddGridCellText( "Modifier" );
  m_WebpageBuilder.AddGridCellText( "Value" );
  m_WebpageBuilder.AddButtonAction( "mod_add_" + String( channel_number ), "Add Mod" );

  // Itr for each mod to find the ones for this channel
  int mod_position = 0;
  for( std::vector<ChannelMod>::iterator ptr_mod = m_channel_mods_vector.begin(); ptr_mod != m_channel_mods_vector.end(); ++ptr_mod ) {
    if( ptr_mod->m_channel == channel_number ) {
      // Add mod type
      m_WebpageBuilder.m_html += "<select name=\"mod_type_" + String( ptr_mod->m_sequence ) + "\" id=\"Mod Type\">";
      for( int mod_type = 0; mod_type <= CHANNELMODTYPE::MAX; mod_type++ )
      {
        m_WebpageBuilder.m_html +=  "<option value=\"" + String( mod_type ) + "\"";
        if( ptr_mod->m_mod_type == mod_type ) {
          m_WebpageBuilder.m_html += " selected";
        }
        m_WebpageBuilder.m_html += ">" + String( ModTypeAsString( mod_type ) ) + "</option>";
      }
      m_WebpageBuilder.m_html += "</select>";
      // Add value
      m_WebpageBuilder.AddGridEntryNumberCell( "mod_value_" + String( ptr_mod->m_sequence ), ptr_mod->m_mod_value, 0, 512, true );
      m_WebpageBuilder.AddButtonAction( "mod_del_" + String( ptr_mod->m_sequence ) + "C" + String( channel_number ), "Remove Mod" );
    }
    mod_position++;
  }

  // Gap
  m_WebpageBuilder.AddGridCellText( "" );
  m_WebpageBuilder.AddGridCellText( "" );
  m_WebpageBuilder.AddGridCellText( "" );

  // Submit button
  //m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddGridCellText( "" );
  m_WebpageBuilder.AddButtonAction( "setup_channelmodsfor" + String( channel_number ), "SAVE" );
  m_WebpageBuilder.AddGridCellText( "" );

  m_WebpageBuilder.EndFormAction();

  m_WebpageBuilder.EndDiv();

  // Cancel button
  m_WebpageBuilder.AddBreak( 3 );
  m_WebpageBuilder.AddButtonActionForm( "/settings_channelmods", "RETURN TO CHANNEL MODS SETUP" );

  m_WebpageBuilder.EndCenter();
  m_WebpageBuilder.EndBody();
  m_WebpageBuilder.EndPage();

  m_ptr_WebServer->send( 200, "text/html", m_WebpageBuilder.m_html );

}

void ConfigServer::HandleWebServerData() {
  bool handled = false;
  
  // Allow reset before anything else.
  if( m_ptr_WebServer->uri() == String( "/reset_all" ) ) {
    m_ptr_WebServer->send( 200, "text/plain", "Resetting everything to defaults - Reconnect to hotspot to setup WiFi." );
    delay( 2000 );
    this->ResetConfigToDefault();
    this->SettingsSave();
    this->ConnectToWiFi();
    return;
  } else if( m_ptr_WebServer->uri() == String( "/reset_wifi" ) ) {
    m_ptr_WebServer->send( 200, "text/plain", "Resetting to WiFi defaults - Reconnect to hotspot to setup WiFi." );
    delay( 2000 );
    this->ResetWiFiToDefault();
    this->SettingsSave();
    this->ConnectToWiFi();
    return;
  } else if( m_ptr_WebServer->uri() == String( "/reset_esp32pins" ) ) {
    this->ResetESP32PinsToDefault();
    this->SettingsSave();
    this->SendESP32PinsSetupPage();
    return;
  } else if( m_ptr_WebServer->uri() == String( "/reset_artnew2dmx" ) ) {
    this->ResetArtnet2DMXToDefault();
    this->SettingsSave();
    this->SendArtnet2DMXSetupPage();
    return;
  } else if( m_ptr_WebServer->uri() == String( "/reset_channelmods" ) ) {
    this->ResetChannelModsToDefault();
    this->SettingsSave();
    this->SendChannelModsSetupPage();
    return;
  }

  if( m_ptr_WebServer->method() == HTTP_GET ) {
    // GET
    handled = this->HandleWebGet();
  } else {
    // POST
    handled = this->HandleWebPost();
  }

  if( !handled ) {
    m_ptr_WebServer->send( 200, "text/plain", "Not found!" );
  }
}

bool ConfigServer::HandleWebGet() {
  // Get starts with a /

  if( m_ptr_WebServer->uri() == "/settings_wifi" ) {
    this->SendWiFiSetupPage();
  } else if( m_ptr_WebServer->uri() == "/settings_esp32pins" ) {
    this->SendESP32PinsSetupPage();
  } else if( m_ptr_WebServer->uri() == "/settings_artnet2dmx" ) {
    this->SendArtnet2DMXSetupPage();
  } else if( m_ptr_WebServer->uri() == "/settings_channelmods" ) {
    this->SendChannelModsSetupPage();
  } else if( m_ptr_WebServer->uri() == "/dmx_disable" ) {
    m_dmx_enabled = false;
    this->SettingsSave();
    this->SendSetupMenuPage();
  } else if( m_ptr_WebServer->uri() == "/dmx_enable" ) {
    m_dmx_enabled = true;
    this->SettingsSave();
    this->SendSetupMenuPage();
  } else {
    // Always send setup page.  
    this->SendSetupMenuPage();
  }
  return true;
}

bool ConfigServer::HandleWebPost() {
  
  if( m_ptr_WebServer->uri() == "/setup_wifi" ) {
    if( this->HandleSetupWiFi() ) {
      Serial.printf( "Restarting WiFi\n" );
      this->ConnectToWiFi();
      return true;
    }
  } else if( m_ptr_WebServer->uri() == "/setup_esp32pins" ) {
    this->HandleSetupESP32Pins();
    this->SendSetupMenuPage();
    return true;
  } else if( m_ptr_WebServer->uri() == "/setup_artnet2dmx" ) {
    this->HandleSetupArtnet2DMX();
    this->SendSetupMenuPage();
    return true;
  } else if( m_ptr_WebServer->uri() == "/setup_channelmods" ) {
    this->HandleSetupChannelMods();
    this->SendSetupMenuPage();
    return true;
  } else if( m_ptr_WebServer->uri().startsWith( "/setup_channelmodsfor" ) ) {
    unsigned int channel = m_ptr_WebServer->uri().substring( 21 ).toInt();
    this->HandleSetupChannelModsForChannel( channel );
    this->SendChannelModsForChannelSetupPage( channel );
    return true;
  } else if( m_ptr_WebServer->uri().startsWith( "/mod_add_" ) ) {
    unsigned int channel = m_ptr_WebServer->uri().substring( 9 ).toInt();
    this->ChannelModsAddMod( channel, CHANNELMODTYPE::NOTHING, 0 );
    this->SettingsSave();
    this->SendChannelModsForChannelSetupPage( channel );
    return true;
  } else if( m_ptr_WebServer->uri().startsWith( "/mod_del_" ) ) {
    size_t position_of_channel = m_ptr_WebServer->uri().indexOf( 'C' );
    unsigned int sequence_number = m_ptr_WebServer->uri().substring( 9, position_of_channel ).toInt();
    unsigned int channel = m_ptr_WebServer->uri().substring( position_of_channel + 1 ).toInt();
    this->ChannelModsRemoveMod( sequence_number );
    this->SettingsSave();
    this->SendChannelModsForChannelSetupPage( channel );
    return true;
  } else if ( m_ptr_WebServer->uri().startsWith( "/channelmods_edit" ) ) {
    unsigned int channel = m_ptr_WebServer->uri().substring( 17 ).toInt();
    this->SendChannelModsForChannelSetupPage( channel );
    return true;
  } else if ( m_ptr_WebServer->uri().startsWith( "/channelmods_remove" ) ) {
    unsigned int channel = m_ptr_WebServer->uri().substring( 19 ).toInt();
    this->ChannelModsRemoveAllForChannel( channel );
    this->SettingsSave();
    this->SendChannelModsSetupPage();
    return true;
  } else {
    Serial.printf("Unhandled WebPost = %s\n", m_ptr_WebServer->uri() );
  }
  return false;
}

bool ConfigServer::HandleSetupWiFi() {  
  String wifi_ssid_new   = "";
  String wifi_pass_new   = "";
  String wifi_ip_new     = "";
  String wifi_subnet_new = "";

  for( int i = 0; i < m_ptr_WebServer->args(); i++ ) {
    if( m_ptr_WebServer->argName( i ) == "wifi_ssid" ) {
      wifi_ssid_new = m_ptr_WebServer->arg( i );
    } else if( m_ptr_WebServer->argName( i ) == "wifi_pass" ) {
      wifi_pass_new = m_ptr_WebServer->arg( i );
    } else if( m_ptr_WebServer->argName( i ) == "ip" ) {
      wifi_ip_new = m_ptr_WebServer->arg( i );
    } else if( m_ptr_WebServer->argName( i ) == "subnet" ) {
      wifi_subnet_new = m_ptr_WebServer->arg( i );
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

    m_ptr_WebServer->send( 200, "text/plain", "Attempting to connect to WiFi. On failure hotspot will re-appear." );
     
    if( this->ConnectToWiFi() ) {
      this->SettingsSave();
    }
    return true;
  }
  return false;
}

bool ConfigServer::HandleSetupESP32Pins() {  

  for( int i = 0; i < m_ptr_WebServer->args(); i++ ) {
    if( m_ptr_WebServer->argName( i ) == "gpio_enable" ) {
      m_gpio_enable = m_ptr_WebServer->arg( i ).toInt();
    } else if( m_ptr_WebServer->argName( i ) == "gpio_transmit" ) {
      m_gpio_transmit = m_ptr_WebServer->arg( i ).toInt();
    } else if( m_ptr_WebServer->argName( i ) == "gpio_receive" ) {
      m_gpio_receive = m_ptr_WebServer->arg( i ).toInt();
    }
  }

  this->SettingsSave();

  return true;
}

bool ConfigServer::HandleSetupArtnet2DMX() {  
  for( int i = 0; i < m_ptr_WebServer->args(); i++ ) {
    if( m_ptr_WebServer->argName( i ) == "artnet_source_ip" ) {
      m_artnet_source_ip = m_ptr_WebServer->arg( i );
    } else if( m_ptr_WebServer->argName( i ) == "artnet_universe" ) {
      m_artnet_universe = m_ptr_WebServer->arg( i ).toInt();
    } else if( m_ptr_WebServer->argName( i ) == "dmx_update_ms" ) {
      m_dmx_update_interval_ms = m_ptr_WebServer->arg( i ).toInt();
    } else if( m_ptr_WebServer->argName( i ) == "artnet_timeout_ms" ) {
      m_artnet_timeout_ms = m_ptr_WebServer->arg( i ).toInt();
    }
  }

  this->SettingsSave();

  return true;
}

bool ConfigServer::HandleSetupChannelMods() {  
  for( int i = 0; i < m_ptr_WebServer->args(); i++ ) {
    if( m_ptr_WebServer->argName( i ) == "channel" ) {
      this->SendChannelModsForChannelSetupPage( m_ptr_WebServer->arg( i ).toInt() );
      break;
    }
  }
  
  return true;
}

bool ConfigServer::HandleSetupChannelModsForChannel( int channel_number ) {
  int sequence_number;
  for( int i = 0; i < m_ptr_WebServer->args(); i++ ) {
    if( m_ptr_WebServer->argName( i ).startsWith( "mod_type_" ) ) {
      sequence_number = m_ptr_WebServer->argName( i ).substring( 9 ).toInt();
      this->ChannelModsUpdateForModType( sequence_number, m_ptr_WebServer->arg( i ).toInt() );
    } else if( m_ptr_WebServer->argName( i ).startsWith( "mod_value_" ) ) {
      sequence_number = m_ptr_WebServer->argName( i ).substring( 10 ).toInt();
      this->ChannelModsUpdateForModValue( sequence_number, m_ptr_WebServer->arg( i ).toInt() );
    }
  }

  this->SettingsSave();

  return true;
}

void ConfigServer::ChannelModsAddMod( unsigned int channel_number, unsigned int mod_type, unsigned int mod_value ) {
  ChannelMod mod;
  mod.m_sequence = this->ChannelModsNextSequenceForChannel( channel_number ) + 1;
  mod.m_channel = channel_number;
  mod.m_mod_type = mod_type;
  mod.m_mod_value = mod_value;
  m_channel_mods_vector.push_back( mod );

  this->ChannelModsSortBySequenceAndRenumber();
}

void ConfigServer::ChannelModsRemoveMod( unsigned int sequence_number ) {
  for( auto mods_itr = m_channel_mods_vector.begin(); mods_itr != m_channel_mods_vector.end(); ) {
    if( mods_itr->m_sequence == sequence_number ) {
      m_channel_mods_vector.erase( mods_itr );
      break;
    }
    ++mods_itr;
  }
  this->ChannelModsSortBySequenceAndRenumber();
}

void ConfigServer::ChannelModsSortBySequenceAndRenumber() {

  // Sort
  std::sort( m_channel_mods_vector.begin(), m_channel_mods_vector.end(), ChannelMod::CompareChannelModSequence );

  // Re-Sequence
  unsigned int sequence_new = 10;
  for( auto& mod: m_channel_mods_vector ) {
    mod.m_sequence = sequence_new;
    sequence_new += 10;
  }
}

void ConfigServer::ChannelModsUpdateForModType( unsigned int sequence_number, unsigned int mod_type ) {
  for( auto& mod: m_channel_mods_vector ) {
    if( mod.m_sequence == sequence_number ) {
      mod.m_mod_type = mod_type;
      break;
    }
  }
}

void ConfigServer::ChannelModsUpdateForModValue( unsigned int sequence_number, unsigned int mod_value ) {
  for( auto& mod: m_channel_mods_vector ) {
    if( mod.m_sequence == sequence_number ) {
      mod.m_mod_value = mod_value;
      break;
    }
  }
}

unsigned int ConfigServer::ChannelModsMaxSequence() {
  unsigned int sequence_max = 0;

  for( auto& mod: m_channel_mods_vector ) {
    if( mod.m_sequence > sequence_max ) {
      sequence_max = mod.m_sequence;
    }
  }

  return sequence_max;
}

unsigned int ConfigServer::ChannelModsNextSequenceForChannel( unsigned int channel_number ) {
  unsigned int sequence_max = 0;
  unsigned int sequence_no_other = 1;

  for( auto& mod: m_channel_mods_vector ) {
    if( mod.m_channel == channel_number && mod.m_sequence > sequence_max ) {
      sequence_max = mod.m_sequence;
    } else if( mod.m_channel < channel_number ) {
      sequence_no_other = mod.m_sequence + 1;
    }
  }

  if( sequence_max == 0 ) {
    return sequence_no_other;
  }
  return sequence_max;
}

void ConfigServer::ChannelModsRemoveAllForChannel( unsigned int channel_number ) {
  for( auto mods_itr = m_channel_mods_vector.begin(); mods_itr != m_channel_mods_vector.end(); ) {
    if( mods_itr->m_channel == channel_number ) {
      m_channel_mods_vector.erase( mods_itr );
      break;
    }
    ++mods_itr;
  }
  this->ChannelModsSortBySequenceAndRenumber();
}
