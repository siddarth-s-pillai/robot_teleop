#include "wifi_setup.hpp"

bool WiFiSetup::configureSoftAP(const char* ssid, const char* password) {
    Serial.print("Setting soft-AP configuration ... ");
    bool configResult = WiFi.softAPConfig(local_IP, gateway, subnet);
    Serial.println(configResult ? "Ready" : "Failed!");

    Serial.print("Setting soft-AP ... ");
    bool apResult = WiFi.softAP(ssid, password);
    Serial.println(apResult ? "Ready" : "Failed!");

    Udp.begin(udpPort);

    if (!configResult || !apResult) {
        Serial.println("Error configuring Soft AP.");
    }
    return configResult && apResult;
}

void WiFiSetup::printAndSendToAllConnectedClients(const char* message) {
    struct station_info *stat_info;
    stat_info = wifi_softap_get_station_info();
    Serial.print("message: ");
    Serial.println(message);

    while (stat_info != NULL) {
        IPAddress ip = IPAddress((uint32_t)(stat_info->ip.addr));
        Serial.print("Client IP: ");
        Serial.println(ip);

        Udp.beginPacket(ip, udpPort);
        Udp.write(message);
        Udp.endPacket();

        stat_info = STAILQ_NEXT(stat_info, next);
    }
    wifi_softap_free_station_info();
}

int WiFiSetup::getNumConnections() {
    return WiFi.softAPgetStationNum();
}


void WiFiSetup::sendBinaryToAllConnectedClients(const uint8_t* data, size_t len) {
    struct station_info *stat_info;
    stat_info = wifi_softap_get_station_info();

    while (stat_info != NULL) {
        IPAddress ip = IPAddress((uint32_t)(stat_info->ip.addr));
        
        Udp.beginPacket(ip, udpPort);
        Udp.write(data, len); // Send 'len' bytes from 'data'
        Udp.endPacket();

        stat_info = STAILQ_NEXT(stat_info, next);
    }
    wifi_softap_free_station_info();
}
