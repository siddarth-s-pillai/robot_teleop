#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define LED_PIN 2 // NodeMCU onboard LED (D4, GPIO2)

IPAddress local_IP(192,168,4,22);
IPAddress gateway(192,168,4,9);
IPAddress subnet(255,255,255,0);

WiFiUDP Udp;
unsigned int udpPort = 20025;
char replyPacket[] = "Hi there! Got the message :-)";

void printAndSendToConnectedClients() {
  struct station_info *stat_info;
  stat_info = wifi_softap_get_station_info();

  while (stat_info != NULL) {
    IPAddress ip = IPAddress((uint32_t)(stat_info->ip.addr));
    Serial.print("Client IP: ");
    Serial.println(ip);

    Udp.beginPacket(ip, udpPort);
    Udp.write(replyPacket);
    Udp.endPacket();

    stat_info = STAILQ_NEXT(stat_info, next);
  }
  wifi_softap_free_station_info();
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // Turn LED off initially (active LOW)

  Serial.print("Setting soft-AP configuration ... ");
  Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");

  Serial.print("Setting soft-AP ... ");
  boolean result = WiFi.softAP("ESPsoftAP_01", "qwerty246810");
  Serial.println(result ? "Ready" : "Failed!");

  Udp.begin(udpPort);
}

void loop() {
  int stations = WiFi.softAPgetStationNum();
  Serial.printf("Stations connected = %d\n", stations);
  printAndSendToConnectedClients();

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
