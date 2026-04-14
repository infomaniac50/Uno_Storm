/*
  NOAA Weather Radio SAME Decoder to Twitter interface.
  
  Adapted from SimplePost.
  
  NOTE:  THIS PROGRAM REQUIRES A MEGA1280 / MEGA2560
         OR EQUIVALENT TO RUN.  
  
*/  

#include "Arduino.h"
#include <SPI.h>
#include <Ethernet.h>
#include <Twitter.h>
#include <SAME.h>
//
//  Location Codes & Names are defined here. (http://nws.noaa.gov/nwr/indexnw.htm)
//
#define LOCATION_CODE_1 51059  //  Leave the leading "0" off of the Location Codes.
#define LOCATION_CODE_2 51600
#define LOCATION_CODE_3 51610
#define LOCATION_CODE_4 51620
#define LOCATION_CODE_5 13000
//
char LOCATION_NAME_1[] = "Lee ";
char LOCATION_NAME_2[] = "Russell ";
char LOCATION_NAME_3[] = "Chattahoochee ";
char LOCATION_NAME_4[] = "Harris ";
char LOCATION_NAME_5[] = "Muscogee ";
char LOCATION_NAME_9[] = "Not in this area.";
//
//  Ethernet Shield Settings.
//
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
//
// If you don't specify the IP address, DHCP is used (Only in Arduino 1.0 or later).
//
byte ip[] = {192, 168, 1, 65};
//
// Your Token to Tweet (get it from http://arduino-tweet.appspot.com/)
//
Twitter twitter("Your Token to Tweet");
//
// Alert message to post to Twitter.
//
char alert_message[160] = "A ";
int alert_index = 2;
char alert_area[] = " is in effect for the following counties: ";
//
//  Setup begins here.
//
void setup()
{
  delay(1000);
  
  if (!Ethernet.begin(mac))  // Try to connect using DHCP
    Ethernet.begin(mac, ip); // Try to connect using a Static IP address.
  delay(100);
  
  Serial.begin(115200);
  delay(100);
  
  Serial.print(F("Connected to IP address: "));  // Print out the IP address.
  for (int i = 0; i < 4; i++)
    {
      Serial.print(Ethernet.localIP()[i], DEC);
      Serial.print("."); 
    }
  Serial.println();
  Serial.println();
  
  Same.begin();
  delay(1000);
      
  // Say that we're ready.
  Serial.println(F("Ready to decode and Post Alert Messages...."));
  Serial.println();
  Serial.println();
}

void loop()
{
  if (SameStatus > 0)  //  Go and process the Same message.
    process_the_message();

  delay(250);   //  Something has to slow this program down.  
}
//
//  Parse the Same message and create the Twitter alert.
//
void process_the_message()
{
  int i;
  int j;
  int count = 0;
  
  if (SameStatus & AVL)
    Same.parse();
  
  if (SameStatus & ERR || SameStatus & EOM)      
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
      
      for (i = 0; i < strlen(SameEventName); i++)
        alert_message[alert_index + i] = SameEventName[i];
            
      alert_index = strlen(alert_message);
      
      for (i = 0; i < strlen(alert_area); i++)
        alert_message[alert_index + i] = alert_area[i];
      
      for (int i = 0; i < SameLocations; i++)
        {
          switch (SameLocationCodes[i])
            {
              case LOCATION_CODE_1:
                alert_index = strlen(alert_message);
                for (j = 0; j < strlen(LOCATION_NAME_1); j++)
                  alert_message[alert_index + j] = LOCATION_NAME_1[j];
                count++;  
                break;
                
              case LOCATION_CODE_2:
                alert_index = strlen(alert_message);
                for (j = 0; j < strlen(LOCATION_NAME_2); j++)
                  alert_message[alert_index + j] = LOCATION_NAME_2[j];
                count++;  
                break;
              
              case LOCATION_CODE_3:
                alert_index = strlen(alert_message);
                for (j = 0; j < strlen(LOCATION_NAME_3); j++)
                  alert_message[alert_index + j] = LOCATION_NAME_3[j];
                count++;  
                break;
              
              case LOCATION_CODE_4:
                alert_index = strlen(alert_message);
                for (j = 0; j < strlen(LOCATION_NAME_4); j++)
                  alert_message[alert_index + j] = LOCATION_NAME_4[j];
                count++;  
                break;
              
              case LOCATION_CODE_5:
                alert_index = strlen(alert_message);
                for (j = 0; j < strlen(LOCATION_NAME_5); j++)
                  alert_message[alert_index + j] = LOCATION_NAME_5[j];
                count++;  
                break;
                
              default:
                break;
            }    
        }
      
      if (count <= 0)
        {
          alert_index = strlen(alert_message);
          for (j = 0; j < strlen(LOCATION_NAME_9); j++)
            alert_message[alert_index + j] = LOCATION_NAME_9[j];
        }    
            
      Serial.print(F("Twitter Post: "));
      Serial.println(alert_message);
      Serial.println();
  
      post();  //  Post the alert.
      
      Same.flush();
      
      alert_index = 2;  //  Reset the alert_index.
  
      for (i = alert_index; i < sizeof(alert_message); i++)  //  Clear out the alert message string.
        alert_message[i] = 0x00;
  
      
      SameStatus = 0;  //  Reset SameStatus.
    }
}  
//
//  The Alert Message gets posted here.
void post()
{
  Serial.println("Connecting ...");
  Serial.println();
  
  if (twitter.post(alert_message))
    {
      int status = twitter.wait();
      
      if (status == 200)
        Serial.println("Message posted.");
      
      else
        {
          Serial.print("Message failed code:  ");
          Serial.println(status);
        }
    } 
  
  else
    Serial.println("Connection failed.");
  
  Serial.println();
  Serial.println();
  delay(1000);
}

//
//
//  End.
    
