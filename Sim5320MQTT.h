/*
  MQTT_Protocol.h - Library for MQTT protocol on SIM5320.
  Created by Boris Deletic, October 30, 2018.
  Released into the public domain.
*/

#ifndef SimMQTT_h
#define SimMQTT_h

#include "Arduino.h"

class SimMQTT
{
	public:
		SimMQTT(const int simSerialRX, const int simSerialTX, const int simPowerPin);
		void genRandomID(char *s, const int len);
		bool MqttOpen(const char* const brokerUrl, const char* const brokerPort);
		bool MqttConnect(const char* clientId, const char* username, const char* password);
		bool MqttSubscribe(const char* topic);
		bool MqttPublish(const char* const topic, const char* const msg);
		bool MqttPingreq();
		void setLogging(HardwareSerial &refSer, bool verbosity);
	private:
		SoftwareSerial Sim5320; 
		bool _gDebug;
		bool verifyResponse(const char MQTT_ACK);
}

#endif