/**
   Motor control for Brigitte BarBot
*/

#include <PPMReader.h>

// Range for stick values
long stickValueRange = 255;

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

#define kBeerFlapAPin 12
#define kBeerFlapBPin 13

// We filter values from the PPM controller because the DC flap motor causes lots of noise
long channelFilter[kChannelCount][3];
int channelFilterIndex[kChannelCount];

long FilterChannel(int channel, long value)
{
  long outputValue = value;

  // Make channel zero-based
  channel--;

  int index = channelFilterIndex[channel];

  channelFilter[channel][index] = value;

  long value0 = channelFilter[channel][0];
  long value1 = channelFilter[channel][1];
  long value2 = channelFilter[channel][2];

  long average = (value0 + value1 + value2) / 3;

  // Of the values in the filter, reject the outlier
  long diff0 = abs(value0 - average);
  long diff1 = abs(value1 - average);
  long diff2 = abs(value2 - average);

  if ((diff0 > diff1) && (diff0 > diff2))
  {
    // Entry 0 is the outlier
    outputValue = (channelFilter[channel][1] + channelFilter[channel][2]) / 2;
  }
  else if ((diff1 > diff0) && (diff1 > diff2))
  {
    // Entry 1 is the outlier
    outputValue = (channelFilter[channel][0] + channelFilter[channel][2]) / 2;
  }
  else if ((diff2 > diff0) && (diff2 > diff1))
  {
    // Entry 2 is the outlier
    outputValue = (channelFilter[channel][0] + channelFilter[channel][1]) / 2;
  }

  index++;
  index %= 3;
  channelFilterIndex[channel] = index;

  return outputValue;
}

long GetValueForChannel(int channel, long range)
{
  // Get the last values from PPM. These centre around 1500 with ~1900 as max and
  // ~1150 as min. We convert these as a value from -100 to +100 for each of the two
  // channels. This is pretty coarse but helps to eliminate noise.
  const long minValue = 1150;
  const long maxValue = 1900;
  const long centreValue = (minValue + maxValue) / 2;
  const long valueRange = maxValue - minValue;

  long value = ppm.latestValidChannelValue(channel, centreValue);
  value = FilterChannel(channel, value);

  // Convert to our range
  value = (value - centreValue) * (range * 2) / valueRange;

  // Ensure the value is constrained to guard against interference or other weirdness
  value = constrain(value, -range, range);

  return value;
}

long GetStickPositionForChannel(int channel)
{
  long value = GetValueForChannel(channel, stickValueRange);

  // If the absolute value is <20 then stop - this gives us a "dead zone)
  if (abs(value) < (stickValueRange / 5))
  {
    value = 0;
  }

  return value;
}

void MotorControl(long leftMotor, long rightMotor)
{
  if (leftMotor == 0)
  {
    digitalWrite(kEnableLeftPin, LOW);
    analogWrite(kSpeedLeftPin, 0);
  }
  else
  {
    digitalWrite(kEnableLeftPin, HIGH);
    digitalWrite(kDirectionLeftPin, leftMotor < 0);
    analogWrite(kSpeedLeftPin, abs(leftMotor));    
  }

  if (rightMotor == 0)
  {
    digitalWrite(kEnableRightPin, LOW);
    analogWrite(kSpeedRightPin, 0);
  }
  else
  {
    // Right hand motor is reversed
    digitalWrite(kEnableRightPin, HIGH);
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
  pinMode(kDirectionLeftPin, OUTPUT);
  pinMode(kEnableRightPin, OUTPUT);
  pinMode(kSpeedRightPin, OUTPUT);
  pinMode(kDirectionRightPin, OUTPUT);
  pinMode(kBeerFlapAPin, OUTPUT);
  pinMode(kBeerFlapBPin, OUTPUT);

  digitalWrite(kEnableLeftPin, LOW);
  digitalWrite(kEnableRightPin, LOW);
  digitalWrite(kDirectionLeftPin, LOW);
  digitalWrite(kDirectionRightPin, LOW);
  digitalWrite(kBeerFlapAPin, LOW);
  digitalWrite(kBeerFlapBPin, LOW);
  analogWrite(kSpeedLeftPin, 0);
  analogWrite(kSpeedRightPin, 0);

  Serial.println("Starting Brigitte...");

  MotorControl(0, 0);
}

void ControlBeerFlap()
{
  long beerFlapControl = GetValueForChannel(3, 100);
  
  if (beerFlapControl > 50)
  {
    digitalWrite(kBeerFlapAPin, HIGH);    
    digitalWrite(kBeerFlapBPin, HIGH);    
    Serial.print("beer flap: up ");
  }
  else
  {
    digitalWrite(kBeerFlapAPin, LOW);    
    digitalWrite(kBeerFlapBPin, LOW);        
    Serial.print("beer flap: down ");
  }

  Serial.print("beerFlapControl: " + String(beerFlapControl) + " ");
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
    
    long rawRightMotor = (v + w) / 2;
    long rawLeftMotor = (v - w) / 2;

    // Give us some sensitivity at the low end
    rightMotor = ((rawRightMotor * rawRightMotor) / stickValueRange);
    leftMotor = ((rawLeftMotor * rawLeftMotor) / stickValueRange);

    // Make sure we preserve the direction
    if (rawRightMotor < 0)
    {
      rightMotor = -rightMotor;
    }

    if (rawLeftMotor < 0)
    {
      leftMotor = -leftMotor;
    }
  }

  if ((stickX == stickValueRange) && (stickY == -stickValueRange))
  {
    // This happens if the RC receiver isn't initialised, all stop
    MotorControl(0, 0);

    Serial.print("RC controller is off");
  }
  else
  {
    MotorControl(leftMotor, rightMotor);

    Serial.print("stickX: " + String(stickX) + " ");
    Serial.print("stickY: " + String(stickY) + " ");
    Serial.print("L: " + String(leftMotor) + " ");
    Serial.print("R: " + String(rightMotor) + " ");
  
    ControlBeerFlap();  
  }

  Serial.println();

  delay(50);
}
