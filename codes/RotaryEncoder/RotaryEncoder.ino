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

    // GET ALL VALUES IN ONE CALL
    RotaryData enc = encoder.getRotaryEncoderData();

    String jsonStr;
    json.create(enc.steps, enc.direction, enc.button, jsonStr);

    Serial.println(jsonStr);

    delay(10);
}
