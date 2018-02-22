#include "Common.h"
#include "ArduinoJson.h"
#include "Rflink.h"

/*********************************************************************************
 * Global Variables
   /*********************************************************************************/
// main input / output buffers
char BUFFER [BUFFER_SIZE];
char JSON   [BUFFER_SIZE];

// message builder buffers
char MQTT_NAME[MAX_DATA_LEN];
char MQTT_ID  [MAX_DATA_LEN];
char MQTT_CHAN[MAX_CHANNEL_LEN];
char FIELD_BUF[MAX_DATA_LEN];


// Serial iterator counter
int CPT;
// reconnection attemps timer counter
long lastReconnectAttempt = 0;

SoftwareSerial mySerial(4, 2, false,BUFFER_SIZE+2); // RX, TX

WiFiClient espClient;
PubSubClient MQTTClient(MQTT_SERVER, MQTT_PORT, callback, espClient);

void setup_wifi() {

        delay(10);

        WiFi.begin(WIFI_SSID,WIFI_PASSWORD);

        while (WiFi.status() != WL_CONNECTED)
        {
                Serial.println(F("WIFI not connected."));
                delay(1000);
        }
}

/**
 * callback to handle rflink order received from MQTT subscribtion
 */
void callback(char* topic, byte* payload, unsigned int len) {
        mySerial.write(payload, len);
        mySerial.print(F("\r\n"));
        Serial.println(F("=== Mqtt packet ==="));
        Serial.print(F("message = "));
        Serial.write(payload, len);
        Serial.print(F("\r\n"));
}

/**
 * build MQTT channel name to pubish to using parsed NAME and ID from rflink message
 */
void buildMqttChannel(char *name, char *ID) {
        MQTT_CHAN[0] = '\0';
        strcat(MQTT_CHAN,MQTT_PUBLISH_CHANNEL);
        strcat(MQTT_CHAN,"/");
        strcat(MQTT_CHAN,MQTT_NAME);
        strcat(MQTT_CHAN,"/");
        strcat(MQTT_CHAN,MQTT_ID);;
        strcat(MQTT_CHAN,"\0");
}

/**
 * send formated messagee to serial
 */
void printToSerial() {
        Serial.println(F("=== rflink packet ==="));
        Serial.print(F("Raw data = ")); Serial.print(BUFFER);
        Serial.print(F("Mqtt pub = "));
        Serial.print(MQTT_CHAN);
        Serial.print("/ => ");
        Serial.println(JSON);
        Serial.println();
}


/**
 * try to connect to MQTT Server
 */
boolean MqttConnect() {

        // connect to Mqtt server and subcribe to order channel
        if (MQTTClient.connect(MQTT_RFLINK_CLIENT_NAME)) {
                MQTTClient.subscribe(MQTT_RFLINK_ORDER_CHANNEL);
        }

        // report mqtt connection status
        Serial.print(F("Mqtt connection state : "));
        Serial.println(MQTTClient.state());

        return MQTTClient.connected();
}


/*********************************************************************************
 * Classic arduino bootstrap
   /*********************************************************************************/
void setup() {

        // Open serial communications and wait for port to open:
        setup_wifi();

        Serial.begin(115200);
        Serial.println(F("Starting..."));
        while (!Serial) {
                ; // wait for serial port to connect. Needed for native USB port only
        }
        Serial.println(F("Init serial done"));

        // set the data rate for the SoftwareSerial port
        mySerial.begin(57600);
        Serial.println(F("Init software serial done"));


        Serial.print(F("WLAN IP : "));
        Serial.println(WiFi.localIP());

        MqttConnect();
        ArduinoOTA.begin();

        mySerial.println(F("10;status;"));
}


void pubFlatMqtt(char* channel,char* json)
{
        DynamicJsonBuffer jsonBuffer(BUFFER_SIZE);
        JsonObject& root = jsonBuffer.parseObject(json);
        char myTopic[40];

        // Test if parsing succeeds.
        if (!root.success()) {
                Serial.println("parseObject() failed");
                return;
        }

        for (auto kv : root) {
                Serial.print(kv.key);
                Serial.print(F(" => "));
                Serial.println(kv.value.as<char*>());
                strcpy(myTopic,channel);
                strcat(myTopic,"/");
                strcat(myTopic,kv.key);
                MQTTClient.publish(myTopic,kv.value.as<char*>());
        }

}

/*********************************************************************************
 * Main loop
   /*********************************************************************************/
void loop() {
        bool DataReady=false;
        // handle lost of connection : retry after 2s on each loop
        if (!MQTTClient.connected()) {
                Serial.println(F("Not connected, retrying in 2s"));
                delay(2000);
                MqttConnect();
        } else {
                // if something arrives from rflink
                if(mySerial.available()) {
                        // bufferize serial message
                        while(mySerial.available() && CPT < BUFFER_SIZE) {
                                BUFFER[CPT] = mySerial.read();
                                CPT++;
                                if (BUFFER[CPT-1] == '\n') {
                                        DataReady = true;
                                        BUFFER[CPT]='\0';
                                        CPT=0;
                                }
                        }
                        if (CPT > BUFFER_SIZE ) CPT=0;
                }

                // parse what we just read
                if (DataReady) {
                        readRfLinkPacket(BUFFER);
                        // construct channel name to publish to
                        buildMqttChannel(MQTT_NAME, MQTT_ID);
                        // publish to MQTT server
                        MQTTClient.publish(MQTT_CHAN,JSON);

                        // report message for debugging
                        printToSerial();

                        pubFlatMqtt(MQTT_CHAN,JSON);

                }

                MQTTClient.loop();
                ArduinoOTA.handle();
        }
}
