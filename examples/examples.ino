#include <SoftwareSerial.h>
#include <avr/wdt.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "LowPower.h"

#define Reset_AVR() wdt_enable(WDTO_1S); while(1) {}
#define ONE_WIRE_BUS 10         // Data wire For Temp Probe is plugged into pin 10 on the Arduino
#define SIM5320_RX 2
#define SIM5320_TX 3

SoftwareSerial Sim5320(SIM5320_RX,SIM5320_TX);
OneWire oneWire(ONE_WIRE_BUS);// Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature sensors(&oneWire);// Pass our oneWire reference to Dallas Temperature.

float temp, EC, lastSentEC, lastSentTemp;
const int reps = 8; // times to read sensors before transmitting
int count ;

///////////////////////////////////
//#define MQTT_BROKER       "37.187.106.16"           // test.mosquitto.org
/*
#define MQTT_BROKER       "test.mosquitto.org"
#define MQTT_PORT         "1883"
#define MQTT_CLIENT_ID    "BorisSimClient" 
*/

//#define MQTT_BROKER       "m11.cloudmqtt.com"
#define MQTT_BROKER       "54.147.156.31"
#define MQTT_PORT         "13800"
#define MQTT_CLIENT_ID    "BorisSimClient"
#define MQTT_USERNAME     "akgvwkte"
#define MQTT_PASSWORD     "vZjfDzMuHAY2"

#define MQTT_MSG_CONNECT        0x10
#define MQTT_MSG_CONNACK        0x20
#define MQTT_MSG_PUBLISH        0x30
#define MQTT_SUBSCRIBE          0x82
#define MQTT_SUBACK             0x90
#define MQTT_PINGREQ            0xc0
#define MQTT_PINGRESP           0xd0

// ASCII Codes
#define CR                      '\r'        // 0x0D
#define LF                      '\n'        // 0x0A
//#define CTRL_Z                  ((uint8_t)0x1A)

#define SENSOR_ID         "SensorA" //just change the letter
#define SENSOR_LISTEN_CH  "monash/senA"  // keep capitalised
// this must match with server transmitter topic

///////////////////////////////////
//char MQTT_CLIENT_ID[12];
char gRxMsg[150];
bool gDebug = true;
const int listenPeriod = 5;     // times in seconds between checking for received data
// bool samplerOn; 
bool connState = false;         // outcome of ping req

///////////////////////////////////
// Function prototypes
bool    sendATcommand(String ATcommand, String expected = "OK", unsigned int timeout = 1000, bool openTCP = false);
//String  byteToHexStr (const uint8_t value, const String prefix = "\\x");
String  byteToHexStr (const uint8_t value, const String prefix = " 0x");
uint8_t ReadSim5320  (bool print = false);
void genRandomID(char* s, const int len);
bool MqttPingreq();
bool receiveCommand();
bool verifyResponse(const char MQTT_ACK);
void liveBufferRead();


//////////////////////////////////////////////////////////////////////////
void setup (void) {
    delay(5000);    // This stops setup() from being called twice
    //clear all flags
     MCUSR = 0;

    // Write logical one to WDCE and WDE
    // Keep old prescaler setting to prevent unintentional time-out
     WDTCSR |= _BV(WDCE) | _BV(WDE);
     WDTCSR = 0;

    //Probe stuff
    pinMode(5, OUTPUT); //Temp probe negative is 8 => 5
    digitalWrite(5, LOW); //temp probe negative is set to low
    pinMode(6, OUTPUT); //ditto but for positive 9 =>6
    digitalWrite(6,HIGH); //temp probe positive is set to high
    pinMode(A0,INPUT); //EC PIN is A0
    pinMode(A3,OUTPUT);//EC Power is A3
    pinMode(A1,OUTPUT);//EC Ground is A1
    digitalWrite(A1,LOW);//EC Ground is set to low
    sensors.begin();

    temp = 0.0;
    EC = 0.0;

   // genRandomID(MQTT_CLIENT_ID, 15);
    ////

    // Note on Sim900 baud rates. The unit defaults to 115200 but I have
    // found it to be unreliable. To change baud rate, issue 'AT+IPREX=115200'
    // or similar (change the baud rate number). After this command the sketch
    // cannot communicate with the Sim5320 any more. Change the next line to
    // match the new baud rate and restart.
    //
    // If echoing commands from the Serial to Sim5320 and vv (see looop()), it
    // is best if the two baud rates are identical.
    const uint32_t cBaudRate = 19200;
    Sim5320.begin(cBaudRate);
    Serial.begin (cBaudRate);

    // Init Sim5320 power control pin
    digitalWrite(8, LOW);
    pinMode(8, OUTPUT);

    InitSim5320();
    InitWeb();

    digitalWrite(3, LOW);
    
    Serial.println("Waiting for commands from Serial monitor");
}

//////////////////////////////////////////////////////////
void readSensors() { //ripped from David's code
   //Temperature
  delay(50);
 // Serial.println(F("read sen"));
//  delay(50);
  float readTemp, readEC;

  sensors.requestTemperatures();// Send the command to get temperatures
  readTemp = sensors.getTempCByIndex(0); //Stores Value in Variable

  //EC
  digitalWrite(A3,HIGH);
  readEC= analogRead(A0);
  readEC= analogRead(A0);// This is not a mistake, First reading will be low beause if charged a capacitor
  digitalWrite(A3,LOW);
  readEC = (5*readEC)/1024.0;
  readEC = (readEC*1025)/(5-readEC);
  readEC = readEC-25;
  readEC = (1000/(readEC*2.88));
  readEC = readEC / (1+ 0.019 *(readTemp-25.0));

//  EC += readEC/reps;
// VERY IMPORTANT SET EC TO 0 FOR BEDROOM TESTING
// TODO: CHANGE THIS BACK FOR FINAL SUBMISSION.
  EC = 0.0;
  temp += readTemp/reps; //add the weighted sensor reading
}


void transmitCheck() {

  float changeEC = sqrt(sq((EC - lastSentEC)/EC));  //relative change as decimal
  float changeTemp = sqrt(sq((temp - lastSentTemp)/temp));

/*  Serial.print("Temperature ");
  Serial.println(temp);
  Serial.print("EC ");
  Serial.println(EC);

  Serial.print("Count ");
  Serial.println(count); 
  
  Serial.print("Change Temp ");
  Serial.println(changeTemp);
  Serial.print("lastSentTemp ");
  Serial.println(lastSentTemp); */
  
  if (changeEC > 0.1 || changeTemp > 0.1 || count % 10 == 0){ // count is measure of how often sensor readings are averaged
    Serial.print("Change EC, Change Temp: ");
    Serial.print(changeEC);
    Serial.print(", ");
    Serial.println(changeTemp);

		Serial.println("transmitting data due to change or an hour having passed");
    lastSentTemp = temp;
    lastSentEC = EC;

 //   Serial.println(String("lastSentTemp ")+lastSentTemp);

    String msg = "";
    msg += SENSOR_ID;
    msg += ',';
    msg += temp; //first index is temperature
    msg += ","; // comma to seperate values
    msg += EC; //EC is second value
    char charMsg[msg.length()+1];
    msg.toCharArray(charMsg, msg.length()+1);

    sendATcommand("AT", "OK", 1000);
    delay(1000);
    sendATcommand("AT", "OK", 1000);

    delay(100);
// remove start mqtt as receiving mode enables a constant connection
  //  StartMqtt(); // start mqtt connection
    
     connState = MqttPingreq();
     if (connState == false) {
        Serial.println("NOT CONNECTED TO MQTT SERVER");
        delay(3000);
        StartMqtt();

     } else if (connState == true) {
        Serial.println("RECEIVED RESPONSE, CONNECTED");
     }

     delay(1000); //channel for data transmission

     MqttPublish("monash/server/data", charMsg);    
      //publish data

    digitalWrite(3,LOW); //set pins to low to save power on SIM5320 chip
  }
}


//////////////////////////////////////////////////////////
bool receiveCommand() {
    Serial.println("listening...");
     //   CheckOk();
  
 //   delay(listenPeriod * 1000);
    uint8_t len = ReadSim5320(gDebug);

/*live reading fixes 64 bit buffer issue, and collects message instantly.
however arduino constantly reads sim, which is excessive and means it cannot sleep.
can fix by expanding buffer size (hack and not propper and not working)*/
        //uint8_t len = 0;
   //     liveBufferRead();
    char command[5];
   // char onMsg[5] = "ON!";
        
    if (len != 0x00) {
        command[0] = gRxMsg[len-3];
        command[1] = gRxMsg[len-2];
        command[2] = gRxMsg[len-1];   
        Serial.println(String("Command: "));
        Serial.println(command);      
        if (String(command) == "ON!") {
            Serial.println("RECEIVED SAMPLER ON COMMAND");
            sendATcommand("AT", "OK", 1000);
            delay(100);
            sendATcommand("AT", "OK", 1000);    
            delay(100);
                    
            // channel for sampler confirmation
            char msg[20] = SENSOR_ID;
            strcat(msg, ",ON!");
            MqttPublish("monash/server/status", msg); // reply confirmation
            
            // turn on sampler
        } else if (String(command) == "OFF") {
            Serial.println("SAMPLER OFF");
            sendATcommand("AT", "OK", 1000);
            delay(100);
            sendATcommand("AT", "OK", 1000);  
            delay(100);  
                    
            char msg[20] = SENSOR_ID;
            strcat(msg, ",OFF");
            MqttPublish("monash/server/status", msg); // reply confirmation
            // turn off sampler
        }
        
        digitalWrite(3, LOW);
    }
 //   Serial.println(String("reclen: ") + byteToHexStr(len));
}


//////////////////////////////////////////////////////////
void loop() {
  count++;
  Serial.println(count);
  for (int i=0; i<reps; i++){
    readSensors();
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
  receiveCommand(); //check for commands after exiting sleep cycle (~1min)
  
  transmitCheck(); // check parameters, and transmit msgs if required 
  temp = 0.0;
  EC = 0.0;

  if (count == 1441) {
    Reset_AVR();
    count = 0;  //reset at a day
  }
}
