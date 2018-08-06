
//============================================================================== Generate 2kHz beeps =======================================================

void Beep(byte beeps)
{
  for (int b = 0; b < beeps; b++)                                             // loop to generate multiple beeps
  {
    PORTB = PORTB | B00001111;  
    // set all motor direction and PWM pins high
    for (int duration = 0; duration < 600; duration++)                        // generate 2kHz tone for 200mS
    {
      delayMicroseconds(250);                                                 // wait 250uS to generate 2kHz tone
      PORTB = PORTB ^ B00001001;                                              // toggle direction bits
    }
    
    PORTB = PORTB & B11110000;                                                // turn off all motors
    delay(200);                                                               // pause for 200mS (1/5th of a second) between beeps
  }
}
