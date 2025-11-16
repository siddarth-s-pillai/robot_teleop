#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

IPAddress local_IP(192,168,4,22);
IPAddress gateway(192,168,4,9);
IPAddress subnet(255,255,255,0);

WiFiUDP Udp; // Make global so available in loop()
unsigned int udpPort = 4210; // Predefined port
char replyPacket[] = "Hi there! Got the message :-)";
  
void printAndSendToConnectedClients() {
  struct station_info *stat_info;
  stat_info = wifi_softap_get_station_info();

  while (stat_info != NULL) {
    IPAddress ip = IPAddress((uint32_t)(stat_info->ip.addr));
    Serial.print("Client IP: ");
    Serial.println(ip);

    // Send UDP to this client
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

  Serial.print("Setting soft-AP configuration ... ");
  Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");

  Serial.print("Setting soft-AP ... ");
  boolean result = WiFi.softAP("ESPsoftAP_01", "qwerty246810");
  Serial.println(result ? "Ready" : "Failed!");

  Udp.begin(udpPort); // Start UDP service on this port
}

void loop() {
  Serial.printf("Stations connected = %d\n", WiFi.softAPgetStationNum());
  printAndSendToConnectedClients(); // Send to all connected stations
  delay(3000);
}
