#ifndef _SENSOR_EVENT_H_
#define _SENSOR_EVENT_H_

struct SensorEvent
{
  uint8_t type;
  uint8_t distance;
  long energy;
};

#endif
