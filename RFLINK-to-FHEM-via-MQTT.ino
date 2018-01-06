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
boolean debug =false;

// See Details at https://github.com/lubeda/RFLink-to-FHEM-via-MQTT

//Design Config
const char* mqtt_prefix_JSON = "RFlink/JSON/"; // Topic Base
const char* statusTopic = "RFlink/status";
String commandTopic = "RFlink/command/";
String debugTopic = "RFlink/debug";
const char* mqtt_prefix_Flat = "RFlink/FHEM/"; // Topic Base
const char* client_name = "espRFlink"; // production version client name for MQTT login - must be unique on your system

// Key User Configuration here:
SoftwareSerial swSer(0, 2, false, 256); // d5 & d6 on the nodu MC v1.0

const byte numChars = 128;
char receivedChars[numChars];
char tempChars[numChars];        // temporary array for use when parsing
char tempString[30]; // COnver float to String
char messageFromPC[numChars] = {0};
char RFName[30]; // name of protocol from RFLINK
char RFID[30]; //ID from RFLINK ie ID=XXXX;
char RFData[numChars]; //the rest from RFLINK - will include one or many pieces of data
char RFDataTemp[numChars]; //temporary area for processing RFData - when we look for temp and convert etc
String MQTTTopic = "";
String FHEMMQTTTopic = "";

boolean willRetain = true;
const char*  willMessage = "offline";

const float TempMax = 80.0; // max temp - if we get a value greater than this, ignore it as an assumed error
const int HumMax = 101; // max hum - if we get a value greater than this, ignore it as an assumed error

String CompassDirTable[17] = {"N","NNE","NE","ENE","E","ESE", "SE","SSE","S","SSW","SW","WSW", "W","WNW","NW","NNW","N"};

WiFiClient espClient;
PubSubClient MQTTclient(espClient);

boolean newData = false;

void Debug(String T,String M)
{
        if (debug)
        {
                MQTTclient.publish(debugTopic.c_str(),(T+": "+M).c_str()); // got someting
        }
}

void setup_wifi() {

        delay(10);
        // We start by connecting to a WiFi network
        Serial.println();
        Serial.print("Connecting to ");
        Serial.println(ssid);

        WiFi.begin(ssid, password);

        while (WiFi.status() != WL_CONNECTED)
        {
                delay(500);
                Serial.print(".");
        }

        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
}

void callback(const char* topic, byte* payload, unsigned int length) {
// Irgendetwas kommt unter mqtt_prefix_JSON+commandTopic  rein => wird ohne Prüfung gesendet
        String result;
        payload[length] = '\0'; // terminate payload
        String strPayload = ((char*)payload);

        Serial.println("Command coming in!: "); // got someting

        Debug("Callback IN-CMD",String(topic) +"=>" +String(strPayload));

        if (String(topic).length() == String(commandTopic).length() ) {
                swSer.println("\r"+strPayload+"\r"); // \n snd data to the RFLink
                Debug("Callback Send Default",strPayload);
                MQTTclient.publish(statusTopic,result.c_str(),false); // got someting
        } else
        {
                // topic = commandTopic + FHEM
                String command = String(topic).substring(commandTopic.length()+5);
                command.replace("/",";");

                result= "10;" + command +";" + strPayload + ";" +"\r\n"; // \n
                swSer.println("\n"+result+"\n"); // snd data to the RFLink
                Debug("Callback Send FHEM",result);
        }

}

void reconnect() {
        // Loop until we're reconnected
        while (!MQTTclient.connected()) {
                Serial.print("Attempting MQTT connection...");
                // Attempt to connect
                if (MQTTclient.connect(client_name, statusTopic, 0, willRetain, willMessage)) {
                        //PubSubClient::connect(const char*, const char*, uint8_t, boolean, const char*)
                        Serial.println("connected");
                        // Once connected, update status to online - will Message will drop in if we go offline ...
                        MQTTclient.publish(statusTopic,"online",true);

                        MQTTclient.subscribe(String(commandTopic+"#").c_str());// subscribe to the command topic - will listen here for comands to the RFLink

                } else {
                        Serial.print("failed, rc=");
                        Serial.print(MQTTclient.state());
                        Serial.println(" try again in 5 seconds");
                        // Wait 5 seconds before retrying
                        delay(5000);
                }
        }
}

void setup() {

// JSON parsing library setup
        DynamicJsonBuffer jsonBuffer;

        JsonObject& root = jsonBuffer.createObject();

        Serial.begin(57600);
        swSer.begin(57600); // this is the baud rate of the RF LINK

        setup_wifi();
        MQTTclient.setServer(mqtt_server, 1883);
        MQTTclient.setCallback(callback);

        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
}

void recvWithStartEndMarkers() {
        static byte ndx = 0;
        char endMarker = '\n';
        char rc;

        while (swSer.available() > 0 && newData == false) {
                rc = swSer.read();

                if (rc != endMarker) {
                        receivedChars[ndx] = rc;
                        ndx++;
                        if (ndx >= numChars) {
                                ndx = numChars - 1;
                        }
                }
                else {
                        receivedChars[ndx] = '\0'; // terminate the string
                        ndx = 0;
                        newData = true;
                }
        }
}

float hextofloat(char* hexchars) {
        return float(strtol(hexchars,NULL,16));
}
int hextoint(char* hexchars) {
        return strtol(hexchars,NULL,16);
}

void parseData() {      // split the data into its parts
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();

        char * strtokIndx; // this is used by strtok() as an index
        float tmpfloat = 0.0; // temporary float used in tests
        int tmpint = 0; // temporary int used in tests
        String value;

        strtokIndx = strtok(tempChars,";");  // get the first part - the string
        strcpy(messageFromPC, strtokIndx); // copy it to messageFromPC

        Debug("parseData msgfromPC",messageFromPC);

        if (strcmp(messageFromPC,"20") == 0 ) { // 20 means a message recieved from RFLINK - this is what we are interested in breaking up for use

                strtokIndx = strtok(NULL, ";" ); // this continues where the previous call left off - which we will ignore as it is the packet count
                strtokIndx = strtok(NULL, ";" ); // this should be the name
                strcpy( RFName, strtokIndx ); // copy name to RFName

                strtokIndx = strtok(NULL, ";");
                strcpy( RFID, strtokIndx+3); // copy next block to RFID

                strtokIndx = strtok(NULL, "\n"); // search for the rest of the buffer input ending in \n
                strcpy(RFData, strtokIndx ); // copy remainder of block to RFData
                // now find if we have TEMP= in RFData and convert from hex to int
                strcpy(RFDataTemp, RFData ); // temp copy of RFData so we can search for temp and convert from hex to decimal

                root["raw"] = receivedChars; // copy the raw data to the json in case we need to debug

                // Read each command pair
                char* command = strtok(RFDataTemp, ";");

                while (command != 0 and strlen(command)>4 )
                {
                        // Split the command in two values
                        char* separator = strchr(command, '=');
                        if (separator != 0)
                        {
                                // Actually split the string in 2: replace '=' with 0
                                *separator = 0;
                                String NamePart = command;
                                ++separator;
                                if (NamePart == "TEMP") { // test if it is TEMP, which is HEX
                                        tmpfloat = hextofloat(separator)*0.10; //convert from hex to float and divide by 10 - using multiply as it is faster than divide
                                        if (tmpfloat < TempMax) //test if we are inside the maximum test point - if not, assume spurious data
                                        {
                                                root.set<float>( NamePart,tmpfloat ); // passed spurious test - add data to root
                                        }
                                } /// end of TEMP block
                                else if (NamePart == "HUM") { // test if it is HUM, which is int
                                        if (strcmp(RFName,"DKW2012") == 0 ) { // digitech weather station - assume it is a hex humidity, not straight int
                                                tmpint = hextoint(separator);
                                        }
                                        else {
                                                tmpint = atoi(separator);
                                        }      // end of setting tmpint to the value we want to use & test
                                        if (tmpint > 0 and tmpint < HumMax) //test if we are inside the maximum test point - if not, assume spurious data
                                        {
                                                root.set<int>( NamePart,tmpint);
                                        }                         // passed the test - add the data to rot, otherwise it will not be added as spurious
                                } // end of HUM block
                                else if (NamePart == "RAIN") // test if it is RAIN, which is HEX
                                {
                                        root.set<float>( NamePart,hextofloat(separator)*0.10);

                                } //  - add data to root
                                else if (NamePart == "WINSP") // test if it is WINSP, which is HEX
                                {
                                        root.set<float>( NamePart,hextofloat(separator)*0.10 );

                                }                                 //  - add data to root
                                else if (NamePart == "WINGS") // test if it is WINGS, which is HEX
                                {
                                        root.set<float>( NamePart,hextofloat(separator)*0.10 );
                                }                                                           //  - add data to root
                                else if (NamePart == "WINDIR") // test if it is WINDIR, which is 0-15 representing the wind angle / 22.5 - convert to compas text
                                {
                                        root[NamePart] = CompassDirTable[atoi(separator)];
                                }
                                else // check if an int, add as int, else add as text
                                if (atoi(separator) == 0) // not an integer
                                {root[NamePart] = separator;} // do normal string add
                                else
                                {root.set<int>( NamePart, atoi(separator) );}// do int add
                        }
                        // Find the next command in input string
                        command = strtok(NULL, ";");
                }
        }
        else { // not a 20 code- something else
                Serial.println("doing the else - not a 20 code ");
                strcpy(RFData, strtokIndx ); // copy all of it to RFData
                strcpy( RFName, "unknown" );
                strcpy( RFID, "");
        }

        MQTTTopic = mqtt_prefix_JSON;
        MQTTTopic += String(RFName);
        MQTTTopic += "/";
        MQTTTopic += String(RFID);

        FHEMMQTTTopic = mqtt_prefix_Flat;
        FHEMMQTTTopic += String(RFName);
        FHEMMQTTTopic += "/";
        FHEMMQTTTopic += String(RFID);

        if (root.containsKey("SWITCH")) {
                Debug("Parsedata Switch",root["SWITCH"]);
                FHEMMQTTTopic += "/" + root["SWITCH"].as<String>();
                MQTTTopic += "/" + root["SWITCH"].as<String>();
        }

        String json;
        root.printTo(json);

        MQTTclient.publish(MQTTTopic.c_str(), json.c_str() );
        if (root.containsKey("SWITCH")) {
                root.remove("SWITCH");
        }
//FHEM Ausgabe:
        if ( strcmp(RFName,"STATUS" )) // STATUS will be droped
        {
                root.remove("raw");
                Debug("Parsedata FHEM Topic",FHEMMQTTTopic);
                for (auto kv : root) {
                        value =String("Debug: ") + kv.value.as<String>() +" " + String(FHEMMQTTTopic) +"/"+ kv.key;
                        Debug(String("Parsedata Keys->") + kv.key,value);
                        value = kv.value.as<String>();
                        MQTTclient.publish(String(FHEMMQTTTopic+"/"+kv.key).c_str(),value.c_str());
                }
        }
}

void loop() {
        recvWithStartEndMarkers();
        if (newData == true) {
                strcpy(tempChars, receivedChars);
                // this temporary copy is necessary to protect the original data
                //   because strtok() used in parseData() replaces the commas with \0
                parseData(); // JSON Publish
                newData = false;
        }
        if (!MQTTclient.connected()) {
                reconnect();
        }
        MQTTclient.loop();
}
