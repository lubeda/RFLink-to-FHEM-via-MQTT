# Warning
This is a **beta** software, use it at your own risk!!!

# RFLink-to-FHEM-via-MQTT
[RFlink](http://rflink.nl) is a cool solution to work with 433 MHz gadgets, like remote-controlled Poweroutlets.

[FHEM](http://www.fhem.de) is a cool software for homeautomation.

Since there is no module in FHEM available i was searching for a way to use RFlink from FHEM.

My first try was [rflink-to-mqtt](https://github.com/Phileep/rflink-to-mqtt) which didn't work with FHEM like expected. But for [Openhab](http://www.openhab.org) it did ;-). 

So i created a new solution based on the module from @github/Phileep .

## Setup:

- You need a working RFLink
- Edit RFLINK-to-FHEM-via-MQTT.ino line 10-12 to your need.
- compile and upload to a ESP01 module
- Wire the ESP01 like shown in ESPRFLINKMQTT.png
- Use a mqtt client to subscribe to `RFlink\#` and see what happens
- Use your remote to send some commands to your devices or wait until your gadgets send some data

## How to use

when rflink receives something this is presented on the serial line eg.:

```
20;2A;Xiron;ID=2801;TEMP=0043;HUM=29;BAT=OK;
```

This software published this to the topics:

```
RFlink/JSON/Xiron/2801
```
as json like this:
```
{"raw":"20;2A;Xiron;ID=2801;TEMP=0043;HUM=29;BAT=OK;\r","TEMP":6.7,"HUM":29,"BAT":"OK"}
```

There is the raw string and the interpreted data with TEMP=6.7 in degree Celsius

For FHEM JSON is harder to parse, so my module publishes also this topics with the coresponding values:
```
RFlink/FHEM/Xiron/2801/TEMP
RFlink/FHEM/Xiron/2801/HUM
RFlink/FHEM/Xiron/2801/BAT
```

The coresponding FHEM-Device looks like this:
```
define Xiron_2801 MQTT_DEVICE
attr Xiron_2801 IODev Mosquitto195
attr Xiron_2801 autoSubscribeReadings RFlink/Flat/Xiron/2801/+
attr Xiron_2801 stateFormat TEMP °C
```

## How to send commands
### normal

publish the command according to the [documentation](http://www.rflink.nl/blog2/protref) to the topic

```
RFlink/command/
```

e.g. 
```
10;NewKaku;01dd77d5;1;OFF;
```

### FHEM
publish only "ON" or "OFF" to the topic build like this scheme:

RFlink/command/FHEM/NewKaku/01dd77d5/1

#### The FHEM-Device looks like this
```
define NewKaku_01dd77d5 MQTT_DEVICE
attr NewKaku_01dd77d5 IODev Mosquitto195
attr NewKaku_01dd77d5 autoSubscribeReadings RFlink/FHEM/NewKaku/01dd77d5/1/
attr NewKaku_01dd77d5 eventMap ON.on OFF:off
attr NewKaku_01dd77d5 publishSet ON OFF RFlink/command/FHEM/NewKaku/01dd77d5/1
attr NewKaku_01dd77d5 stateFormat state
attr NewKaku_01dd77d5 webCmd ON:OFF
```

# Watchout
This is a **beta** software, use it at your own risk!!!
