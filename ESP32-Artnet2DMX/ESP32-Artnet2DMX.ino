/*

  Arduino Libraries used
   : esp_dmx (3.0.2-beta) | 4.1.0

  ** Connection points **

  POWER    ESP32 S2 Mini        MAX485          DMX OUT (Female connector)
  GND      GND                  GND             PIN 1
  VBUS     5V                   VCC             Not connected
           GPIO for Enable      DE & RE         Not connected
           GPIO for Transmit    DI              Not connected
           GPIO for Receive     Not connected   Not connected
                                OUTPUT A        PIN 3  (Data +)
                                OUTPUT B        PIN 2  (Data -)

  ** Pin layout for DMX cables **
     IN           OUT
  DMX Male     DMX Female
   1   2         2   1
     3             3
*/

#include <Arduino.h>

#include "ESP32Artnet2DMX.h"

ESP32Artnet2DMX g_Artnet2dmx;

void setup() {
  if( !Serial ) {
    Serial.begin( 115200 );
  }
  delay( 1000 );

  // Init
  g_Artnet2dmx.Init();
  
  // Start
  g_Artnet2dmx.Start();
}

void loop() {
  // Process
  g_Artnet2dmx.Update();
}
