# Arduino-SIM5320-MQTT
An arduino library for communication with 3G SIM5320 chipset, and implementation of MQTT protocol using AT commands

## Usage
See example1.ino for usage example

Note: Receiving messages from broker on subscribed topics is still under development. Coming shortly with examples.


## Functions 
Contains functions for basic MQTT actions.

MQTT Open:
Opens TCP port on SIM5320 at given url and port. Must be called first before other functions are used.

MQTT Connect:
Connects to broker with username and password, and verifies connection by listening for CONNACK response.

MQTT Subscribe:
Subscribes to a specified topic using SUBSCRIBE action and verifies subscription by listening for SUBACK response.

MQTT Publish:
Publishes specified message using PUBLISH action to connected broker.

MQTT Pingreq:
Pings broker with PINGREQ action and verifies response with PINGRESP action. Useful for keeping connection alive.

## SIM_Interface.ino
Functions for communicating reliably with SIM5320 chip. Backend use required for MQTT_Protocol.ino.

Must configure InitSim5320 function to work with sim card by specifying service provider. 'yesinternet' by default. 

Contains many useful functions such as reliably sending AT commands and reading SIM5320 buffer responses.
