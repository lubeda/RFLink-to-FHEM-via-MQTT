# RFLink-to-FHEM-via-MQTT
[RFLink](http://rflink.nl) is a cool solution to work with 433 MHz gadgets, like remote-controlled Poweroutlets.

[FHEM](http://www.fhem.de) is a cool software for homeautomation.

Since there is no module in FHEM available i was searching for a way to use RFLink from FHEM.

My first try was [rflink-to-mqtt](https://github.com/Phileep/rflink-to-mqtt) which didn't work with FHEM like expected.

So i created a new solution based on the module from @github/Phileep .

## Setup:

- You need a working RFLink-Hardware
- Edit RFLINK-to-FHEM-via-MQTT.ino line 10-12 to your need. (SSID,Wifi Password and ports of the softwareserial)
- compile and upload to a ESP01 module or ESP8266 Board
- Wire the ESP01 like shown in ESPRFLINKMQTT.png
- Use a mqtt client to subscribe to `RFLink\#` and see what happens
- Use your remote to send some commands to your devices or wait until your gadgets send some data

## How to use

when RFLink receives something this is presented on the serial line eg.:

```
20;2A;Xiron;ID=2801;TEMP=0043;HUM=29;BAT=OK;
```

This software published this to the topics:

```
RFLink/status/out
```
as json like this:
```
20;2A;Xiron;ID=2801;TEMP=0043;HUM=29;BAT=OK;
```

This e is the raw string

For FHEM this is hard to parse, so my module publishes also this topics with the coresponding values:
```
RFLink/data/Xiron/2801/TEMP
RFLink/data/Xiron/2801/HUM
RFLink/data/Xiron/2801/BAT
```

The corresponding FHEM-Device looks like this:
```
define Xiron_2801 MQTT_DEVICE
attr Xiron_2801 IODev Mosquitto195
attr Xiron_2801 autoSubscribeReadings RFLink/data/Xiron/2801/+
attr Xiron_2801 stateFormat TEMP °C
```

## How to send commands
### normal

publish the command according to the [documentation](http://www.rflink.nl/blog2/protref) to the topic

```
RFLink/command/
```

e.g.
```
10;NewKaku;01dd77d5;1;OFF;
```

### FHEM
publish only "ON" or "OFF" to the topic build like this scheme:

RFlink/command/NewKaku/01dd77d5/1

#### The FHEM-Device looks like this
```
defmod NewKaku_01dd77d5 MQTT_DEVICE
attr NewKaku_01dd77d5 DbLogExclude .*
attr NewKaku_01dd77d5 IODev Mosquitto195
attr NewKaku_01dd77d5 autoSubscribeReadings RFLink/data/NewKaku/01dd77d5/1/+
attr NewKaku_01dd77d5 eventMap ON.on OFF:off
attr NewKaku_01dd77d5 publishSet ON OFF RFLink/command/FHEM/NewKaku/01dd77d5/1
attr NewKaku_01dd77d5 stateFormat state
attr NewKaku_01dd77d5 webCmd ON:OFF
```

# Watchout
This is a **beta** software, use it at your own risk!!!
