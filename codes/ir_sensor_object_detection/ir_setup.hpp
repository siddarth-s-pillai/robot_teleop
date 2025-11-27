#ifndef IR_SETUP_HPP
#define IR_SETUP_HPP
#include "Arduino.h"

#define SENSOR_PIN    4   // IR sensor OUT/DO pin connected here


class IRSensor {
public:
    IRSensor();
    void begin();
    bool isObjectDetected();
    void updateSensorState();

private:
    bool objectDetected=false;
    int sensor_pin=SENSOR_PIN;
    unsigned long lastReadTime = 0;
};

#endif // IR_SETUP_HPP