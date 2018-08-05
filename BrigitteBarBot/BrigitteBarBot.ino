/**
   Motor control for Brigitte BarBot
*/

#include <Wire.h>
#include <PPMReader.h>

#include "ComMotion.h"

// Range for stick values
const long stickValueRange = 255;

// Initialize a PPMReader on digital pin 2 with 6 expected channels.
int interruptPin = 2;
int channelCount = 6;
PPMReader ppm(interruptPin, channelCount);

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

  // If the absolute value is <10 then stop - this gives us a "dead zone)
  if (abs(value) < (stickValueRange / 10))
  {
    value = 0;
  }

  // Ensure the value is constrained to guard against interference or other weirdness
  value = constrain(value, -stickValueRange, stickValueRange);

  return value;
}

void setup()
{
  // Required to support I2C communication
  Wire.begin(1);
  Wire.setTimeout(1L);
  Serial.begin(9600);

  // Send Basic Configuration packet to controller
  // See ComMotion.h for argument definition list.
  //
  // We configure individual motor control with 1200 mA current limit
  // on each motor.
  BasicConfig(0, 19, 0, 120, 120, 120, 120, 0, 1);

  Serial.println("Starting Brigitte...");
  IndividualMotorControl(0, 0, 0, 0);
  delay(1000);
}

void loop()
{
  // Get the stick values. Channel 1 is left/right, channel 2 is forward/backward,
  // on a scale of -stickValueRange to +stickValueRange.
  long stickX = GetStickPositionForChannel(1);
  long stickY = GetStickPositionForChannel(2);

  Serial.print("stickX: " + String(stickX) + " ");
  Serial.print("stickY: " + String(stickY) + " ");

  // If they are both 0 then all stop
  if ((stickX == 0) && (stickY == 0))
  {
    Serial.println("All stop");
    IndividualMotorControl(0, 0, 0, 0);
    return;
  }

  // Calculate left and right motor speeds on a scale of -stickValueRange (full reverse) to +stickValueRange (full forward).
  // Adapted from http://home.kendra.com/mauser/joystick.html.
  stickX = -stickX;
  long v = ((stickValueRange - abs(stickX)) * stickY / stickValueRange) + stickY;
  long w = ((stickValueRange - abs(stickY)) * stickX / stickValueRange) + stickX;
  long rightMotor = (v + w) / 2;
  long leftMotor = (v - w) / 2;

  Serial.print("L: " + String(leftMotor) + " ");
  Serial.print("R: " + String(rightMotor) + " ");
  Serial.println();

  // Send the values to the motors
  IndividualMotorControl(leftMotor, leftMotor, rightMotor, rightMotor);  
}
