#include <Wire.h>

void setup() {
  Serial.begin (115200);

  while (!Serial)
    {
    }

  Serial.println ();
  Serial.println ("I2C scanner. Scanning ...");
  byte count = 0;
 
  Wire.begin();
  for (byte i = 1; i < 127; i++)
  {
    Wire.beginTransmission (i);
    if (Wire.endTransmission () == 0)
      {
      Serial.print ("Found address: ");
      Serial.print (i, DEC);
      Serial.print (" (0x");
      Serial.print (i, HEX);
      Serial.println (")");
      count++;
      delay (10);
      } // end of good response 
    else if (Wire.endTransmission () == 4)
      {
        Serial.print("Unknown  ");
        if(i < 16)
          Serial.print("0");
        Serial.println(i, HEX);
      }
  } // end of for loop
  Serial.println ("Done.");
  Serial.print ("Found ");
  Serial.print (count, DEC);
  Serial.println (" device(s).");
}  // end of setup

void loop() {}
