#include <SI4707.h>
#include <stdint.h>

enum PacketType : uint8_t
{
  LIGHTNING_PACKET = 1,
  WEATHER_RADIO_PACKET = 2,
  SAME_STATUS_PACKET = 3,
  SAME_MESSAGE_PACKET = 4,
  BOOT_MESSAGE = 5
};

struct __attribute__((packed)) UnoStormPacketHeader
{
  PacketType packetType;
  uint16_t payloadSize;
};

struct __attribute__((packed)) UnoStormPacket
{
  UnoStormPacketHeader header;
  const uint8_t *payload;
};

struct __attribute__((packed)) LightningPacket
{
  uint8_t interruptType;
  uint8_t distance;
  uint32_t energy;
};

enum WeatherRadioInterruptType : uint8_t
{
  WB_STC_INTERRUPT = SI4707_STCINT,
  WB_ALERT_TONE_INTERRUPT = SI4707_ASQINT,
  WB_SAME_INTERRUPT = SI4707_SAMEINT,
  WB_RSQ_INTERRUPT = SI4707_RSQINT,
  WB_ERROR_INTERRUPT = SI4707_ERRINT
};

struct __attribute__((packed)) SeekTuneCompletePacket
{
  WeatherRadioInterruptType weatherRadioInterruptType;
  uint8_t rssi;
  uint8_t snr;
  // "162.550"
  // ['1', '6', '2', '.', '5', '5', '0', '\0']
  char frequency[8];
};

struct __attribute__((packed)) ReceivedSignalQualityPacket
{
  WeatherRadioInterruptType weatherRadioInterruptType;
  uint8_t rssi;
  uint8_t snr;
  signed char freqoff;
};

enum SameInterruptType : uint8_t
{
  WB_SAME_PREAMBLE = 3,
  WB_SAME_END_OF_MESSAGE = 5,
  WB_SAME_MESSAGE_RECEIVED = 7
};

struct __attribute__((packed)) SameStatusPacket
{
  SameInterruptType sameInterruptType;
};

struct __attribute__((packed)) SameMessagePacket
{
  SameInterruptType sameInterruptType;
  // char sameOriginatorName[4];
  uint8_t originatorNameSize;
  char *originatorName;
  // char sameEventName[4];
  uint8_t eventNameSize;
  char *eventName;
  // char sameCallSign[9];
  uint8_t callSignSize;
  char *callSign;
  // uint8_t sameLocations;
  uint8_t locations;
  // uint32_t sameLocationCodes[SI4707_SAME_LOCATION_CODES];
  uint32_t *locationCodes;
  // uint16_t sameDuration;
  uint16_t duration;
  // uint16_t sameDay;
  uint16_t day;
  // uint8_t sameHour;
  uint8_t hour;
  // uint8_t sameMinute;
  uint8_t minute;
};

struct __attribute__((packed)) AlertTonePacket
{
  WeatherRadioInterruptType weatherRadioInterruptType;
  uint8_t alertTone; // 1 = On, 0 = Off
};

struct __attribute__((packed)) ErrorPacket
{
  WeatherRadioInterruptType weatherRadioInterruptType;
};

size_t startPacket(UnoStormPacketHeader header);
size_t sendPacket(UnoStormPacket packet);
size_t sendBuffer(size_t bufferSize, const uint8_t *buffer);

static_assert(sizeof(UnoStormPacketHeader) == 3, "UnoStormPacketHeader size is unexpected");
static_assert(sizeof(UnoStormPacket) == 3 + sizeof(const uint8_t *), "UnoStormPacket size is unexpected");
static_assert(sizeof(LightningPacket) == 6, "LightningPacket size is unexpected");
static_assert(sizeof(SeekTuneCompletePacket) == 11, "SeekTuneCompletePacket size is unexpected");
static_assert(sizeof(ReceivedSignalQualityPacket) == 4, "ReceivedSignalQualityPacket size is unexpected");
static_assert(sizeof(SameStatusPacket) == 1, "SameStatusPacket size is unexpected");
static_assert(sizeof(SameMessagePacket) == 1                        // SameInterruptType sameInterruptType;
                                               + 1                  // uint8_t originatorNameSize;
                                               + sizeof(char *)     // char *originatorName;
                                               + 1                  // uint8_t eventNameSize;
                                               + sizeof(char *)     // char *eventName;
                                               + 1                  // uint8_t callSignSize;
                                               + sizeof(char *)     // char *callSign;
                                               + 1                  // uint8_t locations;
                                               + sizeof(uint32_t *) // uint32_t *locationCodes;
                                               + 2                  // uint16_t duration;
                                               + 2                  // uint16_t day;
                                               + 1                  // uint8_t hour;
                                               + 1,                 // uint8_t minute;
              "SameMessagePacket size is unexpected");
static_assert(sizeof(AlertTonePacket) == 2, "AlertTonePacket size is unexpected");
static_assert(sizeof(ErrorPacket) == 1, "ErrorPacket size is unexpected");
