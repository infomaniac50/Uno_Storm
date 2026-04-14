/*
  
 Arduino program to demonstrate the SAME library functions.
 
 Note:  This is an example, not necessarily correct for real warnings!
 
 by Ray H. Dees
 
 01 APR 2012
 
 */
#include "Arduino.h"
#include <SAME.h>
//
//  LED Pin Definitions.
//
#define GREEN  14
#define YELLOW 15
#define ORANGE 16
#define RED    17
//
//  Location Codes.
//
#define LOCATION_1 51059
#define LOCATION_2 51600
#define LOCATION_3 51610
#define LOCATION_4 51620
#define LOCATION_5 13000
//
//  Setup Loop begins here.
//
void setup()
{
  delay(100);
  pinMode(GREEN, OUTPUT);
  digitalWrite(GREEN, HIGH);
  pinMode(YELLOW, OUTPUT);
  digitalWrite(YELLOW, LOW);
  pinMode(ORANGE, OUTPUT);
  digitalWrite(ORANGE, LOW);
  pinMode(RED, OUTPUT);
  digitalWrite(RED, LOW);
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
  
  if (SameStatus & ERR)      
    {
      SameStatus = 0;
      return;
    }  
  
  if (SameStatus & MSG)
    {
      Serial.print(F("Originator: "));
      Serial.println(SameOriginatorName);
      Serial.println();
      
      Serial.print(F("Event: "));
      Serial.println(SameEventName);
      Serial.println();
                 
      digitalWrite(GREEN, LOW);
      digitalWrite(YELLOW, LOW);
      digitalWrite(ORANGE, LOW);
      digitalWrite(RED, LOW);
      
      SameEventCode &= 0x000F;
      
      for (int i = 0; i < SameLocations; i++)
        {
          switch (SameLocationCodes[i])
            {
              case LOCATION_1:
              case LOCATION_2:
              case LOCATION_3:
              case LOCATION_4:
              case LOCATION_5:
                switch (SameEventCode)
                  {
                    case 0x01:
                    case 0x05:
                      digitalWrite(ORANGE, HIGH);
                      break;
                    case 0x02:
                    case 0x07:
                    case 0x0D:
                      digitalWrite(RED, HIGH);
                      break;
                    case 0x03:
                      digitalWrite(YELLOW, HIGH);
                      break;
                    default:
                      digitalWrite(GREEN, HIGH);
                      break;
                  }    
                 break;
            default:
              break;
          }
        }  
   }
 
  SameStatus = 0;  
}
//
//  The end.
//
