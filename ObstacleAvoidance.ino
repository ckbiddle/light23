#include <Servo.h>  //servo library
Servo myservo;      // create servo object to control servo

int Echo = A4;
int Trig = A5;

#define ENB 5
#define IN1 7
#define IN2 8
#define IN3 9
#define IN4 11
#define ENA 6
#define SCAN_INCREMENTS 10
#define INFINITY_DIST 1000
#define L_TO_R 0
#define R_TO_L 1

int carSpeed = 200;
int forwardCarSpeed = (int) carSpeed / 2;

int distances[SCAN_INCREMENTS][2];
int currentDistance = INFINITY_DIST;
int maxDistance = 0;
int shortestDistance = 0;
int angleAtShortestDistance = 0;

void forward( int pForwardCarSpeed )
{
  analogWrite( ENA, pForwardCarSpeed );
  analogWrite( ENB, pForwardCarSpeed );
  digitalWrite( IN1, HIGH );
  digitalWrite( IN2, LOW );
  digitalWrite( IN3, LOW );
  digitalWrite( IN4, HIGH );
}

void back()
{
  analogWrite( ENA, carSpeed );
  analogWrite( ENB, carSpeed );
  digitalWrite( IN1, LOW );
  digitalWrite( IN2, HIGH );
  digitalWrite( IN3, HIGH );
  digitalWrite( IN4, LOW );
}

void left()
{
  analogWrite( ENA, carSpeed );
  analogWrite( ENB, carSpeed );
  digitalWrite( IN1, LOW );
  digitalWrite( IN2, HIGH );
  digitalWrite( IN3, LOW );
  digitalWrite( IN4, HIGH );
}

void right()
{
  analogWrite( ENA, carSpeed );
  analogWrite( ENB, carSpeed );
  digitalWrite( IN1, HIGH );
  digitalWrite( IN2, LOW );
  digitalWrite( IN3, HIGH );
  digitalWrite( IN4, LOW );
}

void stop()
{
  digitalWrite( ENA, LOW );
  digitalWrite( ENB, LOW );
}

//Ultrasonic distance measurement Sub function
int getDistance()
{
  digitalWrite( Trig, LOW );
  delayMicroseconds( 2 );
  digitalWrite( Trig, HIGH );
  delayMicroseconds( 10 );
  digitalWrite( Trig, LOW );

  return (int) pulseIn( Echo, HIGH ) / 58;
}

void scanDistances( int pDirection )
{
  int angle = 0;

  if ( pDirection == R_TO_L )
  {
    maxDistance = 0;
  
    for ( int inx = 0; inx < SCAN_INCREMENTS; inx++ )
    {
      distances[inx][0] = angle;
      myservo.write( angle );
      delay( 50 );
      currentDistance = getDistance();

      Serial.print( "currentDistance = " );
      Serial.println( currentDistance );
  
      if ( currentDistance < 0 )
      {
        distances[inx][1] = INFINITY_DIST;
      }
      else
      {
        distances[inx][1] = currentDistance;
      }
  
      if ( distances[inx][1] > maxDistance )
      {
        maxDistance = distances[inx][1];
      }
  
      angle += 20;
    }
  }
  else
  {
    angle = 180;
    maxDistance = 0;
  
    for ( int inx = SCAN_INCREMENTS - 1; inx >= 0; inx-- )
    {
      distances[inx][0] = angle;
      myservo.write( angle );
      delay( 50 );
      currentDistance = getDistance();
  
      Serial.print( "currentDistance = " );
      Serial.println( currentDistance );
      
      if ( currentDistance < 0 )
      {
        distances[inx][1] = INFINITY_DIST;
      }
      else
      {
        distances[inx][1] = currentDistance;
      }
  
      if ( distances[inx][1] > maxDistance )
      {
        maxDistance = distances[inx][1];
      }
  
      angle -= 20;
    }
  }

  shortestDistance = maxDistance;
  
  for ( int inx = 0; inx < SCAN_INCREMENTS; inx++ )
  {
    if ( distances[inx][1] < shortestDistance )
    {
      shortestDistance = distances[inx][1];
      angleAtShortestDistance = distances[inx][0];
    }
  }
}

void setup()
{
  myservo.attach(3);  // attach servo on pin 3 to servo object
  
  Serial.begin(9600);
  
  pinMode( Echo, INPUT );
  pinMode( Trig, OUTPUT );
  pinMode( IN1, OUTPUT );
  pinMode( IN2, OUTPUT );
  pinMode( IN3, OUTPUT );
  pinMode( IN4, OUTPUT );
  pinMode( ENA, OUTPUT );
  pinMode( ENB, OUTPUT );
  stop();

  delay( 1000 );

  forward( forwardCarSpeed );
}

int scanDirection = R_TO_L;

void loop()
{
  scanDistances( scanDirection );

  if ( shortestDistance <= 40 )
  {
    stop();

    if ( angleAtShortestDistance >= 0 && angleAtShortestDistance <= 80 )
    {
      left();
      delay( 400 );  // est. 90 deg turn
    }
    else if ( angleAtShortestDistance >= 100 && angleAtShortestDistance <= 180 )
    {
      right();
      delay( 400 );  // est. 90 deg turn
    }

    forward( forwardCarSpeed );
  }

  if ( scanDirection == L_TO_R )
  {
    scanDirection = R_TO_L;
  }
  else
  {
    scanDirection = L_TO_R;
  }
}
