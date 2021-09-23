#include <FastLED.h>

#define LED_PIN     7
#define NUM_LEDS    128
#define BRIGHTNESS  255    // 255 is brightest
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

#define UPDATES_PER_SECOND 100

CRGB rainbowPalette[16] =
  {
    { 0, 153, 170 },
    { 34, 119, 204 },
    { 68, 85, 238 },
    { 102, 51, 255 },
    { 136, 17, 221 },
    { 170, 0, 187 },
    { 204, 34, 153 },
    { 238, 68, 119 },
    { 255, 102, 85 },
    { 221, 136, 51 },
    { 187, 170, 17 },
    { 153, 204, 0 },
    { 119, 238, 34 },
    { 85, 255, 68 },
    { 51, 221, 102 },
    { 17, 187, 136 }
  };

int rowIndex = 0;
int ledsPerRow = 8;
int numPanels = 16;
int framesPerSecond = 40;

void setup()
{
  delay( 3000 ); // power-up safety delay
    
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
    
  FastLED.setBrightness(  BRIGHTNESS );

  delay( 1000 );
}

void loop()
{
  // Single colored panels moving back and forth
  for ( int inx = 0; inx < 3; inx++ )
  {
    cycleBySinglePanel( CRGB::Red, ledsPerRow, numPanels, 40, true );
    cycleBySinglePanel( CRGB::Red, ledsPerRow, numPanels, 40, false );
    cycleBySinglePanel( CRGB::Yellow, ledsPerRow, numPanels, 40, true );
    cycleBySinglePanel( CRGB::Yellow, ledsPerRow, numPanels, 40, false );
    cycleBySinglePanel( CRGB::Green, ledsPerRow, numPanels, 40, true );
    cycleBySinglePanel( CRGB::Green, ledsPerRow, numPanels, 40, false );
  }

  // Moving "train" of 16 contiguous panels
  for ( int inx = 0; inx < 3; inx++ )
  {
    cycleFillPanel( CRGB::Blue, ledsPerRow, numPanels, 20, true );
    cycleFillPanel( CRGB::Black, ledsPerRow, numPanels, 20, true );
    cycleFillPanel( CRGB::Purple, ledsPerRow, numPanels, 20, true );
    cycleFillPanel( CRGB::Black, ledsPerRow, numPanels, 20, true );
    cycleFillPanel( CRGB::Red, ledsPerRow, numPanels, 20, true );
    cycleFillPanel( CRGB::Black, ledsPerRow, numPanels, 20, true );
  }

  // Moving rainbow colors
  for ( int jnx = 0; jnx < 10; jnx++ )
  {
    for ( int inx = 0; inx < numPanels; inx++ )
    {
      cycleRainbow( inx, ledsPerRow, numPanels );    
    }

    // 20 frames per second
    delay( 1000 / 20 );
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
      lightUpRow( pColor, inx, pLedsPerRow, pNumPanels );
      delay( 1000 / pFramesPerSecond );
      clearRow( inx, pLedsPerRow, pNumPanels );
    }
    else
    {
      lightUpRow( pColor, pNumPanels - 1 - inx, pLedsPerRow, pNumPanels );
      delay( 1000 / pFramesPerSecond );
      clearRow( pNumPanels - 1 - inx, pLedsPerRow, pNumPanels );
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
      lightUpRow( pColor, inx, pLedsPerRow, pNumPanels );
      delay( 1000 / pFramesPerSecond );
    }
    else
    {
      lightUpRow( pColor, pNumPanels - 1 - inx, pLedsPerRow, pNumPanels );
      delay( 1000 / pFramesPerSecond );
    }
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
