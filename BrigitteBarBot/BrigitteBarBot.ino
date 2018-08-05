/**
   Motor control for Brigitte BarBot
*/

#include <Wire.h>

#include "ComMotion.h"

void setup()
{
  // Required to support I2C communication
  Wire.begin(1);
  Wire.setTimeout(1L);
  Serial.begin(9600);

  // Send Basic Configuration packet to controller
  // See ComMotion.h for argument definitiion list
  BasicConfig(0, 19, 0, 100, 100, 100, 100, 0, 1);

  Serial.println("Starting ComMotion...");
  IndividualMotorControl(0, 0, 0, 0);
  delay(1000);
}

void loop()
{
  // Speed commands for the four motors.
  // + = forward, - = reverse
  // Speed = 1..255 with 0 = stop
  Serial.println("50% forward for 2 sec.");
  IndividualMotorControl(128, 128, 128, 128);
  delay(2000);

  Serial.println("50% reverse for 2 sec.");
  IndividualMotorControl(0, 0, 0, 0);
  IndividualMotorControl(-128, -128, -128, -128);
  delay(2000);

  Serial.println("Ramp to full speed, 1 sec per step");
  for (int i = 100; i <= 250; i += 25)
  {
    Serial.print("Speed command: ");
    Serial.println(i);
    IndividualMotorControl(i, i, i, i);
     delay(1000);
  }

  Serial.println("Start over in 10 sec.\n");
  IndividualMotorControl(0, 0, 0, 0);
  delay(10000);
}
