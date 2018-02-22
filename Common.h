#ifndef H_COMMON
#define H_COMMON

#include <SoftwareSerial.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

/*********************************************************************************
 * Global parameters
/*********************************************************************************/
// Maximum size of one serial line and json content
#define BUFFER_SIZE 200
// Maximum size for record name, record id, json field name and json field value
#define MAX_DATA_LEN 24
// Maximum channel path size (at least lenght of MQTT_PUBLISH_CHANNEL + 2 x MAX_DATA_LEN)
#define MAX_CHANNEL_LEN 60
// default MQTT server port
#define MQTT_PORT 1883
// default MQTT Server
#define MQTT_SERVER "192.168.178.99"

// network SSID for ESP8266 to connect to
#define WIFI_SSID "WLAN"
// password for the network above
#define WIFI_PASSWORD "12345678"

// default MQTT Channel to publish to
#define MQTT_PUBLISH_CHANNEL "rflink"
// Client name sent to MQTT server
#define MQTT_RFLINK_CLIENT_NAME "rflinkClient"
// MQTT channel to listen to for rflink order
#define MQTT_RFLINK_ORDER_CHANNEL "rflink/command"


/*********************************************************************************
 * functions defined in scketch
/*********************************************************************************/
void callback(char* topic, byte* payload, unsigned int length);
void buildMqttChannel(char *name, char *ID);
void printToSerial();

#endif
