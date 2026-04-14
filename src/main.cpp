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

void showSettingNameNotRecognizedText() {
  Serial.println(F("Setting name not recognized."));
}

//
//  Prints the Function Menu.
//
void showMenu()
{
  Serial.println(F("\n**Radio Commands**"));
  Serial.println(F("h/?:\tDisplay this menu"));
  Serial.println(F("d:\tChannel down"));
  Serial.println(F("u:\tChannel up"));
  Serial.println(F("s:\tScan"));
  Serial.println(F("-:\tVolume -"));
  Serial.println(F("+:\tVolume +"));
  Serial.println(F("m:\tMute / Unmute"));
  Serial.println(F("o:\tOn / Off"));

  Serial.println(F("\n**Lightning Sensor Commands**"));
  Serial.println(F(">:\tSet a setting."));
  Serial.println(F("<:\tGet a setting."));
  Serial.println(F("w:\tSave settings to non-volitile storage."));

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

bool setSetting()
{
  String argName = Serial.readStringUntil(' ');
  String argValue = Serial.readStringUntil('\n');

  argName.trim();
  argValue.trim();

  if (strcasecmp_P(argName.c_str(), PSTR("sensorLocation")) == 0)
  {
    if (strcasecmp_P(argValue.c_str(), PSTR("INDOOR")) == 0)
    {
      sensorSettings.sensorLocation = INDOOR;
    }
    else if (strcasecmp_P(argValue.c_str(), PSTR("OUTDOOR")) == 0)
    {
      sensorSettings.sensorLocation = OUTDOOR;
    }
    else
    {
      Serial.println(F("Invalid argument <value>: You must enter either INDOOR or OUTDOOR."));
      return false;
    }

    SparkFun_AS3935 rawSensor = sensor.getSensor();
    rawSensor.setIndoorOutdoor(sensorSettings.sensorLocation);

    return true;
  }

  if (strcasecmp_P(argName.c_str(), PSTR("tuningCapacitor")) == 0)
  {

    int value = argValue.toInt();
    if (value < 0 || value > 120)
    {
      Serial.println(F("Invalid argument <value>: You must enter a number between 0 and 120."));
      return false;
    }

    if (value % 8 != 0)
    {
      Serial.println(F("Invalid argument <value>: You must enter a number divisible by 8."));
      return false;
    }

    sensorSettings.tuningCapacitor = (uint8_t)value;

    SparkFun_AS3935 rawSensor = sensor.getSensor();
    rawSensor.tuneCap(sensorSettings.tuningCapacitor);

    return true;
  }

  if (strcasecmp_P(argName.c_str(), PSTR("lightningThreshold")) == 0)
  {
    int value = argValue.toInt();
    if (value != 1 && value != 5 && value != 9 && value != 16)
    {
      Serial.println(F("Invalid argument <value>: You must enter 1, 5, 9, or 16."));
      return false;
    }

    sensorSettings.lightningThreshold = (uint8_t)value;

    SparkFun_AS3935 rawSensor = sensor.getSensor();
    rawSensor.lightningThreshold(sensorSettings.lightningThreshold);

    return true;
  }

  if (strcasecmp_P(argName.c_str(), PSTR("watchdogThreshold")) == 0)
  {
    int value = argValue.toInt();
    if (value < 0 || value > 10)
    {
      Serial.println(F("Invalid argument <value>: You must enter a number between 0 and 10."));
      return false;
    }

    sensorSettings.watchdogThreshold = (uint8_t)value;

    SparkFun_AS3935 rawSensor = sensor.getSensor();
    rawSensor.watchdogThreshold(sensorSettings.watchdogThreshold);

    return true;
  }

  if (strcasecmp_P(argName.c_str(), PSTR("noiseFloor")) == 0)
  {
    int value = argValue.toInt();
    if (value < 1 || value > 7)
    {
      Serial.println(F("Invalid argument <value>: You must enter a number between 1 and 7."));
      return false;
    }

    sensorSettings.noiseFloor = (uint8_t)value;

    SparkFun_AS3935 rawSensor = sensor.getSensor();
    rawSensor.setNoiseLevel(sensorSettings.noiseFloor);

    return true;
  }

  if (strcasecmp_P(argName.c_str(), PSTR("spikeRejection")) == 0)
  {
    int value = argValue.toInt();
    if (value < 1 || value > 11)
    {
      Serial.println(F("Invalid argument <value>: You must enter a number between 1 and 11."));
      return false;
    }

    sensorSettings.spikeRejection = (uint8_t)value;

    SparkFun_AS3935 rawSensor = sensor.getSensor();
    rawSensor.spikeRejection(sensorSettings.spikeRejection);

    return true;
  }

  if (strcasecmp_P(argName.c_str(), PSTR("reportDisturber")) == 0)
  {
    int value = argValue.toInt();
    if (value < 0 || value > 1)
    {
      Serial.println(F("The value argument must be 1 or 0, indicating true or false respectively."));
      return false;
    }

    sensorSettings.reportDisturber = (bool)value;

    SparkFun_AS3935 rawSensor = sensor.getSensor();
    rawSensor.maskDisturber(!sensorSettings.reportDisturber);

    return true;
  }

  if (strcasecmp_P(argName.c_str(), PSTR("displayOsc")) == 0)
  {
    int value = EOF;
    int osc = EOF;

    sscanf_P(argValue.c_str(), PSTR("%d %d"), value, osc);

    if (value < 0 || value > 1)
    {
      Serial.println(F("The value argument must be 1 or 0, indicating true or false respectively."));
      return false;
    }

    if (osc < 1 || osc > 3)
    {
      Serial.println(F("The osc argument must be between 1 and 3"));
      return false;
    }

    if (value == 1)
    {
      sensor.detachInterruptPin();
    }

    SparkFun_AS3935 rawSensor = sensor.getSensor();
    rawSensor.displayOscillator((bool)value, osc);

    if (value == 0)
    {
      sensor.attachInterruptPin();
    }

    return true;
  }

  showSettingNameNotRecognizedText();

  return false;
}

bool getSetting()
{
  String argName = Serial.readStringUntil('\n');

  argName.trim();

  if (strcasecmp_P(argName.c_str(), PSTR("sensorLocation")) == 0)
  {
    SparkFun_AS3935 rawSensor = sensor.getSensor();
    sensorSettings.sensorLocation = rawSensor.readIndoorOutdoor();

    if (sensorSettings.sensorLocation == INDOOR)
    {
      Serial.println(F("INDOOR"));
    }
    else if (sensorSettings.sensorLocation == OUTDOOR) {
      Serial.println(F("OUTDOOR"));
    }

    return true;
  }

  if (strcasecmp_P(argName.c_str(), PSTR("tuningCapacitor")) == 0)
  {
    SparkFun_AS3935 rawSensor = sensor.getSensor();
    sensorSettings.tuningCapacitor = rawSensor.readTuneCap();

    Serial.println(sensorSettings.tuningCapacitor);

    return true;
  }

  if (strcasecmp_P(argName.c_str(), PSTR("lightningThreshold")) == 0)
  {
    SparkFun_AS3935 rawSensor = sensor.getSensor();
    sensorSettings.lightningThreshold = rawSensor.readLightningThreshold();

    Serial.println(sensorSettings.lightningThreshold);

    return true;
  }

  if (strcasecmp_P(argName.c_str(), PSTR("watchdogThreshold")) == 0)
  {
    SparkFun_AS3935 rawSensor = sensor.getSensor();
    sensorSettings.watchdogThreshold = rawSensor.readWatchdogThreshold();

    Serial.println(sensorSettings.watchdogThreshold);

    return true;
  }

  if (strcasecmp_P(argName.c_str(), PSTR("noiseFloor")) == 0)
  {
    SparkFun_AS3935 rawSensor = sensor.getSensor();
    sensorSettings.noiseFloor = rawSensor.readNoiseLevel();

    Serial.println(sensorSettings.noiseFloor);

    return true;
  }

  if (strcasecmp_P(argName.c_str(), PSTR("spikeRejection")) == 0)
  {
    SparkFun_AS3935 rawSensor = sensor.getSensor();
    sensorSettings.spikeRejection = rawSensor.readSpikeRejection();

    Serial.println(sensorSettings.spikeRejection);

    return true;
  }

  if (strcasecmp_P(argName.c_str(), PSTR("reportDisturber")) == 0)
  {
    SparkFun_AS3935 rawSensor = sensor.getSensor();

    sensorSettings.reportDisturber = !((bool)rawSensor.readMaskDisturber());

    Serial.println(sensorSettings.reportDisturber, 2);

    return true;
  }

  showSettingNameNotRecognizedText();

  return false;
}

uint8_t lastIntStatus;

void printRssiSnrStatus() {
  Serial.print(F("  RSSI: "));
  Serial.print(rssi);
  Serial.print(F("  SNR: "));
  Serial.println(snr);
}

void printSeekStatus() {
  Serial.print(F("FREQ: "));
  Serial.print(frequency, 3);
  printRssiSnrStatus();
}

void printRsqStatus() {
  Serial.print(F("FREQOFF: "));
  Serial.print(freqoff);
  printRssiSnrStatus();
}
//
//  Status bits are processed here.
//
void getRadioStatus()
{
  Radio.getIntStatus();
  lastIntStatus = intStatus;  

  if (intStatus & SI4707_STCINT)
  {
    Radio.getTuneStatus(SI4707_INTACK);  //  Using SI4707_INTACK clears SI4707_STCINT, SI4707_CHECK preserves it.
    printSeekStatus();
    Radio.sameFlush();             //  This should be done after any tune function.
    //intStatus |= SI4707_RSQINT;         //  We can force it to get rsqStatus on any tune.
  }

  if (intStatus & SI4707_RSQINT)
  {
    Radio.getRsqStatus(SI4707_INTACK);
    printRsqStatus();
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

    if (msgStatus & SI4707_MSGAVL && (!(msgStatus & SI4707_MSGUSD)))  // If a message is available and not already used,
      Radio.sameParse();                                // parse it.

    if (msgStatus & SI4707_MSGPAR)
    {
      msgStatus &= ~SI4707_MSGPAR;                         // Clear the parse status, so that we don't print it again.
      Serial.print(F("Originator: "));
      Serial.println(sameOriginatorName);
      Serial.print(F("Event: "));
      Serial.println(sameEventName);
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

    if (sameWat == asqStatus)
      return;

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

  if (intStatus & SI4707_ERRINT)
  {
    intStatus &= ~SI4707_ERRINT;
    Serial.println(F("An error occured!"));
    Serial.println();
  }
}

//
//  The End.
//

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
    while(1) {
      delay(100);
    }
  }

  Radio.enableInterrupt();

  Serial.println(F("Startup Complete"));
}

void loop()
{
  if (Serial.available() > 0) {
    char c = Serial.read();
    switch (c) {
      case 'h':
      case '?':
        showMenu();
        break;
      case 'w':
        Serial.print(F("Settings "));
        settingsStorage = sensorSettings;
        if (settingsStorage.save()) {
          Serial.println(F("Saved"));
        }
        else {
          Serial.println(F("Save Failed"));
        }
        break;
      case '>':
        setSetting();
        break;
      case '<':
        getSetting();
        break;

      case 'd':
        if (channel <= SI4707_WB_MIN_FREQUENCY)
          break;
        Serial.println(F("Channel down."));
        channel -= SI4707_WB_CHANNEL_SPACING;
        Radio.tune();
        break;

      case 'u':
        if (channel >= SI4707_WB_MAX_FREQUENCY)
          break;
        Serial.println(F("Channel up."));
        channel += SI4707_WB_CHANNEL_SPACING;
        Radio.tune();
        break;
      case 'r':
        Radio.getRsqStatus(SI4707_INTACK);
        printRsqStatus();
        break;
      case 'T':
        printSeekStatus();
        break;
      case 't':
        Serial.println(F("Scanning....."));
        Radio.scan();
        break;

      case '-':
        if (volume <= 0x0000)
          break;
        volume--;
        Radio.setVolume(volume);
        Serial.print(F("Volume: "));
        Serial.println(volume);
        break;

      case '+':
        if (volume >= 0x003F)
          break;
        volume++;
        Radio.setVolume(volume);
        Serial.print(F("Volume: "));
        Serial.println(volume, DEC);
        break;

      case 'm':
        if (mute)
        {
          Radio.setMute(SI4707_OFF);
          Serial.println(F("Mute: Off"));
          break;
        }
        else
        {
          Radio.setMute(SI4707_ON);
          Serial.println(F("Mute: On"));
          break;
        }
      case 's':
        Radio.getSameStatus(0);
        Serial.println(msgStatus, 2);
        break;
      case 'o':
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
        break;
      case 'I':
        getRadioStatus();
      case 'i':
        Serial.println(lastIntStatus, 2);
        break;
      default:
        break;

    }

    Serial.flush();
  }

  checkLightningSensor();
  if (intStatus & SI4707_INTAVL) {
    getRadioStatus();
  }
  taskTimer.tick();

}
