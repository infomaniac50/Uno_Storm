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
#include "SI4707.h"
#include "SparkFun_AS3935.h"
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include <EepromSecureData.h>
#include <LightningSensor.h>
#include <PacketTypes.h>
#include <SensorSettings.h>
#include <StormFrontDistance.h>
#include <arduino-timer.h>

#ifdef OPERATOR_MODE
#include <SimpleSerialShell.h>
#include <String.h>

#define asFlashString(s) (__FlashStringHelper *)(s)

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
#endif

#define ASSERT(x)                                                                                                                                                                  \
  do                                                                                                                                                                               \
  {                                                                                                                                                                                \
    if (!(x))                                                                                                                                                                      \
    {                                                                                                                                                                              \
      setErrorStatus(true);                                                                                                                                                        \
      while (1)                                                                                                                                                                    \
        taskTimer.tick();                                                                                                                                                          \
    }                                                                                                                                                                              \
  } while (0)

SensorSettings sensorSettings;
EepromSecureData<SensorSettings> settingsStorage(sensorSettings);

LightningSensor sensor;

Timer<2> taskTimer; // create a timer with two tasks

#ifdef OPERATOR_MODE
size_t printlnByteBinary(uint8_t n)
{
  char buf[2 + 8 + 1]; // "0b" + 8 bits + '\0'
  char *str = buf;

  *str++ = '0';
  *str++ = 'b';

  for (int i = 7; i >= 0; --i)
  {
    *str++ = (n & (1 << i)) ? '1' : '0';
  }

  *str = '\0';

  return Serial.println(buf);
}
#endif

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

void stopSameFlushTimer()
{
  if (sameFlushTask)
  {
    taskTimer.cancel(sameFlushTask);
  }
  sameFlushTask = 0;
}

bool checkSameFlush(void *)
{
  stopSameFlushTimer();
  Radio.sameFlush();

  return true;
}

void startSameFlushTimer()
{
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

    if (event.type == LIGHTNING)
    {
#ifdef OPERATOR_MODE
      Serial.print(F("Lightning "));
      Serial.print(distanceToString(event.distance));
      Serial.print(' ');
      Serial.println(event.energy);
#else
      LightningPacket payload = {
          .interruptType = event.type,
          .distance = event.distance,
          .energy = event.energy,
      };
      UnoStormPacketHeader header = {
          .packetType = PacketType::LIGHTNING_PACKET,
          .payloadSize = sizeof(payload),
      };
      UnoStormPacket packet = {
          .header = header,
          .payload = reinterpret_cast<const uint8_t *>(&payload),
      };
      sendPacket(&packet);
#endif
    }
  }
}

#ifdef OPERATOR_MODE
int setSetting(int argc, char **argv)
{
  if (argc <= 1)
  {
    Serial.print(asFlashString(MISSING_ARGUMENT_TEXT));
    Serial.println(asFlashString(SETTING_NAME_TEXT));

    return EXIT_FAILURE;
  }

  if (argc <= 2)
  {
    Serial.print(asFlashString(MISSING_ARGUMENT_TEXT));
    Serial.println(asFlashString(SETTING_VALUE_TEXT));

    return EXIT_FAILURE;
  }

  String argName = String(argv[1]);
  String argValue = String(argv[2]);

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
    // set displayOsc <mode> <osc>
    if (argc < 4)
    {
      Serial.print(asFlashString(MISSING_ARGUMENT_TEXT));
      Serial.println(asFlashString(SETTING_VALUE_TEXT));

      return EXIT_FAILURE;
    }

    String argMode = String(argv[2]);
    String argOsc = String(argv[3]);
    argMode.trim();
    argOsc.trim();

    mode = argMode.toInt();
    if (mode < 0 || mode > 1)
    {
      Serial.print(asFlashString(INVALID_ARGUMENT_VALUE_TEXT));
      Serial.println(asFlashString(ENTER_NUMBER_TRUE_OR_FALSE_TEXT));

      return EXIT_FAILURE;
    }

    osc = argOsc.toInt();
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

    // This will send the frequency of the oscillators to the IRQ pin.
    //  osc = 1 = TRCO - System RCO at 32.768kHz
    //  osc = 2 = SRCO - Timer RCO Oscillators 1.1MHz
    //  osc = 3 = LCO  - Frequency of the Antenna
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
  if (argc <= 1)
  {
    Serial.print(asFlashString(MISSING_ARGUMENT_TEXT));
    Serial.println(asFlashString(SETTING_NAME_TEXT));

    return EXIT_FAILURE;
  }

  String argName = String(argv[1]);

  argName.trim();

  if (strcasecmp_P(argName.c_str(), SENSOR_LOCATION_TEXT) == 0)
  {
    SparkFun_AS3935 rawSensor = sensor.getSensor();
    sensorSettings.sensorLocation = rawSensor.readIndoorOutdoor();

    if (sensorSettings.sensorLocation == INDOOR)
    {
      Serial.println(asFlashString(INDOOR_TEXT));
    }
    else if (sensorSettings.sensorLocation == OUTDOOR)
    {
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

int saveSettings(int /*argc*/ = 0, char ** /*argv*/ = NULL)
{
  settingsStorage = sensorSettings;

  return settingsStorage.save() ? EXIT_SUCCESS : EXIT_FAILURE;
}
#endif

uint8_t lastIntStatus;

#ifdef OPERATOR_MODE
void radioRssiSnrStatus()
{
  Serial.print(F("  RSSI: "));
  Serial.print(rssi);
  Serial.print(F("  SNR: "));
  Serial.println(snr);
}

int radioSeekStatus(int /*argc*/ = 0, char ** /*argv*/ = NULL)
{
  Serial.print(F("FREQ: "));
  Serial.print(frequency, 3);
  radioRssiSnrStatus();

  return EXIT_SUCCESS;
}

int radioRsqStatus(int /*argc*/ = 0, char ** /*argv*/ = NULL)
{
  Radio.getRsqStatus(SI4707_INTACK);

  // Might be some kind of tuning error metric.
  // As in the PLL locked to a carrier that's a little off from what we asked for.
  // FREQOFF[7:0] Frequency Offset.
  // Signed frequency offset in kHz.
  Serial.print(F("FREQOFF: "));
  Serial.print((long)freqoff, 10);
  radioRssiSnrStatus();

  return EXIT_SUCCESS;
}
#endif

int radioScan(int /*argc*/ = 0, char ** /*argv*/ = NULL)
{
#ifdef OPERATOR_MODE
  Serial.println(F("Scanning....."));
#endif
  Radio.scan();

  return EXIT_SUCCESS;
}

int radioLastStatus(int /*argc*/ = 0, char ** /*argv*/ = NULL)
{
#ifdef OPERATOR_MODE
  printlnByteBinary(lastIntStatus);
#endif

  return EXIT_SUCCESS;
}

int radioChannelDown(int /*argc*/ = 0, char ** /*argv*/ = NULL)
{
  if (channel <= SI4707_WB_MIN_FREQUENCY)
  {
    return EXIT_FAILURE;
  }
#ifdef OPERATOR_MODE
  Serial.println(F("Channel down."));
#endif
  channel -= SI4707_WB_CHANNEL_SPACING;
  Radio.tune();

  return EXIT_SUCCESS;
}

int radioChannelUp(int /*argc*/ = 0, char ** /*argv*/ = NULL)
{
  if (channel >= SI4707_WB_MAX_FREQUENCY)
  {
    return EXIT_FAILURE;
  }
#ifdef OPERATOR_MODE
  Serial.println(F("Channel up."));
#endif
  channel += SI4707_WB_CHANNEL_SPACING;
  Radio.tune();

  return EXIT_SUCCESS;
}

int radioVolume(int argc, char **argv)
{
  if (argc <= 1)
  {
#ifdef OPERATOR_MODE
    Serial.print(asFlashString(MISSING_ARGUMENT_TEXT));
    Serial.println(asFlashString(SETTING_VALUE_TEXT));
#endif

    return EXIT_FAILURE;
  }

  String argValue = String(argv[1]);

  argValue.trim();

  Radio.setVolume(argValue.toInt());
#ifdef OPERATOR_MODE
  Serial.print(F("Volume: "));
  Serial.println(volume);
#endif

  return EXIT_SUCCESS;
}

int radioMute(int /*argc*/ = 0, char ** /*argv*/ = NULL)
{
  if (mute)
  {
    Radio.setMute(SI4707_OFF);
#ifdef OPERATOR_MODE
    Serial.println(F("Mute: Off"));
#endif
  }
  else
  {
    Radio.setMute(SI4707_ON);
#ifdef OPERATOR_MODE
    Serial.println(F("Mute: On"));
#endif
  }

  return EXIT_SUCCESS;
}

int radioSameStatus(int /*argc*/ = 0, char ** /*argv*/ = NULL)
{
  Radio.getSameStatus(SI4707_CHECK);

#ifdef OPERATOR_MODE
  printlnByteBinary(msgStatus);
#endif

  return EXIT_SUCCESS;
}

int radioPower(int /*argc*/ = 0, char ** /*argv*/ = NULL)
{
  if (power)
  {
    Radio.disableInterrupt();
    Radio.off();
#ifdef OPERATOR_MODE
    Serial.println(F("Radio powered off."));
#endif
  }
  else
  {
    Radio.on();
    Radio.enableInterrupt();
#ifdef OPERATOR_MODE
    Serial.println(F("Radio powered on."));
#endif
    Radio.tune();
  }

  return EXIT_SUCCESS;
}

//
//  Status bits are processed here.
//
int radioStatus(int /*argc*/ = 0, char ** /*argv*/ = NULL)
{
  Radio.getIntStatus();
  lastIntStatus = intStatus;

  if (intStatus & SI4707_STCINT)
  {
    Radio.getTuneStatus(SI4707_INTACK); //  Using SI4707_INTACK clears SI4707_STCINT, SI4707_CHECK preserves it.
#ifdef OPERATOR_MODE
    radioSeekStatus();
#else
    SeekTuneCompletePacket payload;
    payload.weatherRadioInterruptType = WeatherRadioInterruptType::WB_STC_INTERRUPT;
    String frequencyString(frequency, 3);
    frequencyString.toCharArray(payload.frequency, sizeof(payload.frequency));
    payload.rssi = rssi;
    payload.snr = snr;

    UnoStormPacketHeader header = {
        .packetType = PacketType::WEATHER_RADIO_PACKET,
        .payloadSize = sizeof(payload),
    };
    UnoStormPacket packet = {
        .header = header,
        .payload = reinterpret_cast<const uint8_t *>(&payload),
    };
    sendPacket(&packet);
#endif
    Radio.sameFlush(); //  This should be done after any tune function.
    // intStatus |= SI4707_RSQINT;  //  We can force it to get rsqStatus on any tune.
  }

  if (intStatus & SI4707_RSQINT)
  {
#ifdef OPERATOR_MODE
    radioRsqStatus();
#else
    Radio.getRsqStatus(SI4707_INTACK);

    ReceivedSignalQualityPacket payload = {
        .weatherRadioInterruptType = WeatherRadioInterruptType::WB_RSQ_INTERRUPT,
        .rssi = rssi,
        .snr = snr,
        .freqoff = freqoff,
    };
    UnoStormPacketHeader header = {
        .packetType = PacketType::WEATHER_RADIO_PACKET,
        .payloadSize = sizeof(payload),
    };
    UnoStormPacket packet = {
        .header = header,
        .payload = reinterpret_cast<const uint8_t *>(&payload),
    };
    sendPacket(&packet);
#endif
  }

  if (intStatus & SI4707_SAMEINT)
  {
    Radio.getSameStatus(SI4707_INTACK);

#ifdef OPERATOR_MODE
    printlnByteBinary(intStatus);
    printlnByteBinary(msgStatus);
    printlnByteBinary(sameStatus);
    printlnByteBinary(sameState);
    Serial.println(sameLength);
    Serial.println(sameHeaderCount);
#endif

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
      stopSameFlushTimer();
#ifdef OPERATOR_MODE
      Serial.println(F("EOM detected."));
#else
      SameStatusPacket payload = {.sameInterruptType = SameInterruptType::WB_SAME_END_OF_MESSAGE};
      UnoStormPacketHeader header = {
          .packetType = PacketType::SAME_STATUS_PACKET,
          .payloadSize = sizeof(payload),
      };
      UnoStormPacket packet = {
          .header = header,
          .payload = reinterpret_cast<const uint8_t *>(&payload),
      };
      sendPacket(&packet);
#endif
      //  More application specific code could go here. (Mute audio, turn something on/off, etc.)
    }

    if (sameStatus & SI4707_PREDET)
    {
      startSameFlushTimer();
#ifdef OPERATOR_MODE
      Serial.println(F("Preamble detected."));
#else
      SameStatusPacket payload = {.sameInterruptType = SameInterruptType::WB_SAME_PREAMBLE};
      UnoStormPacketHeader header = {
          .packetType = PacketType::SAME_STATUS_PACKET,
          .payloadSize = sizeof(payload),
      };
      UnoStormPacket packet = {
          .header = header,
          .payload = reinterpret_cast<const uint8_t *>(&payload),
      };
      sendPacket(&packet);
#endif
    }

    // If a message is available and not already used,
    if (msgStatus & SI4707_MSGAVL && (!(msgStatus & SI4707_MSGUSD)))
    {
      // parse it.
      Radio.sameParse();
    }

    if (msgStatus & SI4707_MSGPAR)
    {
      msgStatus &= ~SI4707_MSGPAR; // Clear the parse status, so that we don't print it again.
#ifdef OPERATOR_MODE
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
      if (sameHour < 10)
      {
        Serial.print('0');
      }
      Serial.print(sameHour);
      Serial.print(':');
      if (sameMinute < 10)
      {
        Serial.print('0');
      }
      Serial.println(sameMinute);
      Serial.print(F("Callsign: "));
      Serial.println(sameCallSign);
      Serial.println();
#else
      SameMessagePacket payload = {
          .sameInterruptType = SameInterruptType::WB_SAME_MESSAGE_RECEIVED,
          .originatorNameSize = static_cast<uint8_t>(strlen(sameOriginatorName)),
          .originatorName = sameOriginatorName,
          .eventNameSize = static_cast<uint8_t>(strlen(sameEventName)),
          .eventName = sameEventName,
          .callSignSize = static_cast<uint8_t>(strlen(sameCallSign)),
          .callSign = sameCallSign,
          .locations = sameLocations,
          .locationCodes = sameLocationCodes,
          .duration = sameDuration,
          .day = sameDay,
          .hour = sameHour,
          .minute = sameMinute,
      };
      UnoStormPacketHeader header = {
          .packetType = PacketType::SAME_MESSAGE_PACKET,
          // Don't use sizeof on structs with pointers
          .payloadSize = 1 +                               // sameInterruptType:uint8_t
                         1U + payload.originatorNameSize + // originatorNameSize:uint8_t + (originatorName:uint8_t * originatorNameSize)
                         1U + payload.eventNameSize +      // eventNameSize:uint8_t + (eventName:uint8_t * eventNameSize)
                         1U + payload.callSignSize +       // callSignSize:uint8_t + (callSign:uint8_t * callSignSize)
                         1U + (payload.locations * 4) +    // locationa:uint8_t + (locationCodes:uint32_t * locationCodesSize)
                         2U +                              // duration:uint16_t
                         2U +                              // day:uint16_t
                         1U +                              // hour:uint8_t
                         1U                                // minute:uint8_t
          ,
      };
      UnoStormPacket packet;
      packet.header = header;
      startPacket(header);

      size_t bytesSent = 0;

      bytesSent += Serial.write(payload.sameInterruptType);
      bytesSent += Serial.write(payload.originatorNameSize);
      bytesSent += sendBuffer(payload.originatorNameSize, reinterpret_cast<const uint8_t *>(payload.originatorName));
      bytesSent += Serial.write(payload.eventNameSize);
      bytesSent += sendBuffer(payload.eventNameSize, reinterpret_cast<const uint8_t *>(payload.eventName));
      bytesSent += Serial.write(payload.callSignSize);
      bytesSent += sendBuffer(payload.callSignSize, reinterpret_cast<const uint8_t *>(payload.callSign));
      bytesSent += Serial.write(payload.locations);

      for (uint8_t i = 0; i < payload.locations; i++)
      {
        bytesSent += sendBuffer(4, reinterpret_cast<const uint8_t *>(&(payload.locationCodes[i])));
      }

      bytesSent += sendBuffer(2, reinterpret_cast<const uint8_t *>(&(payload.duration)));
      bytesSent += sendBuffer(2, reinterpret_cast<const uint8_t *>(&(payload.day)));
      bytesSent += Serial.write(payload.hour);
      bytesSent += Serial.write(payload.minute);

      // TODO: Remove later when you're confident this code is correct.
      ASSERT(bytesSent == header.payloadSize);

#endif
    }

    if (msgStatus & SI4707_MSGPUR) //  Signals that the third header has been received.
    {
#ifdef OPERATOR_MODE
      Serial.println(F("Third Header Received"));
#endif
      stopSameFlushTimer();
    }
  }

  if (intStatus & SI4707_ASQINT)
  {
    Radio.getAsqStatus(SI4707_INTACK);

    if (sameWat != asqStatus)
    {
      if (asqStatus == 0x01)
      {
        // New Alert Tone
        Radio.setProperty(SI4707_WB_ASQ_INT_SOURCE, (SI4707_ALERTOFIEN));
        // SAME is done by now.
        stopSameFlushTimer();

#ifdef OPERATOR_MODE
        Serial.println(F("WAT is on."));
        Serial.println();
#else
        AlertTonePacket payload = {
            .weatherRadioInterruptType = WeatherRadioInterruptType::WB_ALERT_TONE_INTERRUPT,
            .alertTone = 1,
        };
        UnoStormPacketHeader header = {
            .packetType = PacketType::WEATHER_RADIO_PACKET,
            .payloadSize = sizeof(AlertTonePacket),
        };
        UnoStormPacket packet = {
            .header = header,
            .payload = reinterpret_cast<const uint8_t *>(&payload),
        };
        sendPacket(&packet);
#endif
        //  More application specific code could go here.  (Unmute audio, turn something on/off, etc.)
      }

      if (asqStatus == 0x02)
      {
        Radio.setProperty(SI4707_WB_ASQ_INT_SOURCE, (SI4707_ALERTONIEN));
#ifdef OPERATOR_MODE
        Serial.println(F("WAT is off."));
        Serial.println();
#else
        AlertTonePacket payload = {
            .weatherRadioInterruptType = WeatherRadioInterruptType::WB_ALERT_TONE_INTERRUPT,
            .alertTone = 0,
        };
        UnoStormPacketHeader header = {
            .packetType = PacketType::WEATHER_RADIO_PACKET,
            .payloadSize = sizeof(AlertTonePacket),
        };
        UnoStormPacket packet = {
            .header = header,
            .payload = reinterpret_cast<const uint8_t *>(&payload),
        };
        sendPacket(&packet);
#endif
        //  More application specific code could go here.  (Mute audio, turn something on/off, etc.)
      }

      sameWat = asqStatus;
    }
  }

  if (intStatus & SI4707_ERRINT)
  {
    intStatus &= ~SI4707_ERRINT;
#ifdef OPERATOR_MODE
    Serial.println(F("An error occured!"));
    Serial.println();
#else
    ErrorPacket payload = {
        .weatherRadioInterruptType = WeatherRadioInterruptType::WB_ERROR_INTERRUPT,
    };
    UnoStormPacketHeader header = {
        .packetType = PacketType::WEATHER_RADIO_PACKET,
        .payloadSize = sizeof(ErrorPacket),
    };
    UnoStormPacket packet = {
        .header = header,
        .payload = reinterpret_cast<const uint8_t *>(&payload),
    };
    sendPacket(&packet);
#endif
  }

  return EXIT_SUCCESS;
}

//
//  The End.
//

#ifdef OPERATOR_MODE
//
//  Prints the Function Menu.
//
int showMenu(int /*argc*/ = 0, char ** /*argv*/ = NULL)
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
#endif

void setup()
{
  // Initialize serial and wait for port to open:
  Serial.begin(115200);

#ifdef OPERATOR_MODE
  Serial.println(F("Uno Storm"));
#endif
  settingsStorage.load();
  sensorSettings = settingsStorage;
#ifdef OPERATOR_MODE
  Serial.print(F("Starting Radio"));
#endif
  pinMode(LED_BUILTIN, OUTPUT);

#ifdef OPERATOR_MODE
  Serial.print('.');
#endif
  Radio.begin();
#ifdef OPERATOR_MODE
  Serial.print('.');
#endif
  Radio.patch(); //  Use this one to to include the 1050 Hz patch.
#ifdef OPERATOR_MODE
  Serial.print('.');
#endif
  // Radio.on();           //  Use this one if not using the patch.
  // Radio.getRevision();  //  Only captured on the logic analyzer - not displayed.
  //
  //  All useful interrupts are enabled here.
  //
  Radio.setProperty(SI4707_GPO_IEN, (SI4707_ERRIEN | SI4707_RSQIEN | SI4707_SAMEIEN | SI4707_ASQIEN | SI4707_STCIEN));
#ifdef OPERATOR_MODE
  Serial.print('.');
#endif
  //
  //  RSQ Interrupt Sources.
  //
  // Radio.setProperty(SI4707_WB_RSQ_SNR_HIGH_THRESHOLD, 0x007F);   // 127 dBuV for testing..want it high
  // Radio.setProperty(SI4707_WB_RSQ_SNR_LOW_THRESHOLD, 0x0001);    // 1 dBuV for testing
  // Radio.setProperty(SI4707_WB_RSQ_RSSI_HIGH_THRESHOLD, 0x004D);  // -30 dBm for testing
  // Radio.setProperty(SI4707_WB_RSQ_RSSI_LOW_THRESHOLD, 0x0007);   // -100 dBm for testing
  // Radio.setProperty(SI4707_WB_RSQ_INT_SOURCE, (SI4707_SNRHIEN | SI4707_SNRLIEN | SI4707_RSSIHIEN | SI4707_RSSILIEN));
  //
  //  SAME Interrupt Sources.
  //
  Radio.setProperty(SI4707_WB_SAME_INTERRUPT_SOURCE, (SI4707_EOMDETIEN | SI4707_HDRRDYIEN));
#ifdef OPERATOR_MODE
  Serial.print('.');
#endif

  //
  //  ASQ Interrupt Sources.
  //
  Radio.setProperty(SI4707_WB_ASQ_INT_SOURCE, (SI4707_ALERTONIEN));
#ifdef OPERATOR_MODE
  Serial.print('.');
#endif
  //
  //  Tune to the desired frequency.
  //
  Radio.tune(162550); //  6 digits only.

#ifdef OPERATOR_MODE
  Serial.println(F("Done"));

  Serial.println(F("Starting Lightning Sensor"));
#else
  // Prefix the boot message from the Sparkfun library
  {
    // sensor.begin writes "Calibrating Oscillators\r\n" to Serial
    UnoStormPacketHeader header = {.packetType = PacketType::BOOT_MESSAGE, .payloadSize = 25U};
    startPacket(header);
  }

#endif
  if (sensor.begin(sensorSettings) < 0)
  {
#ifdef OPERATOR_MODE
    Serial.println(F("Failed"));
#endif
    while (1)
    {
      delay(100);
    }
  }

  Radio.enableInterrupt();

#ifdef OPERATOR_MODE
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
#endif

#ifdef TEST_MODE
  {
    SensorEvent event = {.type = LIGHTNING, .distance = STORM_IS_OVERHEAD, .energy = 0xDEAD};
    LightningPacket payload = {
        .interruptType = event.type,
        .distance = event.distance,
        .energy = event.energy,
    };
    UnoStormPacketHeader header = {
        .packetType = PacketType::LIGHTNING_PACKET,
        .payloadSize = sizeof(payload),
    };
    UnoStormPacket packet = {
        .header = header,
        .payload = reinterpret_cast<const uint8_t *>(&payload),
    };
    sendPacket(&packet);
  }
  {
    Radio.getIntStatus();
    lastIntStatus = intStatus;
    Radio.getTuneStatus(SI4707_INTACK);
    SeekTuneCompletePacket payload;
    payload.weatherRadioInterruptType = WeatherRadioInterruptType::WB_STC_INTERRUPT;
    String frequencyString(frequency, 3);
    frequencyString.toCharArray(payload.frequency, sizeof(payload.frequency));
    payload.rssi = rssi;
    payload.snr = snr;

    UnoStormPacketHeader header = {
        .packetType = PacketType::WEATHER_RADIO_PACKET,
        .payloadSize = sizeof(payload),
    };
    UnoStormPacket packet = {
        .header = header,
        .payload = reinterpret_cast<const uint8_t *>(&payload),
    };
    sendPacket(&packet);
  }
  {
    Radio.getRsqStatus(SI4707_INTACK);
    ReceivedSignalQualityPacket payload = {
        .weatherRadioInterruptType = WeatherRadioInterruptType::WB_RSQ_INTERRUPT,
        .rssi = rssi,
        .snr = snr,
        .freqoff = freqoff,
    };
    UnoStormPacketHeader header = {
        .packetType = PacketType::WEATHER_RADIO_PACKET,
        .payloadSize = sizeof(payload),
    };
    UnoStormPacket packet = {
        .header = header,
        .payload = reinterpret_cast<const uint8_t *>(&payload),
    };
    sendPacket(&packet);
  }
  {
    SameStatusPacket payload = {.sameInterruptType = SameInterruptType::WB_SAME_END_OF_MESSAGE};
    UnoStormPacketHeader header = {
        .packetType = PacketType::SAME_STATUS_PACKET,
        .payloadSize = sizeof(payload),
    };
    UnoStormPacket packet = {
        .header = header,
        .payload = reinterpret_cast<const uint8_t *>(&payload),
    };
    sendPacket(&packet);
  }
  {
    SameStatusPacket payload = {.sameInterruptType = SameInterruptType::WB_SAME_PREAMBLE};
    UnoStormPacketHeader header = {
        .packetType = PacketType::SAME_STATUS_PACKET,
        .payloadSize = sizeof(payload),
    };
    UnoStormPacket packet = {
        .header = header,
        .payload = reinterpret_cast<const uint8_t *>(&payload),
    };
    sendPacket(&packet);
  }

  {
    Radio.sameFill("ZCZC-WXR-TOR-039173-039051-139069+0030-1591829-KCLE/NWS-");
    msgStatus |= SI4707_MSGAVL; // Simulate message available
    Radio.sameParse();
    SameMessagePacket payload = {
        .sameInterruptType = SameInterruptType::WB_SAME_MESSAGE_RECEIVED,
        .originatorNameSize = static_cast<uint8_t>(strlen(sameOriginatorName)),
        .originatorName = sameOriginatorName,
        .eventNameSize = static_cast<uint8_t>(strlen(sameEventName)),
        .eventName = sameEventName,
        .callSignSize = static_cast<uint8_t>(strlen(sameCallSign)),
        .callSign = sameCallSign,
        .locations = sameLocations,
        .locationCodes = sameLocationCodes,
        .duration = sameDuration,
        .day = sameDay,
        .hour = sameHour,
        .minute = sameMinute,
    };
    UnoStormPacketHeader header = {
        .packetType = PacketType::SAME_MESSAGE_PACKET,
        // Don't use sizeof on structs with pointers
        .payloadSize = 1U +                              // sameInterruptType:uint8_t
                       1U + payload.originatorNameSize + // originatorNameSize:uint8_t + (originatorName:uint8_t * originatorNameSize)
                       1U + payload.eventNameSize +      // eventNameSize:uint8_t + (eventName:uint8_t * eventNameSize)
                       1U + payload.callSignSize +       // callSignSize:uint8_t + (callSign:uint8_t * callSignSize)
                       1U + (payload.locations * 4) +    // locationa:uint8_t + (locationCodes:uint32_t * locationCodesSize)
                       2U +                              // duration:uint16_t
                       2U +                              // day:uint16_t
                       1U +                              // hour:uint8_t
                       1U                                // minute:uint8_t
        ,
    };
    UnoStormPacket packet;
    packet.header = header;
    startPacket(header);

    size_t bytesSent = 0;

    bytesSent += Serial.write(payload.sameInterruptType);
    bytesSent += Serial.write(payload.originatorNameSize);
    bytesSent += sendBuffer(payload.originatorNameSize, reinterpret_cast<const uint8_t *>(payload.originatorName));
    bytesSent += Serial.write(payload.eventNameSize);
    bytesSent += sendBuffer(payload.eventNameSize, reinterpret_cast<const uint8_t *>(payload.eventName));
    bytesSent += Serial.write(payload.callSignSize);
    bytesSent += sendBuffer(payload.callSignSize, reinterpret_cast<const uint8_t *>(payload.callSign));
    bytesSent += Serial.write(payload.locations);

    for (uint8_t i = 0; i < payload.locations; i++)
    {
      bytesSent += sendBuffer(4, reinterpret_cast<const uint8_t *>(&(payload.locationCodes[i])));
    }

    bytesSent += sendBuffer(2, reinterpret_cast<const uint8_t *>(&(payload.duration)));
    bytesSent += sendBuffer(2, reinterpret_cast<const uint8_t *>(&(payload.day)));
    bytesSent += Serial.write(payload.hour);
    bytesSent += Serial.write(payload.minute);

    // TODO: Remove later when you're confident this code is correct.
    ASSERT(bytesSent == header.payloadSize);
  }
  {
    AlertTonePacket payload = {
        .weatherRadioInterruptType = WeatherRadioInterruptType::WB_ALERT_TONE_INTERRUPT,
        .alertTone = 1,
    };
    UnoStormPacketHeader header = {
        .packetType = PacketType::WEATHER_RADIO_PACKET,
        .payloadSize = sizeof(AlertTonePacket),
    };
    UnoStormPacket packet = {
        .header = header,
        .payload = reinterpret_cast<const uint8_t *>(&payload),
    };
    sendPacket(&packet);
  }
  {
    AlertTonePacket payload = {
        .weatherRadioInterruptType = WeatherRadioInterruptType::WB_ALERT_TONE_INTERRUPT,
        .alertTone = 0,
    };
    UnoStormPacketHeader header = {
        .packetType = PacketType::WEATHER_RADIO_PACKET,
        .payloadSize = sizeof(AlertTonePacket),
    };
    UnoStormPacket packet = {
        .header = header,
        .payload = reinterpret_cast<const uint8_t *>(&payload),
    };
    sendPacket(&packet);
  }
  {
    ErrorPacket payload = {
        .weatherRadioInterruptType = WeatherRadioInterruptType::WB_ERROR_INTERRUPT,
    };
    UnoStormPacketHeader header = {
        .packetType = PacketType::WEATHER_RADIO_PACKET,
        .payloadSize = sizeof(ErrorPacket),
    };
    UnoStormPacket packet = {
        .header = header,
        .payload = reinterpret_cast<const uint8_t *>(&payload),
    };
    sendPacket(&packet);
  }
#endif
}

void loop()
{
#ifdef OPERATOR_MODE
  shell.executeIfInput();
#endif

  checkLightningSensor();
  if (intStatus & SI4707_INTAVL)
  {
    radioStatus();
  }
  taskTimer.tick();
}
