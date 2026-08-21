#ifndef _SENSOR_EVENT_H_
#define _SENSOR_EVENT_H_

#include <stdint.h>

struct SensorEvent
{
  uint8_t type;
  uint8_t distance;
  uint32_t energy;
};

#endif
