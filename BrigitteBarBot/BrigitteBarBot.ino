/**
   Motor control for Brigitte BarBot
*/

#include <Wire.h>
#include <PPMReader.h>

#include "ComMotion.h"

// Range for stick values
const long stickValueRange = 255;

// Initialize a PPMReader on digital pin 2 with 8 expected channels.
#define kInterruptPin 2
#define kChannelCount 8
PPMReader ppm(kInterruptPin, kChannelCount);

// We filter values from the PPM controller as we seem to get spurious values
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
  value = FilterChannel(channel, value);

  // Convert to our range
  value = (value - centreValue) * (stickValueRange * 2) / valueRange;

  // If the absolute value is <10 then stop - this gives us a "dead zone)
  if (abs(value) < (stickValueRange / 10))
  {
    value = 0;
  }

  // Ensure the value is constrained to guard against interference or other weirdness
  value = constrain(value, -stickValueRange, stickValueRange);

  return value;
}

long LimitAcceleration(long newValue, long lastValue)
{
  if (newValue > lastValue)
  {
    newValue = min(newValue, lastValue + 10);
  }
  else if (newValue < lastValue)
  {
    newValue = max(newValue, lastValue - 10);
  }

  return newValue;
} 

void setup()
{
  // Required to support I2C communication
  Wire.begin();
  Wire.setTimeout(1L);

  Serial.begin(9600);

  ppm.channelValueMaxError = 20;
  ppm.blankTime = 5000;
  ppm.minChannelValue = 1100;
  ppm.maxChannelValue = 1900;

  // Initialise the channel filter
  memset(channelFilter, 0, kChannelCount * 3 * sizeof(long));
  memset(channelFilterIndex, 0, kChannelCount * sizeof(long));

  // Wait for the ComMotion shield to be available
  delay(3000);

  // Send Basic Configuration packet to controller
  // See ComMotion.h for argument definition list.
  //
  // We configure individual motor control with 1200 mA current limit
  // on each motor.
  BasicConfig(0, 19, 0, 120, 120, 120, 120, 0, 1);
  EncoderConfig(11500, 100, 10, 10);

  Serial.println("Starting Brigitte...");
  IndividualMotorControl(0, 0, 0, 0);
}

void loop()
{
  // Last values for motor control
  static long lastRightMotor = 0;
  static long lastLeftMotor = 0;

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
    IndividualMotorControl(0, 0, 0, 0);
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

  // Limit the speed of changes
  leftMotor = LimitAcceleration(leftMotor, lastLeftMotor);
  rightMotor = LimitAcceleration(rightMotor, lastRightMotor);

  // Send the values to the motors if they have changed
  if ((leftMotor != lastLeftMotor) || (rightMotor != lastRightMotor))
  {
    IndividualMotorControl(leftMotor, leftMotor, rightMotor, rightMotor);
    lastLeftMotor = leftMotor;
    lastRightMotor = rightMotor;
  }

    Serial.print("L: " + String(leftMotor) + " ");
    Serial.print("R: " + String(rightMotor) + " ");
    Serial.println();
  
  delay(50);
}
