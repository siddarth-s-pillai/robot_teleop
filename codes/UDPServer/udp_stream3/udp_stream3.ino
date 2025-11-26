#include "wifi_setup.hpp"

#define LED_PIN 2 // NodeMCU onboard LED (D4, GPIO2)
WiFiSetup wifiSetup;
char payload[] = "Hi there! Got the message :-)";

void setup() {
  Serial.begin(115200);
  Serial.println();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // Turn LED off initially (active LOW)
  bool res = false;
  bool flip = false;
  while (!res){
    digitalWrite(LED_PIN, flip ? LOW : HIGH);
    flip = !flip;
    res = wifiSetup.configureSoftAP("ESPsoftAP_01", "qwerty246810");
    delay(500);  
  }
}

void loop() {
  int stations = wifiSetup.getNumConnections();
  Serial.printf("Stations connected = %d\n", stations);
  wifiSetup.printAndSendToAllConnectedClients(payload);

  // Blink logic
  if (stations == 0) {
    // Fast blink (100 ms on/off)
    digitalWrite(LED_PIN, LOW); // Turn ON (active LOW)
    delay(100);
    digitalWrite(LED_PIN, HIGH); // Turn OFF
    delay(100);
  } else {
    // Slow blink (600 ms on, 600 ms off)
    digitalWrite(LED_PIN, LOW); // Turn ON
    delay(600);
    digitalWrite(LED_PIN, HIGH); // Turn OFF
    delay(600);
  }
}
