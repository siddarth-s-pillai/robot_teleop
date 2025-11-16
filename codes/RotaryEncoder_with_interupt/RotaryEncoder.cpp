#include "RotaryEncoder.h"

RotaryEncoder* RotaryEncoder::instance = nullptr;

RotaryEncoder::RotaryEncoder(int pinA, int pinB, int pinSW)
: _pinA(pinA), _pinB(pinB), _pinSW(pinSW),
  _steps(0), _direction(0), _lastButton(0) {
    instance = this;
}

void RotaryEncoder::begin() {
    pinMode(_pinA, INPUT_PULLUP);
    pinMode(_pinB, INPUT_PULLUP);
    pinMode(_pinSW, INPUT_PULLUP);

    _lastCLK = digitalRead(_pinA);
    attachInterrupt(digitalPinToInterrupt(_pinA), isrHandler, CHANGE);
}

void IRAM_ATTR RotaryEncoder::isrHandler() {
    int clk = digitalRead(instance->_pinA);
    int dt  = digitalRead(instance->_pinB);

    if (clk != instance->_lastCLK) {
        if (dt != clk) {
            instance->_steps++;
            instance->_direction = 1;
        } else {
            instance->_steps--;
            instance->_direction = -1;
        }
    }
    instance->_lastCLK = clk;
}

void RotaryEncoder::update() {
    // Debounce button
    if (digitalRead(_pinSW) == LOW) {
        unsigned long now = millis();
        if (now - _lastButton > 200) {
            _lastButton = now;
        }
    }
}

long RotaryEncoder::getSteps() {
    noInterrupts();
    long s = _steps;
    interrupts();
    return s;
}

int RotaryEncoder::getDirection() {
    noInterrupts();
    int d = _direction;
    _direction = 0;
    interrupts();
    return d;
}

bool RotaryEncoder::getButton() {
    return digitalRead(_pinSW) == LOW;
}
