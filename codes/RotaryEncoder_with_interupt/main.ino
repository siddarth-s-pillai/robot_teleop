#include <Arduino.h>
#include "RotaryEncoder.h"
#include "JsonEncoder.h"

// CLK, DT, SW pins
RotaryEncoder encoder(D5, D6, D7);
JsonEncoder json;

void setup() {
    Serial.begin(115200);
    encoder.begin();
}

void loop() {
    encoder.update();

    long steps = encoder.getSteps();
    int dir    = encoder.getDirection();
    bool btn   = encoder.getButton();

    String jsonStr;
    json.create(steps, dir, btn, jsonStr);

    Serial.println(jsonStr);

    delay(5);
}
