/*
  SIM_Interface.h - Library for MQTT protocol on SIM5320.
  Created by Boris Deletic, October 30, 2018.
  Released into the public domain.
*/

#ifndef SimInterface_h
#define SimInterface_h

#include <Arduino.h>
#include <Sim5320MQTT.h>
#include <SoftwareSerial.h>

class SimInterface
{
	public:
		SimInterface(SoftwareSerial* pntSim, const int simPowerPin, String provider, String IP);
		bool sendATcommand(String cmd, String expected = "OK", unsigned int timeout = 1000, bool openTCP = false);
		bool CheckOk (void);
		bool verifyResponse(const char MQTT_ACK);
		void setLogging(Stream* pntSer, bool verbosity);
		String byteToHexStr (const uint8_t value, const String prefix = " 0x");
		uint8_t ReadSim5320 (bool print /*= false*/);
		void InitSim5320 (void);
		void InitWeb (void);
	private:
		String network;
		String netIP;
		char gRxMsg[150];
		const int powerPin;
		bool _gDebug;
		Stream* logSer;
		SoftwareSerial* Sim5320; 		

};

#endif