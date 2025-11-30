#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>




class WiFiSetup {
public:
    WiFiSetup() = default;
    bool configureSoftAP(const char* ssid, const char* password);
    void printAndSendToAllConnectedClients(const char* message);
    int getNumConnections();
    void sendBinaryToAllConnectedClients(const uint8_t* data, size_t len);
private:
    WiFiUDP Udp;
    unsigned int udpPort = 20025;
    char replyPacket[30] = "Hi there! Got the message :-)";
    IPAddress local_IP{192, 168, 4, 22};
    IPAddress gateway{192, 168, 4, 9};
    IPAddress subnet{255, 255, 255, 0};
};

#endif` // WIFI_SETUP_H
