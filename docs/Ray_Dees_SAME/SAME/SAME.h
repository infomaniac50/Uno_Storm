/*
  SAME.h 
 
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
//
#ifndef SAME_h
#define SAME_h
//
#include <inttypes.h>
#include <string.h>
//
//  Arduino Definitions.
//
#define WARNING_ALERT  2
#define CARRIER_DETECT 3
#define RECEIVE_DATA   5  //  Moved from Pin 4, to allow the SD card to work.
#define SAME_ACTIVITY  7  //  Moved from Pin 13, to allow the SPI to work.
//
#ifndef NO_INTERRUPTS
#define NO_INTERRUPTS  uint8_t sreg = SREG; cli()
#define INTERRUPTS     SREG = sreg
#endif
//
#ifndef TIMER1_START
#define TIMER1_START  TCNT1 = 0x00; TCCR1B |= (1 << WGM12 | 1 << CS12 | 1 << CS10)
#define TIMER1_STOP   TCCR1B = 0x00
#define TIMER2_START  TIMSK2 = (1 << OCIE2A)
#define TIMER2_STOP   TIMSK2 &= ~(1 << OCIE2A)
#endif
//
//  SAME Definitions.  
//
#define SAME_VALIDATE_COUNT 2   //  Must be 1, 2 or 3, nothing else!
#define SAME_PREAMBLE 0xAB
#define SAME_LONG_PREAMBLE (SAME_PREAMBLE << 8 | SAME_PREAMBLE)
#define SAME_BUFFER_SIZE 270
#define SAME_LOCATION_CODES (31 - 1)  //  Subtract 1, because we count from 0.
//
//  SAME Receive Status Bits.
//
#define PRE 0x01  //  1 = Preamble detected. 
#define HDR 0x02  //  1 = Header detected.
#define MSG 0x04  //  1 = A SAME messsage is available to be displayed.
#define WAT 0x08  //  1 = Warning Alert Tone HIGH during detection.
#define EOM 0x10  //  1 = End Of Message detected.
#define ERR 0x20  //  1 = An Error has occurred.
#define AVL 0x40  //  1 = A validated SAME Message is available to be parsed.
//
//  SAME PLL Definitions.
//
#define SAME_RX_RAMP_LEN 64  
#define SAME_RX_SAMPLES_PER_BIT 8
#define SAME_RAMP_INC (SAME_RX_RAMP_LEN / SAME_RX_SAMPLES_PER_BIT)
#define SAME_RAMP_TRANSITION (SAME_RX_RAMP_LEN / 2)
#define SAME_RAMP_ADJUST 4  
#define SAME_RAMP_INC_RETARD (SAME_RAMP_INC - SAME_RAMP_ADJUST)
#define SAME_RAMP_INC_ADVANCE (SAME_RAMP_INC + SAME_RAMP_ADJUST)
//
//  SAME Receive Status.
//
extern volatile uint8_t SameStatus;
//
//  Global Program Variables.
//
extern uint16_t SameOriginatorCode;
extern uint16_t SameEventCode;
extern uint16_t SamePlusIndex;
extern uint16_t SameLocations;
extern uint32_t SameLocationCodes[];
extern uint16_t SameDuration;
extern uint16_t SameDay;
extern uint16_t SameTime;
//
extern char SameHeader[];
extern char SameOriginatorName[];
extern char SameEventName[];
extern char SameCallSign[];
//
class SameClass 
{
  private:
  
  static volatile uint8_t rxSample;
  static volatile uint8_t rxLastSample;
  static volatile uint8_t rxPllRamp;
  static volatile uint8_t rxIntegrator;

  static volatile uint16_t rxBits;
  static volatile uint16_t rxBitCount;
  static volatile uint8_t  rxSynchronized;

  static char rxBuffer0[];  
  static char rxBuffer1[];
  static uint8_t rxCount[];
  static volatile uint16_t rxBufferIndex;
  static volatile uint16_t rxBufferLength;
  static volatile uint16_t rxBufferPointer;
  
  public:
    
  void begin(void);
  void end(void);
  void pll(void);
  void update_rx_buffers(uint16_t rxBufferIndex, uint8_t rxBits);
  void fill(const String &s);
  int available(void);
  char read(void);
  void parse(void);
  void flush(void);
  void wat(void);
  void reset(void);

};

extern SameClass Same;

#endif

