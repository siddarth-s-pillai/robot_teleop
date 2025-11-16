#ifndef JSON_ENCODER_H
#define JSON_ENCODER_H

#include <Arduino.h>
#include <ArduinoJson.h>

class JsonEncoder {
public:
    JsonEncoder();

    void create(long steps, int dir, bool btn, String &output);

private:
    StaticJsonDocument<200> doc;
};

#endif
