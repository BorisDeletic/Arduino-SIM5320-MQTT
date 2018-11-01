///////////////////////////////////////////////////////////////////////////
bool sendATcommand(String cmd, String expected, unsigned int timeout, bool openTCP) {
    if (gDebug == true) {
        Serial.print("Tx: ");
        Serial.println(cmd);
    }
    uint8_t x=0;
    bool answer=false;

    while (Sim5320.available() > 0) {
        char a = Sim5320.read();      // Clean the input buffer
    }

    if (cmd.length() > 0) {
        Sim5320.println(cmd);         // Send the AT command
    }
    memset(gRxMsg, '\0', sizeof(gRxMsg));    // Initialize input buffer

    unsigned long previous = millis();

    // this loop waits for the answer
    do {
        if (Sim5320.available() != 0) {
            gRxMsg[x++] = Sim5320.read();
            if ((x == 2) && (gRxMsg[0] == CR) && (gRxMsg[1] == LF)) {
                x = 0;    // erase blank lines
            }
            else if ((x > 2) && (gRxMsg[x-2] == CR) && (gRxMsg[x-1] == LF)) {
                //
                // <CR><LF> (aka '\n\r', aka end-of-line) received
                //

                if (gDebug == true) {
                    Serial.print("Rx: ");
                    Serial.print(gRxMsg);
                }

                if (openTCP) {
                    const char *compare = expected.c_str();
                  //  compare = new char[expected.length() + 1];
                  //  memcpy(compare, expected.c_str(), expected.length() + 1);
                 //   char compare = expected.c_str()
                    if (strstr(gRxMsg, compare) != nullptr) {
                     //   Serial.println("FOUND USING STRSTR");
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
    if (answer == false) {
        Serial.println(String("--- [ERROR] --- ") + cmd);
    }
    return answer;
}

///////////////////////////////////////////////////////////////////////////
void InitSim5320 (void) {
    Serial.println("InitSim5320");
    if (sendATcommand("AT") == true) {
        Serial.println("Turning off");
        digitalWrite(8,HIGH);
        delay(1200);
        digitalWrite(8,LOW);
    }
    gRxMsg[0] = '\0';   // Clear receive buffer
    while (strstr(gRxMsg, "PB DONE") == nullptr) {
        Serial.println("Turning on");
        delay(3000);
        digitalWrite(8,HIGH);
        delay(1200);
        digitalWrite(8,LOW);

        sendATcommand("", "PB DONE", 10000);
    }
    sendATcommand("AT+CGDCONT=1,\"IP\",\"yesinternet\",\"0.0.0.0\"");
    sendATcommand("AT+NETOPEN", "+NETOPEN", 10000, true);
    sendATcommand("AT+CTCPKA=1,5,5", "OK");
    
    Serial.println("InitSim5320 done");
    Serial.println("");
}

///////////////////////////////////////////////////////////////////////////
void InitWeb (void) {
  sendATcommand("AT+CGSOCKCONT=1,\"IP\",\"yesinternet\"");
  sendATcommand("AT+CSOCKSETPN=1");
  sendATcommand("AT+CIPMODE=1");

  StartMqtt();
}

//////////////////////////////////////////////////////////////////////
String byteToHexStr (const uint8_t value, const String prefix /*= "\\x"*/) {
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
void dumpHex(const uint8_t* const byteArray, const uint8_t len) {
    for (uint8_t i=0; i<len; ++i) {
        Serial.print(byteToHexStr(byteArray[i], "0x") + " ");
    }
    Serial.println();
}

///////////////////////////////////////////////////////////////////////////
void liveBufferRead() {
    static char buffer[256];
    static size_t pos;              // position of next write

    while (true) { //make this a timed affair so
      while (Sim5320.available() && pos < sizeof buffer - 1) {

        // Read incoming byte.
        char c = Sim5320.read();
        buffer[pos++] = c;

        // Echo received message.
        if (c == '!') {            // \n means "end of message"
            buffer[pos] = '\0';     // terminate the buffer
            for (int i = 0; i < pos; i++) {
                char ch = buffer[i];
                if ((ch>=32) && (ch<=127)) {
                    Serial.write(ch);
                }
                else if ((ch == CR) || (ch == LF)) {
                    Serial.write(ch);
                }
                else {
                    Serial.print(byteToHexStr(ch));
                }
            }
            pos = 0;                // reset to start of buffer
        }
      }
    }
}

///////////////////////////////////////////////////////////////////////////
uint8_t ReadSim5320 (bool print /*= false*/) {
    uint8_t len = 0;
    memset(gRxMsg, '\0', sizeof(gRxMsg));    // Initialize the string
    while (Sim5320.available() != 0) {
        gRxMsg[len++] = Sim5320.read();
        if (print == false) {
            delay(1);
        }
        else {
            char ch = gRxMsg[len - 1];
            if ((ch>=32) && (ch<=127)) {
                Serial.write(ch);
            }
            else if ((ch == CR) || (ch == LF)) {
                Serial.write(ch);
            }
            else {
                Serial.print(byteToHexStr(ch));
            }
        }
    }
    return len;
}

///////////////////////////////////////////////////////////////////////////
bool CheckOk (void) {
    ReadSim5320(gDebug);
    return strstr(gRxMsg, "OK\r\n");
}
