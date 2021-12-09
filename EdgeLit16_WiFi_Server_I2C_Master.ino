/*******************************************************************************
 * 
 * File Name:   EdgeLit16_WiFi_Server_I2C_Master.ino
 * 
 * Description: This code is to be uploaded to the NodeMCU component of the
 *              EL1601 Night Light. It is the web server that displays the
 *              HTML for the control interface displayed on the users smart
 *              phone. The program parses the GET request received from the user
 *              representing the command to invoke to control the LEDs. The
 *              parsed command is passed on to the Arduino Uno via I2C. The
 *              Arduino, in turn responds to the command to control the
 *              LEDs using the FastLED library.  
 *              
 *                         Copyright (C) 2021 Christopher Biddle
 *                         
 *              This program is free software: you can redistribute it and/or
 *              modify it under the terms of the GNU General Public License as
 *              published by the Free Software Foundation, either version 3 of
 *              the License, or (at your option) any later version. This
 *              program is distributed in the hope that it will be useful, but
 *              WITHOUT ANY WARRANTY; without even the implied warranty of
 *              MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *              GNU General Public License for more details.
 *
 *              You should have received a copy of the GNU General Public
 *              License along with this program.  If not, see
 *              <https://www.gnu.org/licenses/>.
 *              
 *******************************************************************************
 *                              MODIFICATION LOG
 *                              
 * Date                Modified By                       Description
 * ---------- ------------------------- ----------------------------------------
 * 12/09/2021 Chris Biddle              Initial release
 * 
 ******************************************************************************/

// Import required libraries
#include <ESP8266WiFi.h>
#include "ESPAsyncWebServer.h"

#include <Wire.h>

// mode
#define CYCLE                        1
#define SINGLE_PANEL_BACK_AND_FORTH  2
#define MULTI_PANEL                  3
#define RAINBOW                      4
#define SINGLE_PANEL_PULL_FORWARD    5
#define GRADIENT_PULSE               6

#define BRIGHTNESS_LOW              10
#define BRIGHTNESS_MEDIUM           11
#define BRIGHTNESS_HIGH             12

#define STANDBY_OFF                 15
#define STANDBY_ON                  16

// Set your access point network credentials
const char* ssid = "EL1601 Night Light";
const char* password = "Z1mb@bw3";

// In most cases, the IP address will be 192.168.4.1

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

String mainPage = String( "<html>"
                          "<head>"
                          "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                          "</head>"
                          "<body style=\"font-family:arial,helvetica,sans-serif\">"
                          "<h1 style=\"text-align:center\">EL1601 Night Light Controller</h1>"
                          "<h2 style=\"text-align:center\">Pattern Selection</h2>"
                          "<div style=\"text-align:center;padding-bottom:10px\"><a href=\"http://192.168.4.1/\">Cycle All Patterns</a></div>"
                          "<div style=\"text-align:center;padding-bottom:10px\"><a href=\"http://192.168.4.1/singlepanel\">Shuffle Back and Forth</a></div>"
                          "<div style=\"text-align:center;padding-bottom:10px\"><a href=\"http://192.168.4.1/singlepanelfwd\">One at a Time</a></div>"
                          "<div style=\"text-align:center;padding-bottom:10px\"><a href=\"http://192.168.4.1/multipanel\">Multi-Panel Train</a></div>"
                          "<div style=\"text-align:center;padding-bottom:10px\"><a href=\"http://192.168.4.1/trailgradient\">Fading Trails</a></div>"
                          "<div style=\"text-align:center\"><a href=\"http://192.168.4.1/rainbow\">Rainbow</a></div>"
                          "<h2 style=\"text-align:center\">Brightness</h2>"
                          "<div style=\"text-align:center;padding-bottom:10px\"><a href=\"http://192.168.4.1/br_low\">Low</a></div>"
                          "<div style=\"text-align:center;padding-bottom:10px\"><a href=\"http://192.168.4.1/br_medium\">Medium</a></div>"
                          "<div style=\"text-align:center\"><a href=\"http://192.168.4.1/br_high\">High</a></div>"
                          "<h2 style=\"text-align:center\">Run Mode</h2>"
                          "<div style=\"text-align:center;padding-bottom:10px\"><a href=\"http://192.168.4.1/sby_off\">On</a></div>"
                          "<div style=\"text-align:center\"><a href=\"http://192.168.4.1/sby_on\">Off</a></div>"
                          "</body>"
                          "</html>"
                        );

void setup()
{
  // Serial port for debugging purposes
  Serial.begin( 115200 );
  Serial.println();
  
  // Setting the ESP as an access point
  Serial.print( "Setting AP (Access Point)…" );
  
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP( ssid, password );

  IPAddress IP = WiFi.softAPIP();
  Serial.print( "AP IP address: " );
  Serial.println( IP );

  server.on( "/", HTTP_GET, []( AsyncWebServerRequest *request )
  {
    // Relay mode value via I2C to device address 8
    Wire.beginTransmission( 8 );
    Wire.write( CYCLE );
    Wire.endTransmission();
    
    request->send_P( 200, "text/html", mainPage.c_str());
  });

  server.on( "/singlepanel", HTTP_GET, []( AsyncWebServerRequest *request )
  {
    // Relay mode value via I2C to device address 8
    Wire.beginTransmission( 8 );
    Wire.write( SINGLE_PANEL_BACK_AND_FORTH );
    Wire.endTransmission();
    
    request->send_P( 200, "text/html", mainPage.c_str());
  });

  server.on( "/singlepanelfwd", HTTP_GET, []( AsyncWebServerRequest *request )
  {
    // Relay mode value via I2C to device address 8
    Wire.beginTransmission( 8 );
    Wire.write( SINGLE_PANEL_PULL_FORWARD );
    Wire.endTransmission();
    
    request->send_P( 200, "text/html", mainPage.c_str());
  });

  server.on( "/multipanel", HTTP_GET, []( AsyncWebServerRequest *request )
  {
    // Relay mode value via I2C to device address 8
    Wire.beginTransmission( 8 );
    Wire.write( MULTI_PANEL );
    Wire.endTransmission();
    
    request->send_P( 200, "text/html", mainPage.c_str());
  });

  server.on( "/trailgradient", HTTP_GET, []( AsyncWebServerRequest *request )
  {
    // Relay mode value via I2C to device address 8
    Wire.beginTransmission( 8 );
    Wire.write( GRADIENT_PULSE );
    Wire.endTransmission();
    
    request->send_P( 200, "text/html", mainPage.c_str());
  });
  
  server.on( "/rainbow", HTTP_GET, []( AsyncWebServerRequest *request )
  {
    // Relay mode value via I2C to device address 8
    Wire.beginTransmission( 8 );
    Wire.write( RAINBOW );
    Wire.endTransmission();
    
    request->send_P( 200, "text/html", mainPage.c_str());
  });

  server.on( "/br_low", HTTP_GET, []( AsyncWebServerRequest *request )
  {
    // Relay mode value via I2C to device address 8
    Wire.beginTransmission( 8 );
    Wire.write( BRIGHTNESS_LOW );
    Wire.endTransmission();
    
    request->send_P( 200, "text/html", mainPage.c_str());
  });

  server.on( "/br_medium", HTTP_GET, []( AsyncWebServerRequest *request )
  {
    // Relay mode value via I2C to device address 8
    Wire.beginTransmission( 8 );
    Wire.write( BRIGHTNESS_MEDIUM );
    Wire.endTransmission();
    
    request->send_P( 200, "text/html", mainPage.c_str());
  });

  server.on( "/br_high", HTTP_GET, []( AsyncWebServerRequest *request )
  {
    // Relay mode value via I2C to device address 8
    Wire.beginTransmission( 8 );
    Wire.write( BRIGHTNESS_HIGH );
    Wire.endTransmission();
    
    request->send_P( 200, "text/html", mainPage.c_str());
  });

  server.on( "/sby_off", HTTP_GET, []( AsyncWebServerRequest *request )
  {
    // Relay mode value via I2C to device address 8
    Wire.beginTransmission( 8 );
    Wire.write( STANDBY_OFF );
    Wire.endTransmission();
    
    request->send_P( 200, "text/html", mainPage.c_str());
  });

  server.on( "/sby_on", HTTP_GET, []( AsyncWebServerRequest *request )
  {
    // Relay mode value via I2C to device address 8
    Wire.beginTransmission( 8 );
    Wire.write( STANDBY_ON );
    Wire.endTransmission();
    
    request->send_P( 200, "text/html", mainPage.c_str());
  });

  bool status;

  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  // status = bme.begin(0x76);
  status = true; 
  if ( !status )
  {
    Serial.println( "Could not find a valid BME280 sensor, check wiring!" );
    while (1);
  }
  
  // Start server
  server.begin();

  // Open and I2C connection between this NodeMCU WiFi server
  // and the Arduino slave that drives the LED display
  Wire.begin( D1, D2 );
}
 
void loop(){
  
}
