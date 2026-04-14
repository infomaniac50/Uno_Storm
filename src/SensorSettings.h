#ifndef _SENSOR_SETTINGS_H_
#define _SENSOR_SETTINGS_H_

#include <SparkFun_AS3935.h>

#define DEFAULT_SENSOR_SETTING_NOISE_FLOOR (uint8_t)2
#define DEFAULT_SENSOR_SETTING_WATCHDOG_THRESHOLD (uint8_t)2
#define DEFAULT_SENSOR_SETTING_SPIKE_REJECTION (uint8_t)2
#define DEFAULT_SENSOR_SETTING_LIGHTNING_THRESHOLD (uint8_t)1
#define DEFAULT_SENSOR_SETTING_TUNING_CAPACITOR (uint8_t)0
#define DEFAULT_SENSOR_SETTING_SENSOR_LOCATION (uint8_t)INDOOR
#define DEFAULT_SENSOR_SETTING_REPORT_DISTURBER (bool)false

// Values for modifying the IC's settings. All of these values are set to their
// default values.
struct SensorSettings
{
  uint8_t noiseFloor = DEFAULT_SENSOR_SETTING_NOISE_FLOOR;
  uint8_t watchdogThreshold = DEFAULT_SENSOR_SETTING_WATCHDOG_THRESHOLD;
  uint8_t spikeRejection = DEFAULT_SENSOR_SETTING_SPIKE_REJECTION;
  uint8_t lightningThreshold = DEFAULT_SENSOR_SETTING_LIGHTNING_THRESHOLD;
  uint8_t tuningCapacitor = DEFAULT_SENSOR_SETTING_TUNING_CAPACITOR;
  uint8_t sensorLocation = DEFAULT_SENSOR_SETTING_SENSOR_LOCATION;
  bool reportDisturber = DEFAULT_SENSOR_SETTING_REPORT_DISTURBER;

  // Constructor with parameters
  // SensorSettings (int noiseFloor, int b, int c) : a (a), b (b), c (c) {}
  // Default constructor
  // SensorSettings() : SensorSettings () {}
  SensorSettings() {}
  // Copy constructor
  SensorSettings (const SensorSettings &) = default; // This is required
  // Comparison operator
  bool  operator == (const SensorSettings &rhs) const {
    return noiseFloor == rhs.noiseFloor && 
      watchdogThreshold == rhs.watchdogThreshold && 
      spikeRejection == rhs.spikeRejection &&
      lightningThreshold == rhs.lightningThreshold &&
      tuningCapacitor == rhs.tuningCapacitor &&
      sensorLocation == rhs.sensorLocation &&
      reportDisturber == rhs.reportDisturber
      ;
  }
  // Inequality operator
  bool  operator != (const SensorSettings &rhs) const {
    return ! (*this == rhs);
  }
  // Print method
  void print (const char *prefix = "", Print & out = Serial) const {
    out.print (prefix);
    out.print (noiseFloor);
    out.write (' ');
    out.print (watchdogThreshold);
    out.write (' ');
    out.println (spikeRejection);
    out.write (' ');
    out.println (lightningThreshold);
    out.write (' ');
    out.println (tuningCapacitor);
    out.write (' ');
    out.println (sensorLocation);
    out.write (' ');
    out.println (reportDisturber);
  }
};
#endif
