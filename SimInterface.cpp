/*
  SIM_Interface.cpp - Library for MQTT protocol on SIM5320.
  Created by Boris Deletic, October 30, 2018.
  Released into the public domain.
*/

#define CR                      '\r'        // 0x0D
#define LF                      '\n'        // 0x0A

#include <Sim5320MQTT.h>


SimInterface::SimInterface(SoftwareSerial* pntSim, const int simPowerPin) :
	_gDebug(false),
	logSer(nullptr),
	powerPin(simPowerPin),
	Sim5320(pntSim)
	{

}


void SimInterface::setLogging(Stream* pntSer, bool verbosity) {
	logSer = pntSer;
	if (verbosity) {
		logSer->println("Enabled logging");
		_gDebug = true;
	} else {
		_gDebug = false;
	}
}

///////////////////////////////////////////////////////////////////////////
bool SimInterface::sendATcommand(String cmd, String expected, unsigned int timeout, bool openTCP) {
    if (_gDebug == true) {
        logSer->print("Tx: ");
        logSer->println(cmd);
    }
    uint8_t x=0;
    bool answer=false;

    while (Sim5320->available() > 0) {
        char a = Sim5320->read();      // Clean the input buffer
    } 

    if (cmd.length() > 0) {
		logSer->println(cmd); /// testing m8
        Sim5320->println(cmd);         // Send the AT command
    }
    memset(gRxMsg, '\0', sizeof(gRxMsg));    // Initialize input buffer

    unsigned long previous = millis();

    // this loop waits for the answer
    do {
        if (Sim5320->available() != 0) {
            gRxMsg[x++] = Sim5320->read();
            if ((x == 2) && (gRxMsg[0] == CR) && (gRxMsg[1] == LF)) {
                x = 0;    // erase blank lines 
            }
            else if ((x > 2) && (gRxMsg[x-2] == CR) && (gRxMsg[x-1] == LF)) {
                //
                // <CR><LF> (aka '\n\r', aka end-of-line) received
                //

                if (_gDebug == true) {
                    logSer->print("Rx: ");
                    logSer->print(gRxMsg);
                }

                if (openTCP) {
                    const char *compare = expected.c_str();
                  //  compare = new char[expected.length() + 1];
                  //  memcpy(compare, expected.c_str(), expected.length() + 1);
                 //   char compare = expected.c_str()
                    if (strstr(gRxMsg, compare) != nullptr) {
                     //   Serial.println("FOUND USING STRSTR stevedog");
                   //     Serial.println(gRxMsg);
                        answer = true;
                        break;
                    }                  
                } else {
                    if ((x >= 4) && (strcmp(gRxMsg, "OK\r\n") == 0)) {
                        answer = true;
                        break;
                    }
                    else if ((x >= 7) && (strcmp(gRxMsg, "ERROR\r\n") == 0)) {
                        answer = false;
                        break;
                    }
                    else if ((x >= expected.length()) && (String(gRxMsg) == expected + "\r\n")) {
                        answer = true;
                        break;
                    }
                }
                x = 0;
                memset(gRxMsg, '\0', sizeof(gRxMsg));    // Clear input buffer
                
            }
        }
        // Waits for the answer with time out
    } while ((millis() - previous) < timeout);
    if ((answer == false) && (_gDebug)) {
        logSer->println(String("--- [ERROR] --- ") + cmd);
    }
    return answer;
}

///////////////////////////////////////////////////////////////////////////
void SimInterface::InitSim5320 (void) {
	if (_gDebug) {
		logSer->println("InitSim5320");
	}
    if (sendATcommand("AT") == true) {
		if (_gDebug) {
			logSer->println("Turning off");
		}
        digitalWrite(powerPin, HIGH);
        delay(1200);
        digitalWrite(powerPin, LOW);
    }
    gRxMsg[0] = '\0';   // Clear receive buffer
    while (strstr(gRxMsg, "PB DONE") == nullptr) {
		
		if (_gDebug) {
			logSer->println("Turning on");
		}
        delay(3000);
        digitalWrite(powerPin, HIGH);
        delay(1200);
        digitalWrite(powerPin, LOW);

        sendATcommand("", "PB DONE", 10000);
    }
    sendATcommand("AT+CGDCONT=1,\"IP\",\"yesinternet\",\"0.0.0.0\"");
    sendATcommand("AT+NETOPEN", "+NETOPEN", 10000, true);
    sendATcommand("AT+CTCPKA=1,5,5", "OK");
    
	if (_gDebug) {
	    logSer->println("InitSim5320 done");
		logSer->println("");	
	}

}

///////////////////////////////////////////////////////////////////////////
void SimInterface::InitWeb (void) {
  sendATcommand("AT+CGSOCKCONT=1,\"IP\",\"yesinternet\"");
  sendATcommand("AT+CSOCKSETPN=1");
  sendATcommand("AT+CIPMODE=1");

}

//////////////////////////////////////////////////////////////////////
String SimInterface::byteToHexStr (const uint8_t value, const String prefix /*= "\\x"*/) {
    String retVal = prefix;
    uint8_t digit = value / 16;
    for (uint8_t i=0; i<2; ++i) {
        if (digit > 9) {
            digit -= 10;
            retVal += char('a' + digit);
        }
        else {
            retVal += char('0' + digit);
        }
        digit = value % 16;
    }
    return retVal;
}


///////////////////////////////////////////////////////////////////////////
uint8_t SimInterface::ReadSim5320 (bool print /*= false*/) {
    uint8_t len = 0;
    memset(gRxMsg, '\0', sizeof(gRxMsg));    // Initialize the string
    while (Sim5320->available() != 0) {
        gRxMsg[len++] = Sim5320->read();
        if (print == false) {
            delay(1);
        }
        else {
            char ch = gRxMsg[len - 1];
			if (_gDebug) {
				if ((ch>=32) && (ch<=127)) {
					logSer->write(ch);
				}
				else if ((ch == CR) || (ch == LF)) {
					logSer->write(ch);
				}
				else {
					logSer->print(byteToHexStr(ch));
				}
            }
        }
    }
    return len;
}

///////////////////////////////////////////////////////////////////////////
bool SimInterface::CheckOk (void) {
    ReadSim5320(_gDebug);
    return strstr(gRxMsg, "OK\r\n");
}

///////////////////////////////////////////////////////////////////////////
bool SimInterface::verifyResponse(const char MQTT_ACK) {
  bool retVal = false;
  CheckOk(); // prime buffer
  
  delay(10000); // wait for server to respond to sim golden boy
  uint8_t len = ReadSim5320(true);
 // Serial.print(String("len-2: ") + byteToHexStr(gRxMsg[len-2]) + String("  len-1") + byteToHexStr(gRxMsg[len-1]));
  if ((gRxMsg[len-2] == MQTT_ACK || gRxMsg[len-4] == MQTT_ACK || gRxMsg[len-5] == MQTT_ACK ) && (gRxMsg[len-1] == 0x00 || gRxMsg[len-1] == 0x01)) {
    if (_gDebug) {
		logSer->println("Rx: ACKNOWLEDGED");
	}
    retVal = true;
  }
  return retVal;
}