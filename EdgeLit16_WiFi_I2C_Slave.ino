/*******************************************************************************
 * 
 * File Name:   EdgeLit16_WiFi_I2C_Slave.ino
 * 
 * Description: This code is to be uploaded to the Arduino Uno component of the
 *              EL1601 Night Light. It uses the FastLED library to control the
 *              matrix of LEDs in the EL1601. It receives user commands via I2C
 *              from the NodeMCU component of the EL1601.
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
 
#include <FastLED.h>
#include <Wire.h>

#define LED_PIN             7
#define NUM_LEDS            128
#define LED_TYPE            WS2812B
#define COLOR_ORDER         GRB
#define DEFAULT_FRAME_RATE  40
#define DEFAULT_BRIGHTNESS  255    // 255 is brightest

// Command byte values from I2C master
// Pattern modes must be less than 10
#define CYCLE                        1
#define SINGLE_PANEL_BACK_AND_FORTH  2
#define MULTI_PANEL                  3
#define RAINBOW                      4
#define SINGLE_PANEL_PULL_FORWARD    5
#define GRADIENT_PULSE               6

#define BRIGHTNESS_LOW              10   // actual brightness  10
#define BRIGHTNESS_MEDIUM           11   // actual brightness  70
#define BRIGHTNESS_HIGH             12   // actual brightness 255

#define STANDBY_OFF                 15
#define STANDBY_ON                  16

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

/*
CRGB longBufferPalette[32] =
  {
    { 255,   0, 0 },
    { 255,   0, 0 },
    { 255,   0, 0 },
    { 255,   0, 0 },
    { 255,   0, 0 },
    { 255,   0, 0 },
    { 255,   0, 0 },
    { 255,   0, 0 },
    { 255,   0, 0 },
    { 255,   0, 0 },
    { 255,   0, 0 },
    { 255,   0, 0 },
    { 255,   0, 0 },
    { 255,   0, 0 },
    { 255,   0, 0 },
    { 255,   0, 0 },
    
    {   0, 255, 0 },
    {   0, 255, 0 },
    {   0, 255, 0 },
    {   0, 255, 0 },
    {   0, 255, 0 },
    {   0, 255, 0 },
    {   0, 255, 0 },
    {   0, 255, 0 },
    {   0, 255, 0 },
    {   0, 255, 0 },
    {   0, 255, 0 },
    {   0, 255, 0 },
    {   0, 255, 0 },
    {   0, 255, 0 },
    {   0, 255, 0 },
    {   0, 255, 0 }
  };
*/

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

void setup()
{
  Serial.begin( 115200 );
  
  Wire.begin( 8 );   // Join I2C bus with address 8
  Wire.onReceive( receiveEvent );
    
  delay( 3000 ); // power-up safety delay

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
    
  FastLED.setBrightness( DEFAULT_BRIGHTNESS );

  delay( 1000 );

  // cycleBufferPalette( 8, ledsPerRow, numPanels );
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
    for ( int jnx = 0; standbyOff && patternMode == CYCLE && jnx < 10; jnx++ )
    {
      for ( int inx = 0; standbyOff && patternMode == CYCLE && inx < numPanels; inx++ )
      {
        cycleRainbow( numPanels - 1 - inx, ledsPerRow, numPanels );   
      }
    }

    currentGradientPulseIteration = 0;

    for ( int jnx = 0; standbyOff && patternMode == CYCLE && jnx < 13; jnx++ )
    {
      Serial.print( "jnx = " );
      Serial.print( jnx );
      Serial.print( " : currentGradientPulseIteration = " );
      Serial.println( currentGradientPulseIteration );
       
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

    /*
    // Gradient pulse
    for ( int jnx = 0; standbyOff && patternMode == CYCLE && jnx < 10; jnx++ )
    {
      // White
      for ( int inx = 0; inx < 16; inx++ )
      {
        gradientTrailPalette[inx][0] = gradientArray[inx];
        gradientTrailPalette[inx][1] = gradientArray[inx];
        gradientTrailPalette[inx][2] = gradientArray[inx];
      }

      for ( int inx = 0; standbyOff && patternMode == CYCLE && inx < numPanels; inx++ )
      {
        cycleGradientTrail( numPanels - 1 - inx, ledsPerRow, numPanels );
      }
    }
    */
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

    /*
    // White
    for ( int inx = 0; inx < 16; inx++ )
    {
      gradientTrailPalette[inx][0] = gradientArray[inx];
      gradientTrailPalette[inx][1] = gradientArray[inx];
      gradientTrailPalette[inx][2] = gradientArray[inx];
    }

    for ( int inx = 0; standbyOff && patternMode == GRADIENT_PULSE && inx < numPanels; inx++ )
    {
       cycleGradientTrail( numPanels - 1 - inx, ledsPerRow, numPanels );
    }
    */
  }
}

void cycleBufferPalette( int pStartRowIndex,  // Must be 0 to 16
                         int pLedsPerRow,
                         int pNumPanels
                       )
{
  for ( int inx = 0; inx < pNumPanels; inx++ )
  {
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

void receiveEvent( int howMany )
{
  bool firstByte = true;
  byte junk = 0;
  byte inputVal = 0;
  
  while( Wire.available() > 0 )
  {
    if ( firstByte )
    {
      inputVal = Wire.read();

      if ( inputVal < 10 )  // if it is a pattern mode
      {
        patternMode = inputVal;
      }
      else if ( inputVal == BRIGHTNESS_LOW )
      {
        FastLED.setBrightness( 10 );
        currentBrightness = 10;
      }
      else if ( inputVal == BRIGHTNESS_MEDIUM )
      {
        FastLED.setBrightness( 70 );
        currentBrightness = 70;
      }
      else if ( inputVal == BRIGHTNESS_HIGH )
      {
        FastLED.setBrightness( 255 );
        currentBrightness = 255;
      }
      else if ( inputVal == STANDBY_OFF )
      {
        standbyOff = true;
      }
      else if ( inputVal == STANDBY_ON )
      {
        standbyOff = false;
      }
      
      firstByte = false;
    }
    else
    {
      junk = Wire.read();
    }
  }

  Serial.print( "patternMode = [" );
  Serial.print( patternMode );
  Serial.println( "]" );
}
