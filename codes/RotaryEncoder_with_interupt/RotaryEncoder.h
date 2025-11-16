#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <Arduino.h>

class RotaryEncoder {
public:
    RotaryEncoder(int pinA, int pinB, int pinSW);

    void begin();
    void update();       // call in loop
    long getSteps();
    int getDirection();  // -1, 0, +1
    bool getButton();

private:
    int _pinA, _pinB, _pinSW;
    volatile long _steps;
    volatile int _direction;
    int _lastCLK;
    unsigned long _lastButton;

    static RotaryEncoder* instance;
    static void IRAM_ATTR isrHandler();
};

#endif
