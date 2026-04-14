 /*
  SAME Test Message parser.  Allows you to create your own messages for testing.
  
  Compiled with Arduino version 1.0 using ATMEGEA328.
 
  by Ray H. Dees
  
  09 JUN 2012
  
*/
#include "Arduino.h"
#include <SAME.h>
//
//  Insert Your Messages Here!
//
const char MESSAGE1[] = "ZCZC-WXR-RWT-013057-013063-013067-013089-013097-013113-013117-013121-013135-013151-013217-013247-013297+0030-1611200-KFFC/NWS-";  
const char MESSAGE2[] = "NNNN";
//
//
//  Setup Loop.
//
void setup()
{
  delay(1000);
  Serial.begin(115200);
  delay(1000);
  Serial.println("Filling buffer............");
  Serial.println();
  delay(1000);
  
  Same.fill(MESSAGE1);
    
  Serial.println();
  delay(1000);
  Serial.println("Processing............");
  delay(1000);
  Serial.println();
  Serial.println();
  Same.parse();       //  Parse the message.
  SameStatus |= PRE;  //  Simulate that a Preamble was detected.
  process_same();
  delay(3000);
  SameStatus |= WAT;  //  Simulate that the Warning Alert Tone was received.
  process_same();
  delay(3000);
  
  Same.fill(MESSAGE2);  
  
  Same.parse();
  process_same();
  
}
//
//  Main Loop.
//
void loop()
{
  
}
//
//
//
void process_same()
{
  if (SameStatus & PRE)
    {
      Serial.println(F("Preamble detected."));
      Serial.println();
    }
      
  if (SameStatus & HDR)
    {
      Serial.print(F("Header: "));
      Serial.println(SameHeader);
      Serial.println();
    }
            
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
      Serial.println(F("Warning Alert Tone detected."));
      Serial.println();
    }  
  
  if (SameStatus & EOM)
    {
      Serial.println(F("End Of Message detected."));
      Serial.println();
    }  

  if (SameStatus & ERR)
    {
      Serial.println(F("Receive Error!"));
      Serial.println();
    }  
  
  Serial.print(F("Status: "));
  Serial.println(SameStatus, BIN);
  Serial.println();
  
  SameStatus = 0x00;
}  
//
//  The End.
//

