/*
  SAME.cpp 
 
  Arduino program for receiving and displaying messages (in ASCII format) that are
  broadcast using the NOAA Weather Radio (NWR) Specific Area Message Encoding (SAME)
  format.  It is based on National Weather Service Instruction 10-1712, dated
  October 3, 2011.  Event Codes are taken from ANSI/CEA-2009-B, dated November, 2010.
 
  Copyright 2012 Ray H. Dees
  
  PLL receive routine adapted from VirtualWire, Copyright (C) 2008 by Mike McCauley.
  
  Compiled with Arduino 1.0 and using a 168/328 or 1280/2560 Arduino running on 5 vdc
  at 16 mHz.  Uses XR-2211 FSK Demodulators (or other circuitry that you design)
  to receive data.
  
  In this example program:
 
  Arduino Digital Pin 2 is Warning Alert Tone, Active Low.
  Arduino Digital Pin 3 is Carrier Detect, Active Low.
  Arduino Digital Pin 5 is Receive Data.
  Arduino Digital Pin 7 is SAME Activity, On during reception.
 
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/
#include "Arduino.h"
#include "SAME.h"
#include "SAME_MESSAGES.h"
//
//  SAME Receive Status.
//
volatile uint8_t SameStatus = 0;
//
//  Global Variables.
//
uint16_t SameOriginatorCode;
uint16_t SameEventCode;
uint16_t SamePlusIndex;
uint16_t SameLocations;
uint32_t SameLocationCodes[SAME_LOCATION_CODES];
uint16_t SameDuration;
uint16_t SameDay;
uint16_t SameTime;
//
char SameHeader[5];
char SameCallSign[9];
char SameOriginatorName[SAME_ORIGINATOR_DISPLAY_LENGTH];
char SameEventName[SAME_EVENT_DISPLAY_LENGTH];
//
//  Static Class Variables.
//
volatile uint8_t SameClass::rxSample;
volatile uint8_t SameClass::rxLastSample;
volatile uint8_t SameClass::rxPllRamp;
volatile uint8_t SameClass::rxIntegrator;
//
volatile uint16_t SameClass::rxBits;
volatile uint16_t SameClass::rxBitCount;
volatile uint8_t  SameClass::rxSynchronized;
//
char SameClass::rxBuffer0[SAME_BUFFER_SIZE];  
char SameClass::rxBuffer1[SAME_BUFFER_SIZE];
uint8_t SameClass::rxCount[SAME_BUFFER_SIZE];
//
volatile uint16_t SameClass::rxBufferIndex;
volatile uint16_t SameClass::rxBufferLength;
volatile uint16_t SameClass::rxBufferPointer;
//
//  Begin SAME Processing.
//
void SameClass::begin(void)
{
  NO_INTERRUPTS;                                    //  Disable global interrupts while we make our changes.
  
  pinMode(WARNING_ALERT, INPUT);
  digitalWrite(WARNING_ALERT, HIGH); 
  
  pinMode(CARRIER_DETECT, INPUT);
  digitalWrite(CARRIER_DETECT, HIGH); 

  pinMode(RECEIVE_DATA, INPUT);
  digitalWrite(RECEIVE_DATA, HIGH); 
  
  pinMode(SAME_ACTIVITY, OUTPUT);
  digitalWrite(SAME_ACTIVITY, LOW);
 
  //  Setup the microprocessor.  (Done this way to override whatever Arduino had going.)
  
#if defined (__AVR_ATmega168__) || defined (__AVR_ATmega328P__)
  EICRA = 0x00;
  EICRA |= (1 << ISC10 | 1 << ISC00);               //  Setup Interrupt 0 & 1 for any logical Change.
  EIFR  |= (1 << INTF1 | 1 << INTF0);               //  Clear pending interrupts.
  EIMSK |= (1 << INT1 | 1 << INT0);                 //  Enable Interrupt 0 & 1.
#elif defined (__AVR_ATmega1280__) || defined (__AVR_ATmega2560__)
  EICRB = 0x00;
  EICRB |= (1 << ISC50 | 1 << ISC40);               //  Setup Interrupt 4 & 5 for any logical Change.
  EIFR  |= (1 << INTF5 | 1 << INTF4);               //  Clear pending interrupts.
  EIMSK |= (1 << INT5 | 1 << INT4);                 //  Enable Interrupt 4 & 5.
#endif  

  ADCSRA = 0x00;                                    //  Disable the analog comparator.
  //TCCR0B = 0x00;                                  //  Disable Timer 0 - Leave this enabled or the Arduino delay() will not function. 
  
  TCCR1A = 0x00;                                    //  Reset TCCR1A to Normal mode.
  TCCR1B = 0x00;                                    //  Reset TCCR1B.  Defined as TIMER1_STOP. 
  //TCCR1B |= (1 << WGM12 | 1 << CS12 | 1 << CS10); //  Set CTC Mode, prescale by 1024.  Defined as TIMER1_START.
  TIFR1 = (1 << OCF1B | 1 << OCF1A | 1 << TOV1);    //  Clear pending interrupts.
  OCR1A = 0xF423;                                   //  Compare Match at 4 seconds.
  TIMSK1 = 0x00;                                    //  Reset TIMSK1.
  TIMSK1 |= (1 << OCIE1A);                          //  Timer 1 Compare Match A Interrupt Enable.
  
  ASSR = 0x00;                                      //  Set Timer 2 to System Clock.
  TCCR2A = 0x00;                                    //  Reset TCCR2A.  
  TCCR2A = (1 << WGM21);                            //  Set CTC mode.
  TCCR2B = 0x00;                                    //  Reset TCCR2B.
  TCCR2B = (1 << CS21 | 1 << CS20);                 //  Prescale by 32. 
  TIFR2 = (1 << OCF2B | 1 << OCF2A | 1 << TOV2);    //  Clear pending interrupts.
  OCR2A = 0x77;                                     //  Set Compare Match for 240 microseconds.
  TIMSK2 = 0x00;                                    //  Reset TIMSK2.  TIMSK2 &= ~(1 << OCIE2A) Defined as TIMER2_STOP.
  //TIMSK2 = (1 << OCIE2A);                         //  Timer 2 Compare Match A Interrupt Enable. Defined as TIMER2_START.
  
  reset();                                          //  Clear all variables.
  
  INTERRUPTS;                                       //  Restore Interrupt Status. 
}
//
//  End SAME processing.
//
void SameClass::end(void)
{
  NO_INTERRUPTS;                                    //  Disable global interrupts while we make our changes.
  
#if defined (__AVR_ATmega168__) || defined (__AVR_ATmega328P__)  
  EIFR  |= (1 << INTF1 | 1 << INTF0);               //  Clear pending interrupts.
  EIMSK &= ~(1 << INT1 | 1 << INT0);                //  Disable Interrupt 0 & 1.
#elif defined (__AVR_ATmega1280__) || defined (__AVR_ATmega2560__)
  EIFR  |= (1 << INTF5 | 1 << INTF4);               //  Clear pending interrupts.
  EIMSK &= ~(1 << INT5 | 1 << INT4);                //  Disable Interrupt 4 & 5.
#else 
  #error Processor Not Supported!  
#endif
  
  TCCR1B = 0x00;                                    //  Stop Timer 1.
  TIFR1 = (1 << OCF1B | 1 << OCF1A | 1 << TOV1);    //  Clear pending interrupts.
  TIMSK1 = 0x00;                                    //  Disable Timer 1 interrupts.
  
  TCCR2B = 0x00;                                    //  Stop Timer 2.
  TIFR2 = (1 << OCF2B | 1 << OCF2A | 1 << TOV2);    //  Clear pending interrupts.
  TIMSK2 = 0x00;                                    //  Disable Timer 2 interrupts.
  
  INTERRUPTS;                                       //  Restore Interrupt Status. 
}  
//
//  The received samples get processed into bits and bytes here.
//
void SameClass::pll(void)
{
  rxSample = digitalRead(RECEIVE_DATA);
  
  if (rxSample)
    rxIntegrator++;

  if (rxSample != rxLastSample)
    {
      rxPllRamp += ((rxPllRamp < SAME_RAMP_TRANSITION) 
        ? SAME_RAMP_INC_RETARD 
        : SAME_RAMP_INC_ADVANCE);
      rxLastSample = rxSample;
    }
  
  else
    rxPllRamp += SAME_RAMP_INC;
  
  if (rxPllRamp >= SAME_RX_RAMP_LEN)
    {
      rxBits >>= 1;

      if (rxIntegrator >= 5)
        rxBits |= 0x0000;
      
      else
        rxBits |= 0x8000;
      
      rxPllRamp -= SAME_RX_RAMP_LEN;
      rxIntegrator = 0;
      rxBitCount++;
      
      if (!rxSynchronized)
        {
          if (rxBits != SAME_LONG_PREAMBLE)
            return;
          rxSynchronized = 1;
        }    
            
      if (rxBitCount > 7)
        {
          rxBits >>= 8;
          
          if (rxBits == 0x00)
            {
              rxBuffer0[rxBufferIndex] = 0x00;
              rxBuffer1[rxBufferIndex] = 0x00;
              rxCount[rxBufferIndex] = SAME_VALIDATE_COUNT;
              update_rx_buffers(rxBufferIndex, rxBits);
              TIMER2_STOP;
              return;
            }
                    
          
          if (rxBits >= 0x20 && rxBits <= 0x7F)
            {
              update_rx_buffers(rxBufferIndex, rxBits);
              
              if (rxBufferIndex < SAME_BUFFER_SIZE)
                rxBufferIndex++;
            }
          
          rxBitCount = 0;
        }  
    }
}    
//
//  The SAME validation process takes place here.
//
void SameClass::update_rx_buffers(uint16_t rxBufferIndex, uint8_t rxBits)
{
  int i;
  uint8_t same_complete = 1;
  
  if (rxBuffer0[rxBufferIndex] == rxBits)
    {
      if (rxCount[rxBufferIndex] < SAME_VALIDATE_COUNT)
        {
          rxCount[rxBufferIndex]++;
        }
        
      else
        {
          rxCount[rxBufferIndex] = SAME_VALIDATE_COUNT;
          rxBuffer1[rxBufferIndex] = rxBits;
        }
    }
    
  else if (rxBuffer1[rxBufferIndex] == rxBits)
    {
      if (rxCount[rxBufferIndex] >= SAME_VALIDATE_COUNT)
        {
          rxCount[rxBufferIndex] = SAME_VALIDATE_COUNT + 1;
        }
        
      else
        {
          rxCount[rxBufferIndex] = SAME_VALIDATE_COUNT;
        }
        
      rxBuffer1[rxBufferIndex] = rxBuffer0[rxBufferIndex];
      rxBuffer0[rxBufferIndex] = rxBits;
    }
    
  else if(!rxCount[rxBufferIndex])
    {
      rxBuffer0[rxBufferIndex] = rxBits;
      rxCount[rxBufferIndex] = 1;
    }
    
  else
    {
      rxBuffer1[rxBufferIndex] = rxBits;
    }

  for (i = 0; i < sizeof(rxBuffer0); i++)
    {  
      if (rxBuffer0[i] == 0x00)
        break;
            
      if (rxCount[i] < SAME_VALIDATE_COUNT)
        same_complete = 0;
    }
  
  if (same_complete)
    {
      rxBufferPointer = 0;
      rxBufferLength = i;
      SameStatus |= AVL;
    }  
}    
//
//  Fill SAME rxBuffer0 for testing purposes.
//
void SameClass::fill(const String &s)
{
  flush();  //  Start by clearing out the buffers. 
  
  for (uint16_t i = 0; i < s.length(); i++)
    {
      rxBuffer0[i] = s[i];
      rxBufferLength++;
    }  
}
//
//  Return available character count.
//
int SameClass::available(void)  //  Used with the Same.read() function.
{
  if (rxBufferPointer == rxBufferLength)
    return -1;
  
  else
    return rxBufferLength - rxBufferPointer;
}  
//
//  Return received characters.
//
char SameClass::read(void) //  Used with the Same.available() function.
{
  char value = 0x00;
     
  if (rxBufferPointer < rxBufferLength)
    {
      value = rxBuffer0[rxBufferPointer];
      rxBufferPointer++;
    }
  
  else
    rxBufferPointer = rxBufferLength = 0;
      
  return value;
}
//
//  The SAME message is parsed here.  (Could be more eloquent, but it works!)
//
void SameClass::parse(void) 
{
  SameOriginatorCode = 0;
  SameEventCode = 0;
  SamePlusIndex = 0;
  SameLocations = 0;
  SameDuration = 0;
  SameDay = 0;
  SameTime = 0;
  SameStatus = 0;
  
  int i = 0;
  
  if (rxSynchronized)   //  If the Preamble was detected,
    SameStatus |= PRE;  //  set the SameStatus bit.
      
  i |= (rxBuffer0[0] <<  8);
  i |= (rxBuffer0[1] <<  0);
  
  switch (i)
    {
      case 0x5A43:          //  "ZC"
        SameStatus |= HDR;  //  If the Header was detected, set the SameStatus bit.
        break;
      case 0x4E4E:          //  "NN"
        flush();
        SameStatus |= EOM;  //  If the End Of Message was detected, set the SameStatus bit.
        return;
      default:              // No match.
        flush();
        SameStatus |= ERR;  // Must have contained errors, set the SameStatus bit 
        return;             //  and return.
    }
      
  for (i = 0; i < 4; i++)
    {
      SameHeader[i] = char(rxBuffer0[i]); //  Read in the new, clear the old.
      SameHeader[i + 1] = (0x00);         //  SameHeader is ready at completion.
    }
  
  i = 4;
  
  SameOriginatorCode += (rxBuffer0[i + 1] << 8);  //  *256  //  Extract the Originator code from the buffer.
  SameOriginatorCode += (rxBuffer0[i + 2] << 4);  //   *16
  SameOriginatorCode += (rxBuffer0[i + 3] << 0);  //    *0
  
  SameEventCode += (rxBuffer0[i + 5] << 8);   //  *256     //  Extract the Event code from the buffer.
  SameEventCode += (rxBuffer0[i + 6] << 4);   //   *16
  SameEventCode += (rxBuffer0[i + 7] << 0);   //    *0
  
  for (i = 0; i < 4; i++)  //  Lookup the Originator Name and copy it.
    {
      if (SameOriginatorCode == (pgm_read_word(&(SAME_ORIGINATOR_CODES[i]))))
        {
          strcpy_P(SameOriginatorName, (char*)pgm_read_word(&(SAME_ORIGINATOR_STRING_INDEX[i])));
          break;
        }
      
      else
        strcpy_P(SameOriginatorName, (char*)pgm_read_word(&(SAME_ORIGINATOR_STRING_INDEX[04])));  // If no match set to "Unrecognized".  
    }  
  
  for (i = 0; i < 81; i++)  //  Lookup the Event Name and copy it.
    {
      if (SameEventCode == (pgm_read_word(&(SAME_EVENT_CODES[i]))))
        {
          strcpy_P(SameEventName, (char*)pgm_read_word(&(SAME_EVENT_STRING_INDEX[i])));  
          break;
        }
      
      else
        strcpy_P(SameEventName, (char*)pgm_read_word(&(SAME_EVENT_STRING_INDEX[81])));  // If no match set to "Unrecognized".
    }
    
  for (i = 12; i < sizeof(rxBuffer0); i++)  //  Look for the Plus Sign.
    {
      if (rxBuffer0[i] == 0x2B)
        SamePlusIndex = i;     //  Found it.
      
      if (rxBuffer0[i] >= 0x30 && rxBuffer0[i] <= 0x39) //  If the value is numeric, strip off the upper bits.
        rxBuffer0[i] = rxBuffer0[i] & 0x0F;
    }  
  
  if (SamePlusIndex == 0)     //  No Plus Sign found.
    {
      SameStatus |= ERR;      //  Set the Error bit
      return;                 //  and return.
    }

  for (i = 12; i < SamePlusIndex; i++)  //  There are no SameLocationCodes past the SamePlusIndex.   
    {
      if (rxBuffer0[i] == 0x2D)  
        {
          SameLocationCodes[SameLocations] = 0;  //  Clear out any remaining data.
          SameLocationCodes[SameLocations] += rxBuffer0[i + 1] * 100000UL;
          SameLocationCodes[SameLocations] += rxBuffer0[i + 2] *  10000UL;
          SameLocationCodes[SameLocations] += rxBuffer0[i + 3] *   1000UL;
          SameLocationCodes[SameLocations] += rxBuffer0[i + 4] *    100UL;
          SameLocationCodes[SameLocations] += rxBuffer0[i + 5] *     10UL;
          SameLocationCodes[SameLocations] += rxBuffer0[i + 6] *      1UL;
          
          if (SameLocations > SAME_LOCATION_CODES) //  SAME_LOCATION_CODES (31) is the maximum allowed.
            {
              SameStatus |= ERR;
              return;
            }
          else
            SameLocations++;
        }
    }  
    
  SameDuration += rxBuffer0[SamePlusIndex + 1] * 600;
  SameDuration += rxBuffer0[SamePlusIndex + 2] *  60;
  SameDuration += rxBuffer0[SamePlusIndex + 3] *  10;
  SameDuration += rxBuffer0[SamePlusIndex + 4] *   1;

  SameDay += rxBuffer0[SamePlusIndex + 6] * 100;
  SameDay += rxBuffer0[SamePlusIndex + 7] *  10;
  SameDay += rxBuffer0[SamePlusIndex + 8] *   1;

  SameTime += rxBuffer0[SamePlusIndex +  9] * 1000;
  SameTime += rxBuffer0[SamePlusIndex + 10] *  100;
  SameTime += rxBuffer0[SamePlusIndex + 11] *   10;
  SameTime += rxBuffer0[SamePlusIndex + 12] *    1;
  
  for (i = 0; i < 8; i++)
    {
      if (rxBuffer0[i + SamePlusIndex + 14] == 0x2D || rxBuffer0[i + SamePlusIndex + 14] == 0x00)
        {
          SameCallSign[i] = 0x00;
          break;
        }  
      
      SameCallSign[i] = (rxBuffer0[i + SamePlusIndex + 14]);
    }
  
  flush();
  SameStatus |= MSG;
}
//
//  Flush the SAME receive buffers.
//
void SameClass::flush(void) //  Can be called as needed.
{
  for (int i = 0; i < SAME_BUFFER_SIZE; i++)  //  Clear out the SAME buffers.
    {
      rxBuffer0[i] = 0x00;
      rxBuffer1[i] = 0x00;
      rxCount[i] =   0x00;
    }
    
  rxBufferIndex = rxBufferPointer = rxBufferLength = 0;
}
//
//  The Warning Alert Tone functions are done here.
//
void SameClass::wat(void) //  Should only be called by Interrupt 0.
{
  uint8_t temp = digitalRead(WARNING_ALERT);  //  The message portion is only used with Same.available().
  
  if (!temp)
    {
      SameStatus |= WAT;   //  Set the WAT SameStatus bit.  
      strcpy_P(rxBuffer0, (char*)pgm_read_word(&(SAME_EVENT_STRING_INDEX[82])));  // Copy the WAT On String.
      rxBufferLength = SAME_WAT_ON_DISPLAY_LENGTH;
    }  
  
  else
    {
      SameStatus &= ~WAT;   //  Clear the WAT SameStatus bit.
      strcpy_P(rxBuffer0, (char*)pgm_read_word(&(SAME_EVENT_STRING_INDEX[83])));  // Copy the WAT Off String.
      rxBufferLength = SAME_WAT_OFF_DISPLAY_LENGTH;
    }
  
  rxBufferPointer = 0;    
  
  TIMER1_STOP;          //  Stop Timer1. (If it was running.)
  TIMER1_START;         //  Restart Timer1 to give the full 4 seconds.  
}  
//
//  Reset all variables to zero.
//
void SameClass::reset(void)
{
  rxSample = 0x00;
  rxLastSample = 0x00;
  rxPllRamp = 0x00;
  rxIntegrator = 0x00;
  rxBits = 0x00;
  rxBitCount = 0;
  rxBufferIndex = 0;
  rxBufferLength = 0;
  rxBufferPointer = 0;
  rxSynchronized = 0;
  SameStatus = 0x00;
}
//
//  Interrupt 0 or 4 Service Routine - Triggered on the Rising and Falling edge of Warning Alert Tone.
//
#if defined (__AVR_ATmega168__) || defined (__AVR_ATmega328P__) 
ISR(INT0_vect)
#elif defined (__AVR_ATmega1280__) || defined (__AVR_ATmega2560__)
ISR(INT4_vect)
#endif
{
  Same.wat(); //  Go handle this.
}  
//
//  Interrupt 1 or 5 Service Routine - Triggered on the Rising and Falling edge of Carrier Detect.
//
#if defined (__AVR_ATmega168__) || defined (__AVR_ATmega328P__) 
ISR(INT1_vect)
#elif defined (__AVR_ATmega1280__) || defined (__AVR_ATmega2560__)
ISR(INT5_vect)
#endif
{
  uint8_t temp = digitalRead(CARRIER_DETECT);
  
  if (!temp)  //  Carrier Detect has gone low - Start of reception.
    {
      digitalWrite(SAME_ACTIVITY, HIGH);  //  Turn on the led.
      TIMER1_STOP;    //  Stop Timer 1.
      Same.reset();   //  Reset variables.
      TIMER2_START;   //  Start Timer 2.
    }  
  
  else       //  Carrier Detect has gone high - End of reception.
    {
      TIMER2_STOP;    //  Stop Timer 2.
      TIMER1_START;   //  Start Timer 1.
      digitalWrite(SAME_ACTIVITY, LOW); //  Turn off the led.  
    }
}    
//
//  Timer 1 Compare Match A Interrupt Service Routine -  Used as a 4 second timer.
//
ISR(TIMER1_COMPA_vect)
{
  uint8_t temp = digitalRead(CARRIER_DETECT);
  
  if (!temp)  //  Still receiving?
    {
      TCNT1 = 0x00;   //  Reset Timer 1
      return;         //  and return.
    }  
  
  digitalWrite(SAME_ACTIVITY, LOW); //  Turn off the led.
  TIMER1_STOP;    //  Stop Timer 1.
  Same.flush();   //  Clear the SAME receive buffers.
}
//
//  Timer 2 Compare Match A Interrupt Service Routine - Samples incoming data stream 8 times per bit. 
//
ISR(TIMER2_COMPA_vect)
{
  Same.pll();
}    
//  
SameClass Same;