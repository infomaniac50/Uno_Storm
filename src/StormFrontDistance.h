#ifndef _STORM_FRONT_DISTANCE_H_
#define _STORM_FRONT_DISTANCE_H_

enum StormFrontDistance : uint8_t
{
  OUT_OF_RANGE = 0b111111,
  DISTANCE_40KM = 0b101000,
  DISTANCE_37KM = 0b100101,
  DISTANCE_34KM = 0b100010,
  DISTANCE_31KM = 0b011111,
  DISTANCE_27KM = 0b011011,
  DISTANCE_24KM = 0b011000,
  DISTANCE_20KM = 0b010100,
  DISTANCE_17KM = 0b010001,
  DISTANCE_14KM = 0b001110,
  DISTANCE_12KM = 0b001100,
  DISTANCE_10KM = 0b001010,
  DISTANCE_8KM = 0b001000,
  DISTANCE_6KM = 0b000110,
  DISTANCE_5KM = 0b000101,
  STORM_IS_OVERHEAD = 0b000001
};

inline const __FlashStringHelper* distanceToString(uint8_t distance)
{
  switch (distance)
  {
    case StormFrontDistance::OUT_OF_RANGE:
      return F("out of range");
    case StormFrontDistance::DISTANCE_40KM:
      return F("40 km away");
    case StormFrontDistance::DISTANCE_37KM:
      return F("37 km away");
    case StormFrontDistance::DISTANCE_34KM:
      return F("34 km away");
    case StormFrontDistance::DISTANCE_31KM:
      return F("31 km away");
    case StormFrontDistance::DISTANCE_27KM:
      return F("27 km away");
    case StormFrontDistance::DISTANCE_24KM:
      return F("24 km away");
    case StormFrontDistance::DISTANCE_20KM:
      return F("20 km away");
    case StormFrontDistance::DISTANCE_17KM:
      return F("17 km away");
    case StormFrontDistance::DISTANCE_14KM:
      return F("14 km away");
    case StormFrontDistance::DISTANCE_12KM:
      return F("12 km away");
    case StormFrontDistance::DISTANCE_10KM:
      return F("10 km away");
    case StormFrontDistance::DISTANCE_8KM:
      return F("8 km away");
    case StormFrontDistance::DISTANCE_6KM:
      return F("6 km away");
    case StormFrontDistance::DISTANCE_5KM:
      return F("5 km away");
    case StormFrontDistance::STORM_IS_OVERHEAD:
      return F("overhead");
    default:
      return F("invalid");
  }
}

#endif
