#ifndef IR_SETUP_HPP
#define IR_SETUP_HPP

#define LED_PIN       2   // Onboard LED (on most ESP32 dev boards)
#define SENSOR_PIN    4   // IR sensor OUT/DO pin connected here


class IRSensor {
public:
    IRSensor(int ledPin=LED_PIN, int sensorPin=SENSOR_PIN)
        : led_pin(ledPin), sensor_pin(sensorPin) {};
    void begin();
    bool isObjectDetected();
    void updateSensorState();

private:
    bool objectDetected=false;
    int led_pin=LED_PIN;
    int sensor_pin=SENSOR_PIN;
    unsigned long lastReadTime = 0;
};

#endif // IR_SETUP_HPP