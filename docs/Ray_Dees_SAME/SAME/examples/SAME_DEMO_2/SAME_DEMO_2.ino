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
  if (SameStatus > 0)  //  Demonstrates a method of parsing the data and printing it to the Serial Monitor.
    process_the_message();

  delay(250);   //  Something has to slow this program down.  
}
//
//
//
void process_the_message()
{
  if (SameStatus & AVL)
    Same.parse();
        
  if (SameStatus & MSG)
    {
      Serial.print(F("Originator: "));
      Serial.println(SameOriginatorName);
      Serial.println();
      
      Serial.print(F("Event: "));
      Serial.println(SameEventName);
      Serial.println();
      
      Serial.print(F("Locations: "));
      Serial.println(SameLocations);
      Serial.println();
      
      Serial.print(F("Location Codes: "));
  
      for (int i = 0; i < SameLocations; i++)
        {
          Serial.print(SameLocationCodes[i]);
          Serial.print(' ');
        }  
  
      Serial.println();
      Serial.println();  
  
      Serial.print(F("Duration: "));
      Serial.println(SameDuration);
      Serial.println();
      
      Serial.print(F("Day: "));
      Serial.println(SameDay);
      Serial.println();
      
      Serial.print(F("Time: "));
      Serial.println(SameTime);
      Serial.println();
             
      Serial.print(F("Callsign: "));
      Serial.println(SameCallSign);
      Serial.println();
    }
    
  if (SameStatus & WAT)
    {
      Serial.print(F("Warning Alert Tone detected."));
      Serial.println();
      Serial.println();
    }
  
  if (SameStatus & EOM)
    {
      Serial.print(F("End Of Message."));
      Serial.println();
      Serial.println();
    }
  
  if (SameStatus & ERR)
    {
      Serial.println(SameStatus, BIN);
      Serial.print(F("A receive error occurred."));
      Serial.println();
      Serial.println();
    }
  
  SameStatus = 0;  
}
//
//  The end.
//
