# RFLink-to-FHEM-via-MQTT
[RFLink](http://rflink.nl) is a cool solution to work with 433 MHz gadgets, like remote-controlled Poweroutlets.

[FHEM](http://www.fhem.de) is a cool software for homeautomation.

Since there is no module in FHEM available i was searching for a way to use RFLink from FHEM.

My first try was [RflinkToJsonMqtt](https://github.com/jit06/RflinkToJsonMqtt) which didn't work with FHEM like expected.

So i created a new solution based on the module from @github/jit06 .

Use it at your own risk!

## Setup:

- You need a working RFLink-Hardware
- Edit common.h line 23-36 to your need. (SSID,Wifi Password and Broker)
- compile and upload to a NodeMCU or ESP8266 Board
- Wire the NodeMCU like shown in ESPRFLINKMQTT.png
- Use a mqtt client to subscribe to `RFLink\#` and see what happens
- Use your remote to send some commands to your devices or wait until your gadgets send some data
- The Hardware Serial from your ESP is used for debugging, the software-serial used to connect to the rflink

## How to use

when RFLink receives something this is presented on the serial line eg.:

```
20;2A;Xiron;ID=2801;TEMP=0043;HUM=29;BAT=OK;
```

This software published this to the topics:

```
RFLink/Xiron/2801
```
as json like this:
```
{TEMP:"6.7",HUM:"29",BAT:"OK"};
```

This is the raw string

For FHEM this is hard to parse, so my module publishes also this topics with the coresponding values:
```
RFLink/Xiron/2801/TEMP
RFLink/Xiron/2801/HUM
RFLink/Xiron/2801/BAT
```

The corresponding FHEM-Device looks like this:
```
define Xiron_2801 MQTT_DEVICE
attr Xiron_2801 IODev Mosquitto195
attr Xiron_2801 autoSubscribeReadings RFLink/Xiron/2801/+
attr Xiron_2801 stateFormat TEMP °C
```

## How to send commands
### normal

publish the command according to the [documentation](http://www.rflink.nl/blog2/protref) to the topic

```
RFLink/command
```

e.g.
```
10;NewKaku;01dd77d5;1;OFF;
```

# Watchout
This is a **beta** software, use it at your own risk!!!
