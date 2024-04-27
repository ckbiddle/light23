/*******************************************************************************
 * 
 * File Name:   EdgeLit16_WiFi.ino
 * 
 * Description: Code for the NodeMCU which drives the LED panels in the
 *              EL1601 Night Light. There are a couple of pre-requisites for
 *              compiling and running this code. You will need:
 *              
 *              1. The ESPAsyncWebServer and Adafruit FastLED libraries. As of
 *                 this writing, these are available from
 *              
 *                 https://github.com/me-no-dev/ESPAsyncWebServer          
 *                 
 *                 and
 *                 
 *                 https://www.arduino.cc/reference/en/libraries/fastled/
 *                 
 *              2. Access to a NodeMCU or compatible board. This can be obtained
 *                 by accessing the boards manager, searching on "NodeMCU", and
 *                 installing the appropriate package for the NodeMCU board.
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
 * 07/28/2022 Chris Biddle              Adapted for NodeMCU.
 * 03/10/2024 Chris Biddle              In setup(), commented out any Serial IO
 *                                      code like Serial.begin(),
 *                                      Serial.println(), etc. Commented out the
 *                                      power up safety delay of 3000ms and the
 *                                      1000ms delay at the very end of setup().
 *                                      Neither are needed. 
 * 04/06/2024 Chris Biddle              Added 1000ms power up safety delay.
 *
 ******************************************************************************/

// Import required libraries
#include "ESPAsyncWebServer.h"

#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>

#define LED_PIN             D8
#define NUM_LEDS            128
#define LED_TYPE            WS2812B
#define COLOR_ORDER         GRB
#define DEFAULT_FRAME_RATE  30
#define HIGH_BRIGHTNESS     200  // 255 is absolute max
#define MEDIUM_BRIGHTNESS   70
#define LOW_BRIGHTNESS      10
#define MAX_RAINBOW_BRIGHTNESS 150
#define DEFAULT_BRIGHTNESS  HIGH_BRIGHTNESS

// Command byte values from I2C master
// Pattern modes must be less than 10
#define CYCLE                        1
#define SINGLE_PANEL_BACK_AND_FORTH  2
#define MULTI_PANEL                  3
#define RAINBOW                      4
#define SINGLE_PANEL_PULL_FORWARD    5
#define GRADIENT_PULSE               6

CRGB leds[NUM_LEDS];

CRGB rainbowPalette[16] =
  {
    { 255,   0,   0 },
    { 255,  66,   0 },
    { 255, 192,   0 },
    { 222, 255,   0 },
    { 126, 255,   0 },
    {  30, 255,   0 },
    {   0, 255,  40 },
    {   0, 255, 162 },
    {   0, 252, 255 },
    {   0, 162, 255 },
    {   0,  66, 255 },
    {  30,   0, 255 },
    {  66,   0, 255 },
    { 180,   0, 255 },
    { 255,   0, 192 },
    { 255,   0,  96 }
  };

CRGB gradientTrailPalette[16];

CRGB longBufferPalette[32];

int gradientArray[16] =
  { 255, 221, 187, 153, 119, 85, 51, 17, 0, 0, 0, 0, 0, 0, 0, 0 };

int rowIndex = 0;
int ledsPerRow = 8;
int numPanels = 16;
int framesPerSecond = DEFAULT_FRAME_RATE;
int patternMode = CYCLE;
byte standbyOff = true;
int currentBrightness = DEFAULT_BRIGHTNESS;
byte currentGradientPulseIteration = 0;

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
  // Serial.begin( 115200 );

  delay( 1000 ); // power-up safety delay

  // Setting the ESP as an access point
  // Serial.print( "Setting AP (Access Point)…" );

  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP( ssid, password );

  IPAddress IP = WiFi.softAPIP();
  // Serial.print( "AP IP address: " );
  // Serial.println( IP );

  server.on( "/", HTTP_GET, []( AsyncWebServerRequest *request )
  {
    if ( currentBrightness == MAX_RAINBOW_BRIGHTNESS )
    {
      currentBrightness = HIGH_BRIGHTNESS;
    }

    patternMode = CYCLE;
    request->send_P( 200, "text/html", mainPage.c_str());
  });

  server.on( "/singlepanel", HTTP_GET, []( AsyncWebServerRequest *request )
  {
    if ( currentBrightness == MAX_RAINBOW_BRIGHTNESS )
    {
      currentBrightness = HIGH_BRIGHTNESS;
    }

    patternMode = SINGLE_PANEL_BACK_AND_FORTH;
    request->send_P( 200, "text/html", mainPage.c_str());
  });

  server.on( "/singlepanelfwd", HTTP_GET, []( AsyncWebServerRequest *request )
  {
    if ( currentBrightness == MAX_RAINBOW_BRIGHTNESS )
    {
      currentBrightness = HIGH_BRIGHTNESS;
    }

    patternMode = SINGLE_PANEL_PULL_FORWARD;
    request->send_P( 200, "text/html", mainPage.c_str());
  });

  server.on( "/multipanel", HTTP_GET, []( AsyncWebServerRequest *request )
  {
    if ( currentBrightness == MAX_RAINBOW_BRIGHTNESS )
    {
      currentBrightness = HIGH_BRIGHTNESS;
    }

    patternMode = MULTI_PANEL;
    request->send_P( 200, "text/html", mainPage.c_str());
  });

  server.on( "/trailgradient", HTTP_GET, []( AsyncWebServerRequest *request )
  {
    if ( currentBrightness == MAX_RAINBOW_BRIGHTNESS )
    {
      currentBrightness = HIGH_BRIGHTNESS;
    }

    patternMode = GRADIENT_PULSE;
    request->send_P( 200, "text/html", mainPage.c_str());
  });
  
  server.on( "/rainbow", HTTP_GET, []( AsyncWebServerRequest *request )
  {
    if ( currentBrightness == HIGH_BRIGHTNESS )
    {
      currentBrightness = MAX_RAINBOW_BRIGHTNESS;
    }
    
    patternMode = RAINBOW;
    request->send_P( 200, "text/html", mainPage.c_str());
  });

  server.on( "/br_low", HTTP_GET, []( AsyncWebServerRequest *request )
  {
    FastLED.setBrightness( LOW_BRIGHTNESS );
    currentBrightness = LOW_BRIGHTNESS;
    request->send_P( 200, "text/html", mainPage.c_str());
  });

  server.on( "/br_medium", HTTP_GET, []( AsyncWebServerRequest *request )
  {
    FastLED.setBrightness( MEDIUM_BRIGHTNESS );
    currentBrightness = MEDIUM_BRIGHTNESS;
    request->send_P( 200, "text/html", mainPage.c_str());
  });

  server.on( "/br_high", HTTP_GET, []( AsyncWebServerRequest *request )
  {
    if ( patternMode == RAINBOW )
    {
      FastLED.setBrightness( MAX_RAINBOW_BRIGHTNESS );
      currentBrightness = MAX_RAINBOW_BRIGHTNESS;
    }
    else
    {
      FastLED.setBrightness( HIGH_BRIGHTNESS );
      currentBrightness = HIGH_BRIGHTNESS;
    }
    
    request->send_P( 200, "text/html", mainPage.c_str());
  });

  server.on( "/sby_off", HTTP_GET, []( AsyncWebServerRequest *request )
  {
    standbyOff = true;
    request->send_P( 200, "text/html", mainPage.c_str());
  });

  server.on( "/sby_on", HTTP_GET, []( AsyncWebServerRequest *request )
  {
    standbyOff = false;
    request->send_P( 200, "text/html", mainPage.c_str());
  });

  // Start server
  server.begin();

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( DEFAULT_BRIGHTNESS );

  // delay( 1000 );
}

void loop()
{
  if ( !standbyOff )
  {
    FastLED.clear();
    FastLED.show();
    delay( 500 );
  }

  if ( standbyOff && patternMode == CYCLE )
  {
    // Single colored panels moving back and forth
    for ( int inx = 0; standbyOff && patternMode == CYCLE && inx < 3; inx++ )
    {
      if ( standbyOff && patternMode == CYCLE )
      {
        cycleBySinglePanel( CRGB::Red, ledsPerRow, numPanels, framesPerSecond, true );
        cycleBySinglePanel( CRGB::Red, ledsPerRow, numPanels, framesPerSecond, false );
      }
      
      if ( standbyOff && patternMode == CYCLE )
      {
        cycleBySinglePanel( CRGB::Yellow, ledsPerRow, numPanels, framesPerSecond, true );
        cycleBySinglePanel( CRGB::Yellow, ledsPerRow, numPanels, framesPerSecond, false );
      }
      
      if ( standbyOff && patternMode == CYCLE )
      {
        cycleBySinglePanel( CRGB::Green, ledsPerRow, numPanels, framesPerSecond, true );
        cycleBySinglePanel( CRGB::Green, ledsPerRow, numPanels, framesPerSecond, false );
      }
      
      if ( standbyOff && patternMode == CYCLE )
      {
        cycleBySinglePanel( CRGB::Blue, ledsPerRow, numPanels, framesPerSecond, true );
        cycleBySinglePanel( CRGB::Blue, ledsPerRow, numPanels, framesPerSecond, false );
      }
    }

    // Single colored panels moving forward only
    for ( int inx = 0; standbyOff && patternMode == CYCLE && inx < 3; inx++ )
    {
      if ( standbyOff && patternMode == CYCLE )
      {
        cycleBySinglePanel( CRGB::Red, ledsPerRow, numPanels, framesPerSecond, true );
        cycleBySinglePanel( CRGB::Red, ledsPerRow, numPanels, framesPerSecond, true );
      }

      if ( standbyOff && patternMode == CYCLE )
      {
        cycleBySinglePanel( CRGB::Yellow, ledsPerRow, numPanels, framesPerSecond, true );
        cycleBySinglePanel( CRGB::Yellow, ledsPerRow, numPanels, framesPerSecond, true );
      }

      if ( standbyOff && patternMode == CYCLE )
      {
        cycleBySinglePanel( CRGB::Green, ledsPerRow, numPanels, framesPerSecond, true );
        cycleBySinglePanel( CRGB::Green, ledsPerRow, numPanels, framesPerSecond, true );
      }

      if ( standbyOff && patternMode == CYCLE )
      {
        cycleBySinglePanel( CRGB::Blue, ledsPerRow, numPanels, framesPerSecond, true );
        cycleBySinglePanel( CRGB::Blue, ledsPerRow, numPanels, framesPerSecond, true );
      }
    }

    // Moving "train" of 16 contiguous panels
    for ( int inx = 0; standbyOff && patternMode == CYCLE && inx < 2; inx++ )
    {
      if ( standbyOff && patternMode == CYCLE )
      {
        cycleFillPanel( CRGB::Red, ledsPerRow, numPanels, framesPerSecond, true );
        cycleFillPanel( CRGB::Black, ledsPerRow, numPanels, framesPerSecond, true );
      }

      if ( standbyOff && patternMode == CYCLE )
      {
        cycleFillPanel( CRGB::Yellow, ledsPerRow, numPanels, framesPerSecond, true );
        cycleFillPanel( CRGB::Black, ledsPerRow, numPanels, framesPerSecond, true );
      }

      if ( standbyOff && patternMode == CYCLE )
      {
        cycleFillPanel( CRGB::Green, ledsPerRow, numPanels, framesPerSecond, true );
        cycleFillPanel( CRGB::Black, ledsPerRow, numPanels, framesPerSecond, true );
      }

      if ( standbyOff && patternMode == CYCLE )
      {
        cycleFillPanel( CRGB::Blue, ledsPerRow, numPanels, framesPerSecond, true );
        cycleFillPanel( CRGB::Black, ledsPerRow, numPanels, framesPerSecond, true );
      }

      if ( standbyOff && patternMode == CYCLE )
      {
        cycleFillPanel( CRGB::Purple, ledsPerRow, numPanels, framesPerSecond, true );
        cycleFillPanel( CRGB::Black, ledsPerRow, numPanels, framesPerSecond, true );
      }
    }

    // Moving rainbow colors
    // We don't want the rainbow pattern to run at HIGH_BRIGHTNESS. The LED matrix
    // runs too hot.
    
    int brightnessBeforeRainbow = currentBrightness;

    if ( currentBrightness == HIGH_BRIGHTNESS )
    {
      currentBrightness = MAX_RAINBOW_BRIGHTNESS;
    }
    
    for ( int jnx = 0; standbyOff && patternMode == CYCLE && jnx < 10; jnx++ )
    {
      for ( int inx = 0; standbyOff && patternMode == CYCLE && inx < numPanels; inx++ )
      {
        cycleRainbow( numPanels - 1 - inx, ledsPerRow, numPanels );   
      }
    }

    // Restore the brightness back to what it was before the rainbow pattern.
    currentBrightness = brightnessBeforeRainbow;

    currentGradientPulseIteration = 0;

    for ( int jnx = 0; standbyOff && patternMode == CYCLE && jnx < 13; jnx++ )
    {
      // Gradient pulse
      // Start from black to red
      if ( currentGradientPulseIteration == 0 )
      {
        for ( int inx = 0; inx < 16; inx++ )
        {
          longBufferPalette[inx] = CRGB::Black;
        }
  
        for ( int inx = 16; inx < 32; inx++ )
        {
          longBufferPalette[inx][0] = gradientArray[inx - 16];
          longBufferPalette[inx][1] = 0;
          longBufferPalette[inx][2] = 0;
        }
  
        for ( int inx = 0; standbyOff && patternMode == CYCLE && inx < numPanels; inx++ )
        {
          cycleBufferPalette( inx, ledsPerRow, numPanels );
        }
      }
      else if ( currentGradientPulseIteration == 1 )
      {
        // Then red to yellow
        for ( int inx = 0; inx < 16; inx++ )
        {
          longBufferPalette[inx][0] = gradientArray[inx];
          longBufferPalette[inx][1] = 0;
          longBufferPalette[inx][2] = 0;
        }
  
        for ( int inx = 16; inx < 32; inx++ )
        {
          longBufferPalette[inx][0] = gradientArray[inx - 16];
          longBufferPalette[inx][1] = gradientArray[inx - 16];
          longBufferPalette[inx][2] = 0;
        }
  
        for ( int inx = 0; standbyOff && patternMode == CYCLE && inx < numPanels; inx++ )
        {
          cycleBufferPalette( inx, ledsPerRow, numPanels );
        }
      }
      else if ( currentGradientPulseIteration == 2 )
      {
        // Then yellow to green
        for ( int inx = 0; inx < 16; inx++ )
        {
          longBufferPalette[inx][0] = gradientArray[inx];
          longBufferPalette[inx][1] = gradientArray[inx];
          longBufferPalette[inx][2] = 0;
        }
  
        for ( int inx = 16; inx < 32; inx++ )
        {
          longBufferPalette[inx][0] = 0;
          longBufferPalette[inx][1] = gradientArray[inx - 16];
          longBufferPalette[inx][2] = 0;
        }
  
        for ( int inx = 0; standbyOff && patternMode == CYCLE && inx < numPanels; inx++ )
        {
          cycleBufferPalette( inx, ledsPerRow, numPanels );
        }
      }
      else if ( currentGradientPulseIteration == 3 )
      {
        // Then green to cyan
        for ( int inx = 0; inx < 16; inx++ )
        {
          longBufferPalette[inx][0] = 0;
          longBufferPalette[inx][1] = gradientArray[inx];
          longBufferPalette[inx][2] = 0;
        }
  
        for ( int inx = 16; inx < 32; inx++ )
        {
          longBufferPalette[inx][0] = 0;
          longBufferPalette[inx][1] = gradientArray[inx - 16];
          longBufferPalette[inx][2] = gradientArray[inx - 16];
        }
  
        for ( int inx = 0; standbyOff && patternMode == CYCLE && inx < numPanels; inx++ )
        {
          cycleBufferPalette( inx, ledsPerRow, numPanels );
        }
      }
      else if ( currentGradientPulseIteration == 4 )
      {
        // Then cyan to blue
        for ( int inx = 0; inx < 16; inx++ )
        {
          longBufferPalette[inx][0] = 0;
          longBufferPalette[inx][1] = gradientArray[inx];
          longBufferPalette[inx][2] = gradientArray[inx];
        }
  
        for ( int inx = 16; inx < 32; inx++ )
        {
          longBufferPalette[inx][0] = 0;
          longBufferPalette[inx][1] = 0;
          longBufferPalette[inx][2] = gradientArray[inx - 16];
        }
  
        for ( int inx = 0; standbyOff && patternMode == CYCLE && inx < numPanels; inx++ )
        {
          cycleBufferPalette( inx, ledsPerRow, numPanels );
        }
      }
      else if ( currentGradientPulseIteration == 5 )
      {
        // Then blue to magenta
        for ( int inx = 0; inx < 16; inx++ )
        {
          longBufferPalette[inx][0] = 0;
          longBufferPalette[inx][1] = 0;
          longBufferPalette[inx][2] = gradientArray[inx];
        }
  
        for ( int inx = 16; inx < 32; inx++ )
        {
          longBufferPalette[inx][0] = gradientArray[inx - 16];
          longBufferPalette[inx][1] = 0;
          longBufferPalette[inx][2] = gradientArray[inx - 16];
        }
  
        for ( int inx = 0; standbyOff && patternMode == CYCLE && inx < numPanels; inx++ )
        {
          cycleBufferPalette( inx, ledsPerRow, numPanels );
        }
      }
      else if ( currentGradientPulseIteration == 6 )
      {
        // Then magenta to red
        for ( int inx = 0; inx < 16; inx++ )
        {
          longBufferPalette[inx][0] = gradientArray[inx];
          longBufferPalette[inx][1] = 0;
          longBufferPalette[inx][2] = gradientArray[inx];
        }
  
        for ( int inx = 16; inx < 32; inx++ )
        {
          longBufferPalette[inx][0] = gradientArray[inx - 16];
          longBufferPalette[inx][1] = 0;
          longBufferPalette[inx][2] = 0;
        }
  
        for ( int inx = 0; standbyOff && patternMode == CYCLE && inx < numPanels; inx++ )
        {
          cycleBufferPalette( inx, ledsPerRow, numPanels );
        }
      }
  
      currentGradientPulseIteration++;
  
      if ( currentGradientPulseIteration == 7 )
      {
        // Don't want to go back to 0 because that
        // was just the initializer with a starting block of black.
        currentGradientPulseIteration = 1;
      }
    }
  }
  else if ( standbyOff && patternMode == SINGLE_PANEL_BACK_AND_FORTH )
  {
    if ( standbyOff && patternMode == SINGLE_PANEL_BACK_AND_FORTH )
    {
      cycleBySinglePanel( CRGB::Red, ledsPerRow, numPanels, framesPerSecond, true );
      cycleBySinglePanel( CRGB::Red, ledsPerRow, numPanels, framesPerSecond, false );
    }

    if ( standbyOff && patternMode == SINGLE_PANEL_BACK_AND_FORTH )
    {
      cycleBySinglePanel( CRGB::Yellow, ledsPerRow, numPanels, framesPerSecond, true );
      cycleBySinglePanel( CRGB::Yellow, ledsPerRow, numPanels, framesPerSecond, false );
    }

    if ( standbyOff && patternMode == SINGLE_PANEL_BACK_AND_FORTH )
    {
      cycleBySinglePanel( CRGB::Green, ledsPerRow, numPanels, framesPerSecond, true );
      cycleBySinglePanel( CRGB::Green, ledsPerRow, numPanels, framesPerSecond, false );
    }

    if ( standbyOff && patternMode == SINGLE_PANEL_BACK_AND_FORTH )
    {
      cycleBySinglePanel( CRGB::Blue, ledsPerRow, numPanels, framesPerSecond, true );
      cycleBySinglePanel( CRGB::Blue, ledsPerRow, numPanels, framesPerSecond, false );
    }
  }
  else if ( standbyOff && patternMode == SINGLE_PANEL_PULL_FORWARD )
  {
    if ( standbyOff && patternMode == SINGLE_PANEL_PULL_FORWARD )
    {
      cycleBySinglePanel( CRGB::Red, ledsPerRow, numPanels, framesPerSecond, true );
      cycleBySinglePanel( CRGB::Red, ledsPerRow, numPanels, framesPerSecond, true );
    }

    if ( standbyOff && patternMode == SINGLE_PANEL_PULL_FORWARD )
    {
      cycleBySinglePanel( CRGB::Yellow, ledsPerRow, numPanels, framesPerSecond, true );
      cycleBySinglePanel( CRGB::Yellow, ledsPerRow, numPanels, framesPerSecond, true );
    }

    if ( standbyOff && patternMode == SINGLE_PANEL_PULL_FORWARD )
    {
      cycleBySinglePanel( CRGB::Green, ledsPerRow, numPanels, framesPerSecond, true );
      cycleBySinglePanel( CRGB::Green, ledsPerRow, numPanels, framesPerSecond, true );
    }

    if ( standbyOff && patternMode == SINGLE_PANEL_PULL_FORWARD )
    {
      cycleBySinglePanel( CRGB::Blue, ledsPerRow, numPanels, framesPerSecond, true );
      cycleBySinglePanel( CRGB::Blue, ledsPerRow, numPanels, framesPerSecond, true );
    }
  }
  else if ( standbyOff && patternMode == MULTI_PANEL )
  {
    if ( standbyOff && patternMode == MULTI_PANEL )
    {
      cycleFillPanel( CRGB::Red, ledsPerRow, numPanels, framesPerSecond, true );
      cycleFillPanel( CRGB::Black, ledsPerRow, numPanels, framesPerSecond, true );
    }

    if ( standbyOff && patternMode == MULTI_PANEL )
    {
      cycleFillPanel( CRGB::Yellow, ledsPerRow, numPanels, framesPerSecond, true );
      cycleFillPanel( CRGB::Black, ledsPerRow, numPanels, framesPerSecond, true );
    }

    if ( standbyOff && patternMode == MULTI_PANEL )
    {
      cycleFillPanel( CRGB::Green, ledsPerRow, numPanels, framesPerSecond, true );
      cycleFillPanel( CRGB::Black, ledsPerRow, numPanels, framesPerSecond, true );
    }
    
    if ( standbyOff && patternMode == MULTI_PANEL )
    {
      cycleFillPanel( CRGB::Blue, ledsPerRow, numPanels, framesPerSecond, true );
      cycleFillPanel( CRGB::Black, ledsPerRow, numPanels, framesPerSecond, true );
    }

    if ( standbyOff && patternMode == MULTI_PANEL )
    {
      cycleFillPanel( CRGB::Purple, ledsPerRow, numPanels, framesPerSecond, true );
      cycleFillPanel( CRGB::Black, ledsPerRow, numPanels, framesPerSecond, true );
    }
  }
  else if ( standbyOff && patternMode == RAINBOW )
  {
    for ( int inx = 0; standbyOff && patternMode == RAINBOW && inx < numPanels; inx++ )
    {
      cycleRainbow( numPanels - 1 - inx, ledsPerRow, numPanels );    
    }
  }
  if ( standbyOff && patternMode == GRADIENT_PULSE )
  {
    // Cycle colors

    // Start from black to red
    if ( currentGradientPulseIteration == 0 )
    {
      for ( int inx = 0; inx < 16; inx++ )
      {
        longBufferPalette[inx] = CRGB::Black;
      }

      for ( int inx = 16; inx < 32; inx++ )
      {
        longBufferPalette[inx][0] = gradientArray[inx - 16];
        longBufferPalette[inx][1] = 0;
        longBufferPalette[inx][2] = 0;
      }

      for ( int inx = 0; standbyOff && patternMode == GRADIENT_PULSE && inx < numPanels; inx++ )
      {
        cycleBufferPalette( inx, ledsPerRow, numPanels );
      }
    }
    else if ( currentGradientPulseIteration == 1 )
    {
      // Then red to yellow
      for ( int inx = 0; inx < 16; inx++ )
      {
        longBufferPalette[inx][0] = gradientArray[inx];
        longBufferPalette[inx][1] = 0;
        longBufferPalette[inx][2] = 0;
      }

      for ( int inx = 16; inx < 32; inx++ )
      {
        longBufferPalette[inx][0] = gradientArray[inx - 16];
        longBufferPalette[inx][1] = gradientArray[inx - 16];
        longBufferPalette[inx][2] = 0;
      }

      for ( int inx = 0; standbyOff && patternMode == GRADIENT_PULSE && inx < numPanels; inx++ )
      {
        cycleBufferPalette( inx, ledsPerRow, numPanels );
      }
    }
    else if ( currentGradientPulseIteration == 2 )
    {
      // Then yellow to green
      for ( int inx = 0; inx < 16; inx++ )
      {
        longBufferPalette[inx][0] = gradientArray[inx];
        longBufferPalette[inx][1] = gradientArray[inx];
        longBufferPalette[inx][2] = 0;
      }

      for ( int inx = 16; inx < 32; inx++ )
      {
        longBufferPalette[inx][0] = 0;
        longBufferPalette[inx][1] = gradientArray[inx - 16];
        longBufferPalette[inx][2] = 0;
      }

      for ( int inx = 0; standbyOff && patternMode == GRADIENT_PULSE && inx < numPanels; inx++ )
      {
        cycleBufferPalette( inx, ledsPerRow, numPanels );
      }
    }
    else if ( currentGradientPulseIteration == 3 )
    {
      // Then green to cyan
      for ( int inx = 0; inx < 16; inx++ )
      {
        longBufferPalette[inx][0] = 0;
        longBufferPalette[inx][1] = gradientArray[inx];
        longBufferPalette[inx][2] = 0;
      }

      for ( int inx = 16; inx < 32; inx++ )
      {
        longBufferPalette[inx][0] = 0;
        longBufferPalette[inx][1] = gradientArray[inx - 16];
        longBufferPalette[inx][2] = gradientArray[inx - 16];
      }

      for ( int inx = 0; standbyOff && patternMode == GRADIENT_PULSE && inx < numPanels; inx++ )
      {
        cycleBufferPalette( inx, ledsPerRow, numPanels );
      }
    }
    else if ( currentGradientPulseIteration == 4 )
    {
      // Then cyan to blue
      for ( int inx = 0; inx < 16; inx++ )
      {
        longBufferPalette[inx][0] = 0;
        longBufferPalette[inx][1] = gradientArray[inx];
        longBufferPalette[inx][2] = gradientArray[inx];
      }

      for ( int inx = 16; inx < 32; inx++ )
      {
        longBufferPalette[inx][0] = 0;
        longBufferPalette[inx][1] = 0;
        longBufferPalette[inx][2] = gradientArray[inx - 16];
      }

      for ( int inx = 0; standbyOff && patternMode == GRADIENT_PULSE && inx < numPanels; inx++ )
      {
        cycleBufferPalette( inx, ledsPerRow, numPanels );
      }
    }
    else if ( currentGradientPulseIteration == 5 )
    {
      // Then blue to magenta
      for ( int inx = 0; inx < 16; inx++ )
      {
        longBufferPalette[inx][0] = 0;
        longBufferPalette[inx][1] = 0;
        longBufferPalette[inx][2] = gradientArray[inx];
      }

      for ( int inx = 16; inx < 32; inx++ )
      {
        longBufferPalette[inx][0] = gradientArray[inx - 16];
        longBufferPalette[inx][1] = 0;
        longBufferPalette[inx][2] = gradientArray[inx - 16];
      }

      for ( int inx = 0; standbyOff && patternMode == GRADIENT_PULSE && inx < numPanels; inx++ )
      {
        cycleBufferPalette( inx, ledsPerRow, numPanels );
      }
    }
    else if ( currentGradientPulseIteration == 6 )
    {
      // Then magenta to red
      for ( int inx = 0; inx < 16; inx++ )
      {
        longBufferPalette[inx][0] = gradientArray[inx];
        longBufferPalette[inx][1] = 0;
        longBufferPalette[inx][2] = gradientArray[inx];
      }

      for ( int inx = 16; inx < 32; inx++ )
      {
        longBufferPalette[inx][0] = gradientArray[inx - 16];
        longBufferPalette[inx][1] = 0;
        longBufferPalette[inx][2] = 0;
      }

      for ( int inx = 0; standbyOff && patternMode == GRADIENT_PULSE && inx < numPanels; inx++ )
      {
        cycleBufferPalette( inx, ledsPerRow, numPanels );
      }
    }

    currentGradientPulseIteration++;

    if ( currentGradientPulseIteration == 7 )
    {
      // Don't want to go back to 0 because that
      // was just the initializer with a starting block of black.
      currentGradientPulseIteration = 1;
    }
  }
}

void switchRainbowToHigh()
{
  if ( currentBrightness == MAX_RAINBOW_BRIGHTNESS )
  {
    currentBrightness = HIGH_BRIGHTNESS;
  }

  
}

void cycleBufferPalette( int pStartRowIndex,  // Must be 0 to 16
                         int pLedsPerRow,
                         int pNumPanels
                       )
{
  for ( int inx = 0; inx < pNumPanels; inx++ )
  {
    delay( 1 );  // Adapt for NodeMCU
    lightUpRow( longBufferPalette[inx + pStartRowIndex], inx, pLedsPerRow, pNumPanels );
  }
}

void cycleBySinglePanel( CRGB pColor,
                         int  pLedsPerRow,
                         int  pNumPanels,
                         int  pFramesPerSecond,
                         boolean pForward
                       )
{
  for ( int inx = 0; inx < pNumPanels; inx++ )
  {
    delay( 1 );  // Adapt for NodeMCU
    
    if ( pForward )
    {
      lightUpRow( pColor, pNumPanels - 1 - inx, pLedsPerRow, pNumPanels );
      delay( 1000 / pFramesPerSecond );
      clearRow( pNumPanels - 1 - inx, pLedsPerRow, pNumPanels );
    }
    else
    {
      lightUpRow( pColor, inx, pLedsPerRow, pNumPanels );
      delay( 1000 / pFramesPerSecond );
      clearRow( inx, pLedsPerRow, pNumPanels );
    }
  }
}

void cycleFillPanel( CRGB pColor,
                     int  pLedsPerRow,
                     int  pNumPanels,
                     int  pFramesPerSecond,
                     boolean pForward
                   )
{
  for ( int inx = 0; inx < pNumPanels; inx++ )
  {
    if ( pForward )
    {
      lightUpRow( pColor, pNumPanels - 1 - inx, pLedsPerRow, pNumPanels );
      delay( 1000 / pFramesPerSecond );
    }
    else
    {
      lightUpRow( pColor, inx, pLedsPerRow, pNumPanels );
      delay( 1000 / pFramesPerSecond );
    }
  }
}

void cycleGradientTrail( int pStartRowIndex,
                         int pLedsPerRow,
                         int pNumPanels
                       )
{
  int currentPanelIndex = pStartRowIndex;

  for ( int inx = 0; inx < pNumPanels; inx++ )
  {
    currentPanelIndex = pStartRowIndex + inx;

    if ( currentPanelIndex >= pNumPanels )
    {
      currentPanelIndex = currentPanelIndex - pNumPanels;
    }
    
    lightUpRow( gradientTrailPalette[inx], currentPanelIndex, pLedsPerRow, pNumPanels );
  }
}

void cycleRainbow( int pStartRowIndex,
                   int pLedsPerRow,
                   int pNumPanels
                 )
{
  int currentPanelIndex = pStartRowIndex;
  
  for ( int inx = 0; inx < pNumPanels; inx++ )
  {
    currentPanelIndex = pStartRowIndex + inx;
    
    if ( currentPanelIndex >= pNumPanels )
    {
      currentPanelIndex = currentPanelIndex - pNumPanels;            
    }

    delay( 1 );    // Adapt for NodeMCU

    lightUpRow( rainbowPalette[inx], currentPanelIndex, pLedsPerRow, pNumPanels );
  }
}

void lightUpRow( CRGB pColor,
                 int  pRowIndex,
                 int  pLedsPerRow,
                 int  pNumPanels
               )
{
  int ledIndex = ( pNumPanels - 1 - pRowIndex ) * pLedsPerRow;
  
  for ( int inx = ledIndex; inx < ledIndex + ledsPerRow; inx++ )
  {
    leds[inx] = pColor;
  }

  FastLED.show();
}

void clearRow( int pRowIndex,
               int pLedsPerRow,
               int pNumPanels
             )
{
  int ledIndex = ( pNumPanels - 1 - pRowIndex ) * pLedsPerRow;
  
  for ( int inx = ledIndex; inx < ledIndex + ledsPerRow; inx++ )
  {
    leds[inx] = CRGB::Black;
  }

  FastLED.show();
}
