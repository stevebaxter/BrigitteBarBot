
//============================================================== Receive I²C command from external device or other MCU =======================

void I2C_Receive(int bytes)                                   // received command as I²C slave
{
  for (byte i = 0; i < bytes; i++)
  {
    datapack[i] = Wire.read();                                // transfer data from I²C buffer to datapack
  }

  command = datapack[0];
  packsize = bytes;
}




