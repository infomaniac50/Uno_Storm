/*
  
 Arduino program to demonstrate the SAME library functions.
 
 by Ray H. Dees
 
 01 APR 2012
 
 */
#include "Arduino.h"
#include <SAME.h>
//
//  Setup Loop begins here.
//
void setup()
{
  delay(100);
  Serial.begin(115200);
  delay(100);
  Same.begin();
  delay(1000); 
  Serial.println("Ready to receive......");
  Serial.println();
  Serial.println();
}
//
//  Main Loop begins here.
//
void loop()
{

  if (Same.available() > 0)  //  Demonstrates a simple print capability - Could be used to send a valid SAME message to an external source.
    send_the_message();


  delay(250);   //  Something has to slow this program down.  
}
//
//  Send the data to the Serial port.
//
void send_the_message()  //  Prints out the SAME receive buffer.
{
  while (Same.available() > 0)
  {
    char same_character = Same.read();
    Serial.print(same_character);
  }
  Same.flush();  //  Flush the SAME buffers.
  Serial.println();      
  Serial.println();
}  
//
//  The end.
//

