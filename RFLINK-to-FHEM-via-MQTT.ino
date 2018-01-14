#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>

// Network Config
const char* ssid = "MyWiFi"; // network SSID for ESP8266 to connect to
const char* password = "MyWiFi-Password"; // password for the network above
const char* mqtt_server = "192.168.178.195"; // address of the MQTT server that we will communicte with */
char* client_name = "espRFLink"; // production version client name for MQTT login - must be unique on your system
SoftwareSerial swSer(4, 2, false, 256); // RX=GIO04 & TX=GPIO2

const char buffersize=240;
char OutData[buffersize];

boolean DataReady=false;

const char*  statusTopic="RFLink/status";
String commandTopic="RFLink/command/";
const char*  dataTopic="RFLink/data";

WiFiClient espClient;
PubSubClient MQTTClient(espClient);

// array of wind directions - used by weather stations. output from the RFLink under WINDIR is an integr 0-15 - will lookup the array for the compass text version
String CompassDirTable[17] = {"N","NNE","NE","ENE","E","ESE", "SE","SSE","S","SSW","SW","WSW", "W","WNW","NW","NNW","N"};
String HStatusTable[4] = {"Normal","Comfortable","Dry","Wet"};
String BForecastTable[5] = {"No Info","Sunny","Partly Cloudy","Cloudy","Rain"};

void setup_wifi() {

        delay(10);

        WiFi.begin(ssid, password);

        while (WiFi.status() != WL_CONNECTED)
        {
                delay(500);
        }
}

void callback(const char* topic, byte* payload, unsigned int length) {
// Irgendetwas kommt unter mqtt_prefix_JSON+commandTopic  rein => wird ohne Prüfung gesendet
        payload[length] = '\0'; // terminate payload
        String result;
        String strPayload = ((char*)payload);
        String StrTopic = String(statusTopic)+"/in";

        if (String(topic).length() == commandTopic.length() ) {
                swSer.println("\n"+strPayload+"\n"); // \n snd data to the RFLink
                MQTTClient.publish(StrTopic.c_str(),strPayload.c_str());
        } else
        {
                // topic = commandTopic + FHEM
                String command = String(topic).substring(commandTopic.length());
                command.replace("/",";");

                result= "10;" + command +";" + strPayload + ";" +"\r\n"; // \n
                swSer.println("\n"+result+"\n"); // snd data to the RFLink
                MQTTClient.publish(StrTopic.c_str(),result.c_str());
        }
}

void reconnect() {
        // Loop until we're reconnected
        while (!MQTTClient.connected()) {
                // Attempt to connect
                if (MQTTClient.connect(client_name, statusTopic, 0, true, "offline")) {
                        // Once connected, update status to online - will Message will drop in if we go offline ...
                        MQTTClient.publish(statusTopic,"online",true);

                        MQTTClient.subscribe((commandTopic+"#").c_str());// subscribe to the command topic - will listen here for comands to the RFLink

                } else {
                        // Wait 5 seconds before retrying
                        delay(5000);
                }
        }
}

void setup() {
        swSer.begin(57600); // this is the baud rate of the RF LINK

        setup_wifi();
        MQTTClient.setServer(mqtt_server, 1883);
        MQTTClient.setCallback(callback);
}

void recvWithStartEndMarkers() {
        static byte ndx = 0;
        char endMarker = '\n';
        char rc;

        while (swSer.available() > 0 && DataReady == false) {
                rc = swSer.read();

                if (rc != endMarker) {

                        OutData[ndx] = rc;
                        ndx++;
                        if (ndx >= buffersize) {
                                ndx = buffersize - 1;
                        }
                }
                else {
                        OutData[ndx] = '\0'; // terminate the string
                        ndx = 0;
                        DataReady = true;
                }
        }
}

void parseData()
{
        float tmpfloat = 0.0; // temporary float used in tests
        int tmpint = 0; // temporary int used in tests
        int iID=-1;
        int iSWITCH=-1;
        int iRest=-1;
        int iValue;
        String StrTopic,StrValue,StrName,BaseTopic;
        String StrWork,StrID,StrSWITCH,StrMode;

        // Text gesammt

        BaseTopic = statusTopic+String("/out");
        MQTTClient.publish(BaseTopic.c_str(), OutData);

        StrWork = String(OutData);
        if ((StrWork.indexOf("STATUS;")>=0) || (StrWork.indexOf("DEBUG;")>=0) {
            MQTTClient.publish(statusTopic,StrWork.substring(6).c_str());
            StrWork="";
        }

        if (StrWork.startsWith("20"))
        {
                BaseTopic = statusTopic;
                BaseTopic += "/";
                iRest = 6;
                StrMode = StrWork.substring(iRest,StrWork.indexOf(";",6));

                iID = StrWork.indexOf("ID=");
                if (iID >=0) {
                        iRest = StrWork.indexOf(";",iID);
                        StrID= StrWork.substring(iID+3,iRest);
                        ++iRest;
                        BaseTopic = String(dataTopic)+"/"+ StrMode + "/"+StrID+"/";
                        iSWITCH = StrWork.indexOf("SWITCH=");
                }
                if (iSWITCH >=0) {
                        iRest = StrWork.indexOf(";",iSWITCH);
                        StrSWITCH= StrWork.substring(iSWITCH+7,iRest);
                        ++iRest;
                        BaseTopic += StrSWITCH +"/";
//                      MQTTClient.publish(BaseTopic.c_str(), StrSWITCH.c_str());
                }

                StrWork =StrWork.substring(iRest);

                if ( StrMode.startsWith("Nodo ") || (StrMode.indexOf("=")>=0) ) {
                        MQTTClient.publish(statusTopic, StrWork.c_str());
                        StrWork = String("");
                }

                while ((StrWork.length()>1)) { // 0x0d

                        iValue = StrWork.indexOf("=");

                        if (iValue >= 0) {
                                iRest = StrWork.indexOf(";");
                                StrValue = StrWork.substring(iValue+1,iRest);
                                StrName =  StrWork.substring(0,iValue);
                                ++iRest;
                                StrTopic = BaseTopic + StrName;
                                if ((StrName == "TEMP") || (StrName == "RAIN") || (StrName == "RAINRATE") || (StrName == "WINSP") || (StrName == "AWINSP") ) {
                                        tmpfloat = strtol(StrValue.c_str(),NULL,16)*0.10; //convert from hex to float and divide by 10 - using multiply as it is faster than divide
                                        StrValue = String(tmpfloat);
                                } else if   (StrName == "WINDIR") {                         // test if it is HUM, which is int
                                        StrValue = CompassDirTable[atoi(StrValue.c_str())];
                                } else if   (StrName == "HSTATUS") {                         // test if it is HUM, which is int
                                              StrValue = HStatusTable[atoi(StrValue.c_str())];
                                } else if   (StrName == "BFORECAST") {                         // test if it is HUM, which is int
                                        StrValue = BForecastTable[atoi(StrValue.c_str())];
                                } else if ((StrName == "BARO") || (StrName == "KWATT") ||(StrName == "WATT") || (StrName == "UV") || (StrName == "LUX")|| (StrName == "WINGS") ){
                                          tmpint = strtol(StrValue.c_str(),NULL,16);
                                          StrValue = String(tmpint);
                                        }

                                MQTTClient.publish(StrTopic.c_str(), StrValue.c_str());
                        } else {
                                MQTTClient.publish(BaseTopic.c_str(), StrWork.c_str());
                        }
                        StrWork = StrWork.substring(iRest);
                }
        }
}

void loop() {
        recvWithStartEndMarkers();
        if (DataReady == true) {
                parseData();
                DataReady = false;
        }
        if (!MQTTClient.connected()  ) {
                reconnect();
        }
        MQTTClient.loop();
}
