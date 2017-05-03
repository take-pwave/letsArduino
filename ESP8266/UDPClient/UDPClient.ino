/*
 *  This sketch sends a UDP message to a UDP nc server.
 *
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const char *ssid =  "WiFiのSSID";
const char *password =  "WiFiのパスワード";

#define SENDTO "255.255.255.255"

#define PORT 50000

void setup() {
    Serial.begin(115200);
    delay(500);

    WiFi.begin(ssid, password);
    WiFi.mode(WIFI_STA);

    Serial.println();
    Serial.println();
    Serial.print("Wait for WiFi... ");

    while(WiFi.status() != WL_CONNECTED) {
        Serial.print(WiFi.status());
        Serial.print(".");
        delay(500);
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    delay(500);
}


void loop() {
    WiFiUDP udp;

    udp.beginPacket(SENDTO, PORT);
    udp.println("Hello World!");
    udp.endPacket();
    delay(5000);
}


