/*
  MQTT_Protocol.h - Library for MQTT protocol on SIM5320.
  Created by Boris Deletic, October 30, 2018.
  Released into the public domain.
*/
#include "Arduino.h"
#include "SimMQTT.h"

#define MQTT_MSG_CONNECT        0x10
#define MQTT_MSG_CONNACK        0x20
#define MQTT_MSG_PUBLISH        0x30
#define MQTT_SUBSCRIBE          0x82
#define MQTT_SUBACK             0x90
#define MQTT_PINGREQ            0xc0
#define MQTT_PINGRESP           0xd0

//////////////////////////////////////////////////////////////////////////
SimMQTT::SimMQTT(const int simSerialRX, const int simSerialTX, const int simPowerPin) {
	
	// Note on Sim900 baud rates. The unit defaults to 115200 but I have
    // found it to be unreliable. To change baud rate, issue 'AT+IPREX=115200'
    // or similar (change the baud rate number). After this command the sketch
    // cannot communicate with the Sim5320 any more. Change the next line to
    // match the new baud rate and restart.
    //
    // If echoing commands from the Serial to Sim5320 and vv (see looop()), it
    // is best if the two baud rates are identical.
    Sim5320.begin(19200);
	_gDebug = false;
	
	
	
}

void SimMQTT::setLogging(HardwareSerial &refSer, bool verbosity) {
	refSer.begin(cBaudRate);
	if (verbosity) {
		refSer.println("Enabled logging");
		_gDebug = true;
	} else {
		refSer.println("Disabled logging");
		_gDebug = false;
	}
}


//////////////////////////////////////////////////////////////////////////
// Generate random client ID, pass in char array to be modified
void SimMQTT::genRandomID(char *s, const int len) {
    static const char alphanum[] =
        "012346789"
        "ABCDEFGHIJKLMNOPQRSTUVWYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}

///////////////////////////////////////////////////////////////////////////
bool SimMQTT::MqttOpen (const char* const brokerUrl, const char* const brokerPort) {
    bool _gDebug = true;
    String cmd = String("AT+CIPOPEN=0,\"TCP\",\"") + brokerUrl + "\"," + brokerPort;
    for (int n=0; n=5; n++) {
        delay(500);
        if (sendATcommand(cmd, "OK", 10000)) {
          delay(1000);
          ReadSim5320(_gDebug);
          return true;
        }
        else {
          if (sendATcommand(cmd, "OK", 10000)) {
              delay(1000);
              ReadSim5320(_gDebug);
              return true;
          } else {
              sendATcommand("AT+CIPCLOSE=0", "OK", 10000);
          }
        }
    }
    _gDebug = false;
    return false;
}

///////////////////////////////////////////////////////////////////////////
bool SimMQTT::MqttConnect (const char* clientId, const char* username = '', const char* password = '') {

    //
    // See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html for details
    //
    
    uint8_t clientIdLength = strlen(clientId);
    uint8_t usernameLength = strlen(username);
    uint8_t passwordLength = strlen(password);

    uint8_t mqttMessage[128];
    uint8_t mqttMessageLength = 19 + clientIdLength + usernameLength + passwordLength;

    // Header
    mqttMessage[0] = MQTT_MSG_CONNECT;
    mqttMessage[1] = 17 + clientIdLength + usernameLength + passwordLength;    // Remaining length of the message (bytes 2-13 + clientId)

    // Protocol name
    mqttMessage[2] = 0;                      // Protocol Name Length MSB
    mqttMessage[3] = 4;                      // Protocol Name Length LSB
    mqttMessage[4] = 'M';
    mqttMessage[5] = 'Q';
    mqttMessage[6] = 'T';
    mqttMessage[7] = 'T';

    // Protocol level
    mqttMessage[8] = 4;                      // MQTT Protocol version = 4 (mqtt 3.1.1)

    // Connection flags   2 for clean session
 //   mqttMessage[9] = 2;
    mqttMessage[9] = 194;  //enable username, password, clean session

    // Keep-alive (maximum duration)
    mqttMessage[10] = 100;                     // Keep-alive Time Length MSB
    mqttMessage[11] = 100;                     // Keep-alive Time Length LSB

    // Client ID
    mqttMessage[12] = 0;                     // Client ID length MSB
    mqttMessage[13] = clientIdLength;        // Client ID length LSB
    memcpy(&mqttMessage[14], clientId, clientIdLength);

    // Username
    mqttMessage[14 + clientIdLength] = 0;                     // Client ID length MSB
    mqttMessage[15 + clientIdLength] = usernameLength;        // Client ID length LSB
    memcpy(&mqttMessage[16 + clientIdLength], username, usernameLength);

    // Password
    mqttMessage[16 + clientIdLength + usernameLength] = 0;                     // Client ID length MSB
    mqttMessage[17 + clientIdLength + usernameLength] = passwordLength;        // Client ID length LSB
    memcpy(&mqttMessage[18 + clientIdLength + usernameLength], password, passwordLength);

    
    delay(1000);
    // Prepare command
	if (_gDebug) {
		refSer.println("Tx: MQTT_CONNECT");
	}
    
    String cmd = "AT+CIPSEND=0,";
    cmd += mqttMessageLength;
    cmd += CR;

    // Send command
    ReadSim5320(_gDebug);              // clear rx buffer
    Sim5320.print(cmd);
    delay(100);
    // Send message
    for (int i=0; i<mqttMessageLength; i++) {
        Sim5320.write(mqttMessage[i]); // Message contents
		if (_gDebug) {
			refSer.print(byteToHexStr(mqttMessage[i]));
			refSer.println();
		}
        
    }

    // Check if send was successful
  delay(100);

  bool retVal = verifyResponse(MQTT_MSG_CONNACK);
  return retVal;
}

//////////////////////////////////////////////////////////////////////////
bool SimMQTT::MqttSubscribe (const char* const topic) {
  bool retVal = false;

  uint8_t lenTopic = strlen(topic);

  uint8_t mqttMessage[128];
  uint8_t written  = 0;

  // Write fixed header
  mqttMessage[written++] = MQTT_SUBSCRIBE;
  mqttMessage[written++] = 5 + lenTopic;

  //Variable Header
  mqttMessage[written++] = 0; // MSB
  mqttMessage[written++] = rand() % 256; // LSB message ID
  // TODO: make message id dynamic

  // Write topic
  mqttMessage[written++] = 0;         // lenTopic MSB
  mqttMessage[written++] = lenTopic;  // lenTopic LSB

  memcpy(&mqttMessage[written], topic, lenTopic);
  written += lenTopic;

  // Write QoS
  mqttMessage[written++] = 1;
  
  if (_gDebug) {
	refSer.println("Tx: MQTT_SUBSCRIBE");
  }
  
  String cmd = "AT+CIPSEND=0,";
  cmd += written;
  cmd += CR;

  // Send command
  ReadSim5320(_gDebug);              // clear rx buffer
  Sim5320.print(cmd);
  //uint8_t fie = ReadSim5320(true);
  delay(100);
  // Send message
  for (int i=0; i<written; i++) {
      Sim5320.write(mqttMessage[i]); // Message contents
     // refSer.print(byteToHexStr(mqttMessage[i]));
  }
 
  delay(100);

  retVal = verifyResponse(MQTT_SUBACK);

  return retVal;
}

///////////////////////////////////////////////////////////////////////////
bool SimMQTT::MqttPublish (const char* const topic, const char* const msg) {
    bool retVal = true;

    uint8_t lenTopic = strlen(topic);
    uint8_t lenMsg   = strlen(msg);

    uint8_t mqttMessage[128];
    uint8_t written  = 0;

    // Write fixed header
    mqttMessage[written++] = MQTT_MSG_PUBLISH;
    mqttMessage[written++] = 2 + lenTopic + lenMsg;

    // Write topic
    mqttMessage[written++] = 0;         // lenTopic MSB
    mqttMessage[written++] = lenTopic;  // lenTopic LSB
    memcpy(&mqttMessage[written], topic, lenTopic);
    written += lenTopic;

    // Write msg
    memcpy(&mqttMessage[written], msg, lenMsg);
    written += lenMsg;



    // Prepare command
	if (_gDebug) {
		refSer.println("Tx: MQTT_PUBLISH");
	}
    
    String cmd = "AT+CIPSEND=0,";
    cmd += written;
    cmd += CR;

    // Send command
    ReadSim5320(_gDebug);              // clear rx buffer
    Sim5320.print(cmd);
    delay(20);
    // Send message
    for (int i=0; i<written; i++) {
        Sim5320.write(mqttMessage[i]); // Message contents
    }

    // Check if send was successful
    delay(100);
    retVal = CheckOk();
    if ((retVal == false) && (_gDebug == true)) {
		refSer.println(String("[ERROR] Publish failed") + gRxMsg);
	}
		

    return retVal;
}

///////////////////////////////////////////////////////////////////////////
bool SimMQTT::MqttPingreq() {
  delay(1000);

  bool retVal = false;

  uint8_t mqttMessage[40];
  uint8_t written  = 0;

  // Write fixed header
  mqttMessage[written++] = MQTT_PINGREQ;
  mqttMessage[written++] = 0;

  if (_gDebug) {
		refSer.println("Tx: MQTT_PINGREQ");
  }
  
  String cmd = "AT+CIPSEND=0,";
  cmd += written;
  cmd += CR;

  // Send command
  ReadSim5320(_gDebug);              // clear rx buffer
  Sim5320.print(cmd);
  //uint8_t fie = ReadSim5320(true);
  delay(100);
  // Send message
  for (int i=0; i<written; i++) {
      Sim5320.write(mqttMessage[i]); // Message contents
  //    Serial.print(byteToHexStr(mqttMessage[i]));
  }
  delay(100);

  retVal = verifyResponse(MQTT_PINGRESP);
  return retVal;
  
}
