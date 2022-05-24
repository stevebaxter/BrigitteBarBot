/**
   Motor control for Brigitte BarBot
*/

#include <PPMReader.h>

// Range for stick values
const long stickValueRange = 100;

// Initialize a PPMReader on digital pin 2 with 8 expected channels.
#define kInterruptPin 2
#define kChannelCount 8
PPMReader ppm(kInterruptPin, kChannelCount);

#define kSpeedLeftPin 3 // White
#define kEnableLeftPin 4 // Orange
#define kDirectionLeftPin 5 // Purple

#define kSpeedRightPin 6 // White
#define kDirectionRightPin 7 // Purple
#define kEnableRightPin 8 // Orange

long GetStickPositionForChannel(int channel)
{
  // Get the last values from PPM. These centre around 1500 with ~1900 as max and
  // ~1150 as min. We convert these as a value from -100 to +100 for each of the two
  // channels. This is pretty coarse but helps to eliminate noise.
  const long minValue = 1150;
  const long maxValue = 1900;
  const long centreValue = (minValue + maxValue) / 2;
  const long valueRange = maxValue - minValue;

  // Get the raw value
  long value = ppm.latestValidChannelValue(channel, centreValue);

  // Convert to our range
  value = (value - centreValue) * (stickValueRange * 2) / valueRange;

  // If the absolute value is <20 then stop - this gives us a "dead zone)
  if (abs(value) < (stickValueRange / 5))
  {
    value = 0;
  }

  // Ensure the value is constrained to guard against interference or other weirdness
  value = constrain(value, -stickValueRange, stickValueRange);

  return value;
}

void MotorControl(long leftMotor, long rightMotor)
{
  if (leftMotor == 0)
  {
    analogWrite(kSpeedLeftPin, 0);
  }
  else
  {
    digitalWrite(kDirectionLeftPin, leftMotor < 0);
    analogWrite(kSpeedLeftPin, abs(leftMotor));    
  }

  if (rightMotor == 0)
  {
    analogWrite(kSpeedRightPin, 0);
  }
  else
  {
    // Right hand motor is reversed
    digitalWrite(kDirectionRightPin, rightMotor > 0);
    analogWrite(kSpeedRightPin, abs(rightMotor));    
  }
}

void setup()
{
  Serial.begin(9600);

  ppm.channelValueMaxError = 20;
  ppm.blankTime = 5000;
  ppm.minChannelValue = 1100;
  ppm.maxChannelValue = 1900;

  // Set up motor controls
  pinMode(kEnableLeftPin, OUTPUT);
  pinMode(kSpeedLeftPin, OUTPUT);
  analogWrite(kSpeedLeftPin, 0);
  pinMode(kDirectionLeftPin, OUTPUT);
  digitalWrite(kDirectionLeftPin, LOW);
  pinMode(kEnableRightPin, OUTPUT);
  pinMode(kSpeedRightPin, OUTPUT);
  analogWrite(kSpeedRightPin, 0);
  pinMode(kDirectionRightPin, OUTPUT);
  digitalWrite(kDirectionRightPin, LOW);

  digitalWrite(kEnableLeftPin, HIGH);
  digitalWrite(kEnableRightPin, HIGH);

  Serial.println("Starting Brigitte...");

  MotorControl(0, 0);
}

void loop()
{
  // Get the stick values. Channel 1 is left/right, channel 2 is forward/backward,
  // on a scale of -stickValueRange to +stickValueRange.
  long stickX = GetStickPositionForChannel(1);
  long stickY = GetStickPositionForChannel(2);

  long rightMotor = 0;
  long leftMotor = 0;

  // If they are both 0 then all stop
  if ((stickX == 0) && (stickY == 0))
  {
    // Make sure we are stopped
    MotorControl(0, 0);
  }
  else
  {
    // Calculate left and right motor speeds on a scale of -stickValueRange (full reverse) to +stickValueRange (full forward).
    // Adapted from http://home.kendra.com/mauser/joystick.html.
    stickX = -stickX;
    long v = ((stickValueRange - abs(stickX)) * stickY / stickValueRange) + stickY;
    long w = ((stickValueRange - abs(stickY)) * stickX / stickValueRange) + stickX;
    rightMotor = (v + w) / 2;
    leftMotor = (v - w) / 2;
  }

  if ((stickX == stickValueRange) && (stickY == -stickValueRange))
  {
    // This happens if the RC receiver isn't initialised, all stop
    MotorControl(0, 0);
  }
  else
  {
    MotorControl(leftMotor, rightMotor);
  }

  Serial.print("stickX: " + String(stickX) + " ");
  Serial.print("stickY: " + String(stickY) + " ");
  Serial.print("L: " + String(leftMotor) + " ");
  Serial.print("R: " + String(rightMotor) + " ");
  Serial.println();
  
  delay(50);
}
