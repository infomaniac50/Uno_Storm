/*
  SAME_MESSAGES.h 
 
  Arduino program for receiving and displaying messages (in ASCII format) that are
  broadcast using the NOAA Weather Radio (NWR) Specific Area Message Encoding (SAME)
  format.  It is based on National Weather Service Instruction 10-1712, dated
  October 3, 2011.  Event Codes are taken from ANSI/CEA-2009-B, dated November, 2010.
 
  Copyright (C) 2012 by Ray H. Dees
  
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
#ifndef SAME_MESSAGES_h
#define SAME_MESSAGES_h
//
#include <inttypes.h>
#include <avr/pgmspace.h>
//
#define SAME_ORIGINATOR_DISPLAY_LENGTH 40  //  Maximum length of an Originator string.
#define SAME_EVENT_DISPLAY_LENGTH 40       //  Maximum length of an Event string.
#define SAME_WAT_ON_DISPLAY_LENGTH 4       //  Length of the WAT On string.
#define SAME_WAT_OFF_DISPLAY_LENGTH 4      //  Length of the WAT Off string.
//
//  SAME Originator Codes.
//
static const uint16_t SAME_ORIGINATOR_CODES[] PROGMEM =  
{ 
  0x47E6, 0x4963, 0x54A0, 0x5CD2, 0x432F
};
//
//  SAME Originator Strings.
//
static const char SAME_ORIGINATOR_STRING_00[] PROGMEM = "Civil Authorities";
static const char SAME_ORIGINATOR_STRING_01[] PROGMEM = "Emergency Alert System";
static const char SAME_ORIGINATOR_STRING_02[] PROGMEM = "Primary Entry Point";
static const char SAME_ORIGINATOR_STRING_03[] PROGMEM = "National Weather Service";
static const char SAME_ORIGINATOR_STRING_04[] PROGMEM = "Unrecognized Originator";
//
//  SAME Originator String Index.
//
static const char* const SAME_ORIGINATOR_STRING_INDEX[] PROGMEM =  
{
  SAME_ORIGINATOR_STRING_00, SAME_ORIGINATOR_STRING_01, SAME_ORIGINATOR_STRING_02, SAME_ORIGINATOR_STRING_03,
  SAME_ORIGINATOR_STRING_04
};
//
//  SAME Event Codes.
//
static const uint16_t SAME_EVENT_CODES[] PROGMEM =  
{ 
  0x4592, 0x46A1, 0x46B7, 0x46D7, 0x4787, 0x47C7, 0x47F7, 0x4755, 0x4797, 0x479D,
  0x47A1, 0x47B7, 0x47D7, 0x48C7, 0x4861, 0x4877, 0x48A7, 0x491F, 0x4987, 0x495E,
  0x4964, 0x4A67, 0x4AA9, 0x4AA1, 0x4A87, 0x4AA1, 0x4AB3, 0x4AB7, 0x4B01, 0x4B13,
  0x4B17, 0x4B77, 0x4B87, 0x4BF7, 0x4D13, 0x4D27, 0x4D91, 0x4DA7, 0x4DB1, 0x4DC7,
  0x4D77, 0x4DB7, 0x5055, 0x50A7, 0x5187, 0x5264, 0x52D3, 0x531E, 0x5354, 0x5384,
  0x53A7, 0x5543, 0x56D7, 0x5724, 0x57C4, 0x5827, 0x5853, 0x5857, 0x58A1, 0x58B2,
  0x58B3, 0x5931, 0x5935, 0x5942, 0x5961, 0x5977, 0x5971, 0x5987, 0x59C2, 0x59C6,
  0x59CF, 0x59D0, 0x5B47, 0x5BA1, 0x5BB7, 0x5C71, 0x5C87, 0x4331, 0x4335, 0x4343,
  0x4347, 0x432F
};
//
//  SAME Event Strings.
//
static const char SAME_EVENT_STRING_00[] PROGMEM = "Administrative Message";
static const char SAME_EVENT_STRING_01[] PROGMEM = "Avalanche Watch";
static const char SAME_EVENT_STRING_02[] PROGMEM = "Avalanche Warning";
//
static const char SAME_EVENT_STRING_03[] PROGMEM = "Biological Hazard Warning";
static const char SAME_EVENT_STRING_04[] PROGMEM = "Blowing Snow Warning";
static const char SAME_EVENT_STRING_05[] PROGMEM = "Boil Water Warning";
static const char SAME_EVENT_STRING_06[] PROGMEM = "Blizzard Warning";
//
static const char SAME_EVENT_STRING_07[] PROGMEM = "Child Abduction Emergency";
static const char SAME_EVENT_STRING_08[] PROGMEM = "Civil Danger Warning";
static const char SAME_EVENT_STRING_09[] PROGMEM = "Civil Emergency Warning";
static const char SAME_EVENT_STRING_10[] PROGMEM = "Coastal Flood Watch";
static const char SAME_EVENT_STRING_11[] PROGMEM = "Coastal Flood Warning";
static const char SAME_EVENT_STRING_12[] PROGMEM = "Chemical Hazard Warning";
static const char SAME_EVENT_STRING_13[] PROGMEM = "Contaminated Water Warning";
//
static const char SAME_EVENT_STRING_14[] PROGMEM = "Dam Break Watch";
static const char SAME_EVENT_STRING_15[] PROGMEM = "Dam Break Warning";
static const char SAME_EVENT_STRING_16[] PROGMEM = "Contagious Disease Warning";
static const char SAME_EVENT_STRING_17[] PROGMEM = "Practice / Demo";
static const char SAME_EVENT_STRING_18[] PROGMEM = "Dust Storm Warning";
//
static const char SAME_EVENT_STRING_19[] PROGMEM = "Emergency Action Notification";
static const char SAME_EVENT_STRING_20[] PROGMEM = "Emergency Action Termination";
static const char SAME_EVENT_STRING_21[] PROGMEM = "Earthquake Warning";
static const char SAME_EVENT_STRING_22[] PROGMEM = "Immediate Evacuation";
static const char SAME_EVENT_STRING_23[] PROGMEM = "Evacuation Watch";
//
static const char SAME_EVENT_STRING_24[] PROGMEM = "Food Contamination Warning";
static const char SAME_EVENT_STRING_25[] PROGMEM = "Flash Flood Watch";
static const char SAME_EVENT_STRING_26[] PROGMEM = "Flash Flood Statement";
static const char SAME_EVENT_STRING_27[] PROGMEM = "Flash Flood Warning";
static const char SAME_EVENT_STRING_28[] PROGMEM = "Flood Watch";
static const char SAME_EVENT_STRING_29[] PROGMEM = "Flood Statement";
static const char SAME_EVENT_STRING_30[] PROGMEM = "Flood Warning";
static const char SAME_EVENT_STRING_31[] PROGMEM = "Fire Warning";
static const char SAME_EVENT_STRING_32[] PROGMEM = "Flash Freeze Warning";
static const char SAME_EVENT_STRING_33[] PROGMEM = "Freeze Warning";
//
static const char SAME_EVENT_STRING_34[] PROGMEM = "Hurricane Statement";
static const char SAME_EVENT_STRING_35[] PROGMEM = "Hazardous Material Warning";
static const char SAME_EVENT_STRING_36[] PROGMEM = "Hurricane Watch";
static const char SAME_EVENT_STRING_37[] PROGMEM = "Hurricane Warning";
static const char SAME_EVENT_STRING_38[] PROGMEM = "High Wind Watch";
static const char SAME_EVENT_STRING_39[] PROGMEM = "High Wind Warning";
//
static const char SAME_EVENT_STRING_40[] PROGMEM = "Iceberg Warning";
static const char SAME_EVENT_STRING_41[] PROGMEM = "Industrial Fire Warning";
//
static const char SAME_EVENT_STRING_42[] PROGMEM = "Local Area Emergency";
static const char SAME_EVENT_STRING_43[] PROGMEM = "Law Enforcement Warning";
static const char SAME_EVENT_STRING_44[] PROGMEM = "Land Slide Warning";
//
static const char SAME_EVENT_STRING_45[] PROGMEM = "National Audible Test";
static const char SAME_EVENT_STRING_46[] PROGMEM = "National Information Center";
static const char SAME_EVENT_STRING_47[] PROGMEM = "Network Notification Message";
static const char SAME_EVENT_STRING_48[] PROGMEM = "National Periodic Test";
static const char SAME_EVENT_STRING_49[] PROGMEM = "National Silent Test";
static const char SAME_EVENT_STRING_50[] PROGMEM = "Nuclear Power Plant Warning";
//
static const char SAME_EVENT_STRING_51[] PROGMEM = "Power Outage Statement";
//
static const char SAME_EVENT_STRING_52[] PROGMEM = "Radiological Hazard Warning";
static const char SAME_EVENT_STRING_53[] PROGMEM = "Required Monthly Test";
static const char SAME_EVENT_STRING_54[] PROGMEM = "Required Weekly Test";
//
static const char SAME_EVENT_STRING_55[] PROGMEM = "Special Marine Warning";
static const char SAME_EVENT_STRING_56[] PROGMEM = "Special Weather Statement";
static const char SAME_EVENT_STRING_57[] PROGMEM = "Shelter In-Place Warning";
static const char SAME_EVENT_STRING_58[] PROGMEM = "Severe Thunderstorm Watch";
static const char SAME_EVENT_STRING_59[] PROGMEM = "Severe Thunderstorm Warning";
static const char SAME_EVENT_STRING_60[] PROGMEM = "Severe Weather Statement";
//
static const char SAME_EVENT_STRING_61[] PROGMEM = "Tornado Watch";
static const char SAME_EVENT_STRING_62[] PROGMEM = "911 Telephone Outage Emergency";
static const char SAME_EVENT_STRING_63[] PROGMEM = "Tornado Warning";
static const char SAME_EVENT_STRING_64[] PROGMEM = "Tropical Storm Watch";
static const char SAME_EVENT_STRING_65[] PROGMEM = "Tropical Storm Warning";
static const char SAME_EVENT_STRING_66[] PROGMEM = "Tsunami Watch";
static const char SAME_EVENT_STRING_67[] PROGMEM = "Tsunami Warning";
static const char SAME_EVENT_STRING_68[] PROGMEM = "Transmitter Backup On";
static const char SAME_EVENT_STRING_69[] PROGMEM = "Transmitter Carrier Off";
static const char SAME_EVENT_STRING_70[] PROGMEM = "Transmitter Carrier On";
static const char SAME_EVENT_STRING_71[] PROGMEM = "Transmitter Primary On";
//
static const char SAME_EVENT_STRING_72[] PROGMEM = "Volcano Warning";
//
static const char SAME_EVENT_STRING_73[] PROGMEM = "Wild Fire Watch";
static const char SAME_EVENT_STRING_74[] PROGMEM = "Wild Fire Warning";
static const char SAME_EVENT_STRING_75[] PROGMEM = "Winter Storm Watch";
static const char SAME_EVENT_STRING_76[] PROGMEM = "Winter Storm Warning";
//
static const char SAME_EVENT_STRING_77[] PROGMEM = "Unrecognized Watch";
static const char SAME_EVENT_STRING_78[] PROGMEM = "Unrecognized Emergency";
static const char SAME_EVENT_STRING_79[] PROGMEM = "Unrecognized Statement";
static const char SAME_EVENT_STRING_80[] PROGMEM = "Unrecognized Warning";
static const char SAME_EVENT_STRING_81[] PROGMEM = "Unrecognized Event";
//
static const char SAME_EVENT_STRING_82[] PROGMEM = "WAT+";  //  Warning Alert Tone On.
static const char SAME_EVENT_STRING_83[] PROGMEM = "WAT-";  //  Warning Alert Tone Off.
//
//  SAME Event String Index.
//
static const char* const SAME_EVENT_STRING_INDEX[] PROGMEM =
{
  SAME_EVENT_STRING_00, SAME_EVENT_STRING_01, SAME_EVENT_STRING_02, SAME_EVENT_STRING_03, SAME_EVENT_STRING_04, SAME_EVENT_STRING_05,
  SAME_EVENT_STRING_06, SAME_EVENT_STRING_07, SAME_EVENT_STRING_08, SAME_EVENT_STRING_09, SAME_EVENT_STRING_10, SAME_EVENT_STRING_11,
  SAME_EVENT_STRING_12, SAME_EVENT_STRING_13, SAME_EVENT_STRING_14, SAME_EVENT_STRING_15, SAME_EVENT_STRING_16, SAME_EVENT_STRING_17,
  SAME_EVENT_STRING_18, SAME_EVENT_STRING_19, SAME_EVENT_STRING_20, SAME_EVENT_STRING_21, SAME_EVENT_STRING_22, SAME_EVENT_STRING_23,
  SAME_EVENT_STRING_24, SAME_EVENT_STRING_25, SAME_EVENT_STRING_26, SAME_EVENT_STRING_27, SAME_EVENT_STRING_28, SAME_EVENT_STRING_29,
  SAME_EVENT_STRING_30, SAME_EVENT_STRING_31, SAME_EVENT_STRING_32, SAME_EVENT_STRING_33, SAME_EVENT_STRING_34, SAME_EVENT_STRING_35,
  SAME_EVENT_STRING_36, SAME_EVENT_STRING_37, SAME_EVENT_STRING_38, SAME_EVENT_STRING_39, SAME_EVENT_STRING_40, SAME_EVENT_STRING_41,
  SAME_EVENT_STRING_42, SAME_EVENT_STRING_43, SAME_EVENT_STRING_44, SAME_EVENT_STRING_45, SAME_EVENT_STRING_46, SAME_EVENT_STRING_47,
  SAME_EVENT_STRING_48, SAME_EVENT_STRING_49, SAME_EVENT_STRING_50, SAME_EVENT_STRING_51, SAME_EVENT_STRING_52, SAME_EVENT_STRING_53,
  SAME_EVENT_STRING_54, SAME_EVENT_STRING_55, SAME_EVENT_STRING_56, SAME_EVENT_STRING_57, SAME_EVENT_STRING_58, SAME_EVENT_STRING_59,
  SAME_EVENT_STRING_60, SAME_EVENT_STRING_61, SAME_EVENT_STRING_62, SAME_EVENT_STRING_63, SAME_EVENT_STRING_64, SAME_EVENT_STRING_65,
  SAME_EVENT_STRING_66, SAME_EVENT_STRING_67, SAME_EVENT_STRING_68, SAME_EVENT_STRING_69, SAME_EVENT_STRING_70, SAME_EVENT_STRING_71,
  SAME_EVENT_STRING_72, SAME_EVENT_STRING_73, SAME_EVENT_STRING_74, SAME_EVENT_STRING_75, SAME_EVENT_STRING_76, SAME_EVENT_STRING_77,
  SAME_EVENT_STRING_78, SAME_EVENT_STRING_79, SAME_EVENT_STRING_80, SAME_EVENT_STRING_81, SAME_EVENT_STRING_82, SAME_EVENT_STRING_83
};
//
//
//
#endif

