/*
  This example code will walk you through the rest of the functions not
  mentioned in the other example code. This includes different ways to reduce
  false events, how to power down (and what that entails) and wake up your
  board, as well as how to reset all the settings to their factory defaults.

  By: Elias Santistevan
  SparkFun Electronics
  Date: July, 2019
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
*/
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "SparkFun_AS3935.h"
#include "SI4707.h"

#include <LightningSensor.h>
#include <String.h>
#include <arduino-timer.h>
#include <StormFrontDistance.h>
#include <SensorSettings.h>
#include <EepromSecureData.h>
#include <SimpleSerialShell.h>

#define asFlashString(s) (__FlashStringHelper*)(s)

static const char MISSING_ARGUMENT_TEXT[] PROGMEM = "Missing Argument: ";
static const char SETTING_NAME_TEXT[] PROGMEM = "Setting name";
static const char SETTING_VALUE_TEXT[] PROGMEM = "Setting Value";
static const char NOT_RECOGNIZED_TEXT[] PROGMEM = " not recognized";

static const char INVALID_ARGUMENT_VALUE_TEXT[] PROGMEM = "Invalid argument <value>: ";

static const char SENSOR_LOCATION_TEXT[] PROGMEM = "sensorLocation";
static const char INDOOR_TEXT[] PROGMEM = "INDOOR";
static const char OUTDOOR_TEXT[] PROGMEM = "OUTDOOR";

static const char TUNING_CAPACTIOR_TEXT[] PROGMEM = "tuningCapacitor";

static const char YOU_MUST_ENTER_A_NUMBER_TEXT[] PROGMEM = "You must enter a number";

static const char LIGHTNING_THRESHOLD_TEXT[] PROGMEM = "lightningThreshold";
static const char WATCHDOG_THRESHOLD_TEXT[] PROGMEM = "watchdogThreshold";
static const char NOISE_FLOOR_TEXT[] PROGMEM = "noiseFloor";
static const char SPIKE_REJECTION_TEXT[] PROGMEM = "spikeRejection";
static const char REPORT_DISTURBER_TEXT[] PROGMEM = "reportDisturber";

static const char ENTER_NUMBER_TRUE_OR_FALSE_TEXT[] PROGMEM = "You must enter 1 or 0, indicating true or false respectively.";

static const char DISPLAY_OSC_TEXT[] PROGMEM = "displayOsc";

SensorSettings sensorSettings;
EepromSecureData<SensorSettings> settingsStorage(sensorSettings);

LightningSensor sensor;

Timer<2> taskTimer; // create a timer with two tasks

size_t printlnByteBinary(uint8_t n)
{
  char buf[2 + 8 + 1]; // "0b" + 8 bits + '\0'
  char *str = buf;

  *str++ = '0';
  *str++ = 'b';

  for (int i = 7; i >= 0; --i) {
    *str++ = (n & (1 << i)) ? '1' : '0';
  }

  *str = '\0';

  return Serial.println(buf);
}

bool toggleErrorLed(void *)
{
  static bool state = false;

  state = !state;
  digitalWrite(LED_BUILTIN, state);

  return true;
}

void setErrorStatus(bool isErrored = true)
{
  static Timer<>::Task error_task = 0;

  if (isErrored)
  {
    if (error_task == 0)
    {
      error_task = taskTimer.every(500, toggleErrorLed);
    }
  }
  else
  {
    if (error_task != 0)
    {
      taskTimer.cancel(error_task);
      error_task = 0;
    }
  }
}

Timer<>::Task sameFlushTask = 0;

void stopSameFlushTimer() {
  if (sameFlushTask) {
    taskTimer.cancel(sameFlushTask);
  }
  sameFlushTask = 0;
}

bool checkSameFlush(void *) {
  stopSameFlushTimer();
  Radio.sameFlush();

  return true;
}

void startSameFlushTimer() {
  stopSameFlushTimer();
  sameFlushTask = taskTimer.in(SI4707_SAME_TIME_OUT * 1000, checkSameFlush);
}

void checkLightningSensor()
{
  // Run the interrupt check every loop for faster latency.
  // We miss later strikes if more of them happen right after we process the first one.
  if (sensor.isTriggered())
  {
    SensorEvent event;
    sensor.getSensorEvent(&event);

    Serial.print(event.type);
    Serial.print(',');
    Serial.print(event.distance);
    Serial.print(',');
    Serial.print(event.energy);
  }
}

int setSetting(int argc, char **argv)
{
  if (argc <= 0) {
    Serial.print(asFlashString(MISSING_ARGUMENT_TEXT));
    Serial.println(asFlashString(SETTING_NAME_TEXT));

    return EXIT_FAILURE;
  }

  if (argc <= 1) {
    Serial.print(asFlashString(MISSING_ARGUMENT_TEXT));
    Serial.println(asFlashString(SETTING_VALUE_TEXT));

    return EXIT_FAILURE;
  }

  String argName = String(argv[0]);
  String argValue = String(argv[1]);

  argName.trim();
  argValue.trim();

  if (strcasecmp_P(argName.c_str(), SENSOR_LOCATION_TEXT) == 0)
  {
    if (strcasecmp_P(argValue.c_str(), INDOOR_TEXT) == 0)
    {
      sensorSettings.sensorLocation = INDOOR;
    }
    else if (strcasecmp_P(argValue.c_str(), OUTDOOR_TEXT) == 0)
    {
      sensorSettings.sensorLocation = OUTDOOR;
    }
    else
    {
      Serial.print(asFlashString(INVALID_ARGUMENT_VALUE_TEXT));
      Serial.println(F("You must enter either INDOOR or OUTDOOR."));

      return EXIT_FAILURE;
    }

    SparkFun_AS3935 rawSensor = sensor.getSensor();
    rawSensor.setIndoorOutdoor(sensorSettings.sensorLocation);

    return EXIT_SUCCESS;
  }

  if (strcasecmp_P(argName.c_str(), TUNING_CAPACTIOR_TEXT) == 0)
  {

    int value = argValue.toInt();
    if (value < 0 || value > 120)
    {
      Serial.print(asFlashString(INVALID_ARGUMENT_VALUE_TEXT));
      Serial.print(asFlashString(YOU_MUST_ENTER_A_NUMBER_TEXT));
      Serial.println(F(" between 0 and 120."));

      return EXIT_FAILURE;
    }

    if (value % 8 != 0)
    {
      Serial.print(asFlashString(INVALID_ARGUMENT_VALUE_TEXT));
      Serial.print(asFlashString(YOU_MUST_ENTER_A_NUMBER_TEXT));
      Serial.println(F(" divisible by 8."));

      return EXIT_FAILURE;
    }

    sensorSettings.tuningCapacitor = (uint8_t)value;

    SparkFun_AS3935 rawSensor = sensor.getSensor();
    rawSensor.tuneCap(sensorSettings.tuningCapacitor);

    return EXIT_SUCCESS;
  }

  if (strcasecmp_P(argName.c_str(), LIGHTNING_THRESHOLD_TEXT) == 0)
  {
    int value = argValue.toInt();
    if (value != 1 && value != 5 && value != 9 && value != 16)
    {
      Serial.print(asFlashString(INVALID_ARGUMENT_VALUE_TEXT));
      Serial.println(F("You must enter 1, 5, 9, or 16."));

      return EXIT_FAILURE;
    }

    sensorSettings.lightningThreshold = (uint8_t)value;

    SparkFun_AS3935 rawSensor = sensor.getSensor();
    rawSensor.lightningThreshold(sensorSettings.lightningThreshold);

    return EXIT_SUCCESS;
  }

  if (strcasecmp_P(argName.c_str(), WATCHDOG_THRESHOLD_TEXT) == 0)
  {
    int value = argValue.toInt();
    if (value < 0 || value > 10)
    {
      Serial.print(asFlashString(INVALID_ARGUMENT_VALUE_TEXT));
      Serial.print(asFlashString(YOU_MUST_ENTER_A_NUMBER_TEXT));
      Serial.println(F(" between 0 and 10."));

      return EXIT_FAILURE;
    }

    sensorSettings.watchdogThreshold = (uint8_t)value;

    SparkFun_AS3935 rawSensor = sensor.getSensor();
    rawSensor.watchdogThreshold(sensorSettings.watchdogThreshold);

    return EXIT_SUCCESS;
  }

  if (strcasecmp_P(argName.c_str(), NOISE_FLOOR_TEXT) == 0)
  {
    int value = argValue.toInt();
    if (value < 1 || value > 7)
    {
      Serial.print(asFlashString(INVALID_ARGUMENT_VALUE_TEXT));
      Serial.print(asFlashString(YOU_MUST_ENTER_A_NUMBER_TEXT));
      Serial.println(F(" between 1 and 7."));

      return EXIT_FAILURE;
    }

    sensorSettings.noiseFloor = (uint8_t)value;

    SparkFun_AS3935 rawSensor = sensor.getSensor();
    rawSensor.setNoiseLevel(sensorSettings.noiseFloor);

    return EXIT_SUCCESS;
  }

  if (strcasecmp_P(argName.c_str(), SPIKE_REJECTION_TEXT) == 0)
  {
    int value = argValue.toInt();
    if (value < 1 || value > 11)
    {
      Serial.print(asFlashString(INVALID_ARGUMENT_VALUE_TEXT));
      Serial.print(asFlashString(YOU_MUST_ENTER_A_NUMBER_TEXT));
      Serial.println(F(" between 1 and 11."));

      return EXIT_FAILURE;
    }

    sensorSettings.spikeRejection = (uint8_t)value;

    SparkFun_AS3935 rawSensor = sensor.getSensor();
    rawSensor.spikeRejection(sensorSettings.spikeRejection);

    return EXIT_SUCCESS;
  }

  if (strcasecmp_P(argName.c_str(), REPORT_DISTURBER_TEXT) == 0)
  {
    int value = argValue.toInt();
    if (value < 0 || value > 1)
    {
      Serial.print(asFlashString(INVALID_ARGUMENT_VALUE_TEXT));
      Serial.println(asFlashString(ENTER_NUMBER_TRUE_OR_FALSE_TEXT));
      return false;
    }

    sensorSettings.reportDisturber = (bool)value;

    SparkFun_AS3935 rawSensor = sensor.getSensor();
    rawSensor.maskDisturber(!sensorSettings.reportDisturber);

    return EXIT_SUCCESS;
  }

  if (strcasecmp_P(argName.c_str(), DISPLAY_OSC_TEXT) == 0)
  {
    int mode = EOF;
    int osc = EOF;

    if (argc < 3) {
      Serial.print(asFlashString(MISSING_ARGUMENT_TEXT));
      Serial.println(asFlashString(SETTING_VALUE_TEXT));

      return EXIT_FAILURE;
    }

    String argOsc = String(argv[2]);
    argOsc.trim();

    if (mode < 0 || mode > 1)
    {
      Serial.print(asFlashString(INVALID_ARGUMENT_VALUE_TEXT));
      Serial.println(asFlashString(ENTER_NUMBER_TRUE_OR_FALSE_TEXT));

      return EXIT_FAILURE;
    }

    if (osc < 1 || osc > 3)
    {
      Serial.print(asFlashString(INVALID_ARGUMENT_VALUE_TEXT));
      Serial.println(F("The osc argument must be between 1 and 3"));

      return EXIT_FAILURE;
    }

    if (mode == 1)
    {
      sensor.detachInterruptPin();
    }

    SparkFun_AS3935 rawSensor = sensor.getSensor();
    rawSensor.displayOscillator((bool)mode, osc);

    if (mode == 0)
    {
      sensor.attachInterruptPin();
    }

    return EXIT_SUCCESS;
  }

  Serial.print(asFlashString(SETTING_NAME_TEXT));
  Serial.println(asFlashString(NOT_RECOGNIZED_TEXT));

  return EXIT_FAILURE;
}

int getSetting(int argc, char **argv)
{
  if (argc <= 0) {
    Serial.print(asFlashString(MISSING_ARGUMENT_TEXT));
    Serial.println(asFlashString(SETTING_NAME_TEXT));

    return EXIT_FAILURE;
  }

  String argName = String(argv[0]);

  argName.trim();

  if (strcasecmp_P(argName.c_str(), SENSOR_LOCATION_TEXT) == 0)
  {
    SparkFun_AS3935 rawSensor = sensor.getSensor();
    sensorSettings.sensorLocation = rawSensor.readIndoorOutdoor();

    if (sensorSettings.sensorLocation == INDOOR)
    {
      Serial.println(asFlashString(INDOOR_TEXT));
    }
    else if (sensorSettings.sensorLocation == OUTDOOR) {
      Serial.println(asFlashString(OUTDOOR_TEXT));
    }

    return EXIT_SUCCESS;
  }

  if (strcasecmp_P(argName.c_str(), TUNING_CAPACTIOR_TEXT) == 0)
  {
    SparkFun_AS3935 rawSensor = sensor.getSensor();
    sensorSettings.tuningCapacitor = rawSensor.readTuneCap();

    Serial.println(sensorSettings.tuningCapacitor);

    return EXIT_SUCCESS;
  }

  if (strcasecmp_P(argName.c_str(), LIGHTNING_THRESHOLD_TEXT) == 0)
  {
    SparkFun_AS3935 rawSensor = sensor.getSensor();
    sensorSettings.lightningThreshold = rawSensor.readLightningThreshold();

    Serial.println(sensorSettings.lightningThreshold);

    return EXIT_SUCCESS;
  }

  if (strcasecmp_P(argName.c_str(), WATCHDOG_THRESHOLD_TEXT) == 0)
  {
    SparkFun_AS3935 rawSensor = sensor.getSensor();
    sensorSettings.watchdogThreshold = rawSensor.readWatchdogThreshold();

    Serial.println(sensorSettings.watchdogThreshold);

    return EXIT_SUCCESS;
  }

  if (strcasecmp_P(argName.c_str(), NOISE_FLOOR_TEXT) == 0)
  {
    SparkFun_AS3935 rawSensor = sensor.getSensor();
    sensorSettings.noiseFloor = rawSensor.readNoiseLevel();

    Serial.println(sensorSettings.noiseFloor);

    return EXIT_SUCCESS;
  }

  if (strcasecmp_P(argName.c_str(), SPIKE_REJECTION_TEXT) == 0)
  {
    SparkFun_AS3935 rawSensor = sensor.getSensor();
    sensorSettings.spikeRejection = rawSensor.readSpikeRejection();

    Serial.println(sensorSettings.spikeRejection);

    return EXIT_SUCCESS;
  }

  if (strcasecmp_P(argName.c_str(), REPORT_DISTURBER_TEXT) == 0)
  {
    SparkFun_AS3935 rawSensor = sensor.getSensor();

    sensorSettings.reportDisturber = !((bool)rawSensor.readMaskDisturber());

    printlnByteBinary(sensorSettings.reportDisturber);

    return EXIT_SUCCESS;
  }

  Serial.print(asFlashString(SETTING_NAME_TEXT));
  Serial.println(asFlashString(NOT_RECOGNIZED_TEXT));

  return EXIT_FAILURE;
}

int saveSettings(int /*argc*/ = 0, char** /*argv*/ = NULL)
{
  settingsStorage = sensorSettings;

  return settingsStorage.save() ? EXIT_SUCCESS : EXIT_FAILURE;
}

uint8_t lastIntStatus;

void radioRssiSnrStatus() {
  Serial.print(F("  RSSI: "));
  Serial.print(rssi);
  Serial.print(F("  SNR: "));
  Serial.println(snr);
}

int radioSeekStatus(int /*argc*/ = 0, char** /*argv*/ = NULL) {
  Serial.print(F("FREQ: "));
  Serial.print(frequency, 3);
  radioRssiSnrStatus();

  return EXIT_SUCCESS;
}

int radioRsqStatus(int /*argc*/ = 0, char** /*argv*/ = NULL) {
  Radio.getRsqStatus(SI4707_INTACK);

  Serial.print(F("FREQOFF: "));
  Serial.print(freqoff);
  radioRssiSnrStatus();

  return EXIT_SUCCESS;
}

int radioScan(int /*argc*/ = 0, char** /*argv*/ = NULL) {
  Serial.println(F("Scanning....."));
  Radio.scan();

  return EXIT_SUCCESS;
}

int radioLastStatus(int /*argc*/ = 0, char** /*argv*/ = NULL) {
  printlnByteBinary(lastIntStatus);

  return EXIT_SUCCESS;
}

//
//  Status bits are processed here.
//
int radioStatus(int /*argc*/ = 0, char** /*argv*/ = NULL)
{
  Radio.getIntStatus();
  lastIntStatus = intStatus;

  if (intStatus & SI4707_STCINT)
  {
    Radio.getTuneStatus(SI4707_INTACK);  //  Using SI4707_INTACK clears SI4707_STCINT, SI4707_CHECK preserves it.
    radioSeekStatus();
    Radio.sameFlush();             //  This should be done after any tune function.
    //intStatus |= SI4707_RSQINT;         //  We can force it to get rsqStatus on any tune.
  }

  if (intStatus & SI4707_RSQINT)
  {
    radioRsqStatus();
  }

  if (intStatus & SI4707_SAMEINT)
  {
    Radio.getSameStatus(SI4707_INTACK);

    printlnByteBinary(intStatus);
    printlnByteBinary(msgStatus);
    printlnByteBinary(sameStatus);
    printlnByteBinary(sameState);
    Serial.println(sameLength);
    Serial.println(sameHeaderCount);

    // Response
    // | Bit    | D7  | D6  | D5 | D4 | D3     | D2      | D1     | D0     |
    // |--------|-----|-----|----|----|--------|---------|--------|--------|
    // | STATUS | CTS | ERR | X  | X  | RSQINT | SAMEINT | ASQINT | STCINT |
    // | RESP1  | X   | X   | X  | X  | EOMDET | SOMDET  | PREDET | HDRRDY |

    // RESP2 STATE[7:0]
    // State Machine Status
    // 0 = End of message.
    // 1 = Preamble detected.
    // 2 = Receiving SAME header message.
    // 3 = SAME header message complete.
    if (sameStatus & SI4707_EOMDET)
    {
      Radio.sameFlush();
      stopSameFlushTimer();
      Serial.println(F("EOM detected."));
      //  More application specific code could go here. (Mute audio, turn something on/off, etc.)
    }

    if (sameStatus & SI4707_PREDET)
    {
      startSameFlushTimer();
      Serial.println(F("Preamble detected."));
    }

    // If a message is available and not already used,
    if (msgStatus & SI4707_MSGAVL && (!(msgStatus & SI4707_MSGUSD)))
    {
      // parse it.
      Radio.sameParse();
    }

    if (msgStatus & SI4707_MSGPAR)
    {
      msgStatus &= ~SI4707_MSGPAR;                         // Clear the parse status, so that we don't print it again.
      Serial.print(F("Originator: "));
      Serial.println(sameOriginatorName);
      Serial.print(F("Event: "));
      Serial.println(sameEventName);
      // | Event Code | Event Description                                           | Event Level |
      // |------------|-------------------------------------------------------------|-------------|
      // | ADR        | Administrative Message                                      | ADV         |
      // | AVA        | Avalanche Watch                                             | WCH         |
      // | AVW        | Avalanche Warning                                           | WRN         |
      // | BLU        | Blue Alert                                                  | WRN         |
      // | BZW        | Blizzard Warning                                            | WRN         |
      // | CAE        | Child Abduction Emergency                                   | ADV         |
      // | CDW        | Civil Danger Warning                                        | WRN         |
      // | CEM        | Civil Emergency Message                                     | WRN         |
      // | CFA        | Coastal Flood Watch                                         | WCH         |
      // | CFW        | Coastal Flood Warning                                       | WRN         |
      // | DMO        | Practice/Demo Warning                                       | TEST        |
      // | DSW        | Dust Storm Warning                                          | WRN         |
      // | EAN        | National Emergency Message                                  | WRN         |
      // | EAT        | Emergency Action Termination                                | ADV         |
      // | EQW        | Earthquake Warning                                          | WRN         |
      // | EVI        | Evacuation Immediate                                        | WRN         |
      // | EWW        | Extreme Wind Warning                                        | WRN         |
      // | FFA        | Flash Flood Watch                                           | WCH         |
      // | FFS        | Flash Flood Statement                                       | ADV         |
      // | FFW        | Flash Flood Warning                                         | WRN         |
      // | FLA        | Flood Watch                                                 | WCH         |
      // | FLS        | Flood Statement                                             | ADV         |
      // | FLW        | Flood Warning                                               | WRN         |
      // | FRW        | Fire Warning                                                | WRN         |
      // | FSW        | Flash Freeze Warning                                        | WRN         |
      // | FZW        | Freeze Warning (also known as a "Frost Warning" in Canada.) | WRN         |
      // | HLS        | Hurricane Local Statement                                   | ADV         |
      // | HMW        | Hazardous Materials Warning                                 | WRN         |
      // | HUA        | Hurricane Watch                                             | WCH         |
      // | HUW        | Hurricane Warning                                           | WRN         |
      // | HWA        | High Wind Watch                                             | WCH         |
      // | HWW        | High Wind Warning                                           | WRN         |
      // | LAE        | Local Area Emergency                                        | ADV         |
      // | LEW        | Law Enforcement Warning                                     | WRN         |
      // | MEP        | Missing and Endangered Persons                              | ADV         |
      // | NAT        | National Audible Test                                       | TEST        |
      // | NIC        | National Information Center                                 | ADV         |
      // | NMN        | Network Notification Message                                | ADV         |
      // | NPT        | Nationwide Test of the Emergency Alert System               | TEST        |
      // | NST        | National Silent Test                                        | TEST        |
      // | NUW        | Nuclear Power Plant Warning                                 | WRN         |
      // | RHW        | Radiological Hazard Warning                                 | WRN         |
      // | RMT        | Required Monthly Test                                       | TEST        |
      // | RWT        | Required Weekly Test                                        | TEST        |
      // | SMW        | Special Marine Warning                                      | WRN         |
      // | SPS        | Special Weather Statement                                   | ADV         |
      // | SPW        | Shelter In-Place warning                                    | WRN         |
      // | SQW        | Snow Squall Warning                                         | WRN         |
      // | SSA        | Storm Surge Watch                                           | WCH         |
      // | SSW        | Storm Surge Warning                                         | WRN         |
      // | SVA        | Severe Thunderstorm Watch                                   | WCH         |
      // | SVR        | Severe Thunderstorm Warning                                 | WRN         |
      // | SVS        | Severe Weather Statement (U.S., CAN)                        | ADV         |
      // | TOA        | Tornado Watch                                               | WCH         |
      // | TOE        | 911 Telephone Outage Emergency                              | ADV         |
      // | TOR        | Tornado Warning/Emergency                                   | WRN         |
      // | TRA        | Tropical Storm Watch                                        | WCH         |
      // | TRW        | Tropical Storm Warning                                      | WRN         |
      // | TSA        | Tsunami Watch                                               | WCH         |
      // | TSW        | Tsunami Warning                                             | WRN         |
      // | VOW        | Volcano Warning                                             | WRN         |
      // | WSA        | Winter Storm Watch                                          | WCH         |
      // | WSW        | Winter Storm Warning                                        | WRN         |
      // | ??A        | Unrecognized Watch                                          | WCH         |
      // | ??E        | Unrecognized Emergency                                      | ADV         |
      // | ??S        | Unrecognized Statement                                      | ADV         |
      // | ??W        | Unrecognized Warning                                        | WRN         |
      Serial.print(F("Locations: "));
      Serial.println(sameLocations);
      Serial.print(F("Location Codes: "));

      for (int i = 0; i < sameLocations; i++)
      {
        Serial.print(sameLocationCodes[i]);
        Serial.print(' ');
      }

      Serial.println();
      Serial.print(F("Duration: "));
      Serial.println(sameDuration);
      Serial.print(F("Day: "));
      Serial.println(sameDay);
      Serial.print(F("Time: "));
      if (sameHour < 10) {
        Serial.print('0');
      }
      Serial.print(sameHour);
      Serial.print(':');
      if (sameMinute < 10) {
        Serial.print('0');
      }
      Serial.println(sameMinute);
      Serial.print(F("Callsign: "));
      Serial.println(sameCallSign);
      Serial.println();
    }

    if (msgStatus & SI4707_MSGPUR)   //  Signals that the third header has been received.
    {
      Serial.println(F("Third Header Received"));
      stopSameFlushTimer();
      Radio.sameFlush();
    }
  }

  if (intStatus & SI4707_ASQINT)
  {
    Radio.getAsqStatus(SI4707_INTACK);

    if (sameWat != asqStatus) {
      if (asqStatus == 0x01)
      {
        // New Alert Tone
        Radio.setProperty(SI4707_WB_ASQ_INT_SOURCE, (SI4707_ALERTOFIEN));
        // SAME is done by now.
        stopSameFlushTimer();
        Radio.sameFlush();

        Serial.println(F("WAT is on."));
        Serial.println();
        //  More application specific code could go here.  (Unmute audio, turn something on/off, etc.)
      }

      if (asqStatus == 0x02)
      {
        Radio.setProperty(SI4707_WB_ASQ_INT_SOURCE, (SI4707_ALERTONIEN));
        Serial.println(F("WAT is off."));
        Serial.println();
        //  More application specific code could go here.  (Mute audio, turn something on/off, etc.)
      }

      sameWat = asqStatus;
    }
  }

  if (intStatus & SI4707_ERRINT)
  {
    intStatus &= ~SI4707_ERRINT;
    Serial.println(F("An error occured!"));
    Serial.println();
  }

  return EXIT_SUCCESS;
}

int radioChannelDown(int /*argc*/ = 0, char** /*argv*/ = NULL) {
  if (channel <= SI4707_WB_MIN_FREQUENCY) {
    return EXIT_FAILURE;
  }
  Serial.println(F("Channel down."));
  channel -= SI4707_WB_CHANNEL_SPACING;
  Radio.tune();

  return EXIT_SUCCESS;
}

int radioChannelUp(int /*argc*/ = 0, char** /*argv*/ = NULL) {
  if (channel >= SI4707_WB_MAX_FREQUENCY) {
    return EXIT_FAILURE;
  }
  Serial.println(F("Channel up."));
  channel += SI4707_WB_CHANNEL_SPACING;
  Radio.tune();

  return EXIT_SUCCESS;
}

int radioVolume(int argc, char **argv) {
  if (argc <= 0) {
    Serial.print(asFlashString(MISSING_ARGUMENT_TEXT));
    Serial.println(asFlashString(SETTING_VALUE_TEXT));

    return EXIT_FAILURE;
  }

  String argValue = String(argv[0]);

  argValue.trim();

  Radio.setVolume(argValue.toInt());
  Serial.print(F("Volume: "));
  Serial.println(volume);

  return EXIT_SUCCESS;
}

int radioMute(int /*argc*/ = 0, char** /*argv*/ = NULL) {
  if (mute)
  {
    Radio.setMute(SI4707_OFF);
    Serial.println(F("Mute: Off"));
  }
  else
  {
    Radio.setMute(SI4707_ON);
    Serial.println(F("Mute: On"));
  }

  return EXIT_SUCCESS;
}

int radioSameStatus(int /*argc*/ = 0, char** /*argv*/ = NULL) {
  Radio.getSameStatus(SI4707_CHECK);
  printlnByteBinary(msgStatus);

  return EXIT_SUCCESS;
}

int radioPower(int /*argc*/ = 0, char** /*argv*/ = NULL) {
  if (power)
  {
    Radio.disableInterrupt();
    Radio.off();
    Serial.println(F("Radio powered off."));
  }
  else
  {
    Radio.on();
    Radio.enableInterrupt();
    Serial.println(F("Radio powered on."));
    Radio.tune();
  }

  return EXIT_SUCCESS;
}

//
//  The End.
//

//
//  Prints the Function Menu.
//
int showMenu(int /*argc*/ = 0, char** /*argv*/ = NULL)
{
  shell.printHelp(0, NULL);

  Serial.println(F("\n**Lightning Sensor Settings**"));
  Serial.println(F("sensorLocation <value> Must be a string equal to \"INDOOR\" or \"OUTDOOR\"."));
  Serial.println(F("tuningCapacitor <value> Must be a number between 0 and 120 and divisible by 8."));
  Serial.println(F("lightningThreshold <value> Must be 1, 5, 9, or 16."));
  Serial.println(F("watchdogThreshold <value> Must be a number between 0 and 10."));
  Serial.println(F("noiseFloor <value> Must be a number between 1 and 7."));
  Serial.println(F("spikeRejection <value> Must be a number between 1 and 11."));
  Serial.println(F("reportDisturber <value> Must be 1 or 0, indicating true or false respectively."));
  Serial.println(F("displayOsc <mode> <osc>"));
  Serial.println(F("\tThis setting can only be written to."));
  Serial.println(F("\tThe mode argument must be 1 or 0, indicating true or false respectively."));
  Serial.println(F("\tThe osc argument must be between 1 and 3."));
  Serial.println();

  return EXIT_SUCCESS;
}

void setup()
{
  // Initialize serial and wait for port to open:
  Serial.begin(115200);

  Serial.println(F("Uno Storm"));
  settingsStorage.load();
  sensorSettings = settingsStorage;

  Serial.print(F("Starting Radio"));

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.print('.');

  Radio.begin();
  Serial.print('.');
  Radio.patch();          //  Use this one to to include the 1050 Hz patch.
  Serial.print('.');
  // Radio.on();           //  Use this one if not using the patch.
  //Radio.getRevision();  //  Only captured on the logic analyzer - not displayed.
  //
  //  All useful interrupts are enabled here.
  //
  Radio.setProperty(SI4707_GPO_IEN, (SI4707_ERRIEN | SI4707_RSQIEN | SI4707_SAMEIEN | SI4707_ASQIEN | SI4707_STCIEN));
  Serial.print('.');
  //
  //  RSQ Interrupt Sources.
  //
  // Radio.setProperty(SI4707_WB_RSQ_SNR_HIGH_THRESHOLD, 0x007F);   // 127 dBuV for testing..want it high
  // Radio.setProperty(SI4707_WB_RSQ_SNR_LOW_THRESHOLD, 0x0001);    // 1 dBuV for testing
  // Radio.setProperty(SI4707_WB_RSQ_RSSI_HIGH_THRESHOLD, 0x004D);  // -30 dBm for testing
  // Radio.setProperty(SI4707_WB_RSQ_RSSI_LOW_THRESHOLD, 0x0007);   // -100 dBm for testing
  //Radio.setProperty(SI4707_WB_RSQ_INT_SOURCE, (SI4707_SNRHIEN | SI4707_SNRLIEN | SI4707_RSSIHIEN | SI4707_RSSILIEN));
  //
  //  SAME Interrupt Sources.
  //
  Radio.setProperty(SI4707_WB_SAME_INTERRUPT_SOURCE, (SI4707_EOMDETIEN | SI4707_HDRRDYIEN));
  Serial.print('.');

  //
  //  ASQ Interrupt Sources.
  //
  Radio.setProperty(SI4707_WB_ASQ_INT_SOURCE, (SI4707_ALERTONIEN));
  Serial.print('.');
  //
  //  Tune to the desired frequency.
  //
  Radio.tune(162550);  //  6 digits only.

  Serial.println(F("Done"));

  Serial.println(F("Starting Lightning Sensor"));

  if (sensor.begin(sensorSettings) < 0) {
    Serial.println(F("Failed"));
    while (1) {
      delay(100);
    }
  }

  Radio.enableInterrupt();

  shell.attach(Serial);
  shell.addCommand(F("save Save settings to non-volatile storage."), saveSettings);
  shell.addCommand(F("get <name> Get a setting."), getSetting);
  shell.addCommand(F("set <name> <value> Set a setting."), setSetting);
  shell.addCommand(F("help Display this menu"), showMenu);

  shell.addCommand(F("radioChannelDown Channel down"), radioChannelDown);
  shell.addCommand(F("radioChannelUp Channel up"), radioChannelUp);

  shell.addCommand(F("radioScan Scan for best signal"), radioScan);

  shell.addCommand(F("radioVolume <value> Must be a number between 0 and 63."), radioVolume);
  shell.addCommand(F("radioMute Toggle Hard Mute"), radioMute);

  shell.addCommand(F("radioPower Toggle Radio Power"), radioPower);

  shell.addCommand(F("radioRsqStatus Print signal strength"), radioRsqStatus);
  shell.addCommand(F("radioSeekStatus Print last tune status"), radioSeekStatus);
  shell.addCommand(F("radioSameStatus Print SAME message status"), radioSameStatus);
  shell.addCommand(F("radioStatus Check interrupt status now"), radioStatus);
  shell.addCommand(F("radioLastStatus Print last interrupt status"), radioLastStatus);

  Serial.println(F("Startup Complete"));
}

void loop()
{
  shell.executeIfInput();

  checkLightningSensor();
  if (intStatus & SI4707_INTAVL) {
    radioStatus();
  }
  taskTimer.tick();

}
