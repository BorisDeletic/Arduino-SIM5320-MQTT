#include <Sim5320MQTT.h>
#include <SimInterface.h>
#include <SoftwareSerial.h>



void setup() {
  // put your setup code here, to run once:
  int RX = 2;
  int TX = 3;
  int power = 8;
  Serial.begin(9600);
  SimMQTT Sim5320(RX, TX, power, "yesinternet", "0.0.0.0"); // service provider and IP to be passed to AT+CGDCONT command and AT+CGSOCKCONT
  
  Sim5320.setLogging(&Serial, true);
  delay(1000);
  Sim5320.MqttOpen("test.moquitto.org", "1883"); //open TCP port on host address and on given port. use IP address if domain name doesn't work
  delay(1000);
  Sim5320.MqttConnect("ClientID", "username", "password"); //Connect with given client ID and username/password
  delay(1000);
  Sim5320.MqttSubscribe("subscribe/topic"); //subscribe to given topic
  delay(1000);
  Sim5320.MqttPublish("publish/topic", "PublishMessage: 1337");
  delay(1000);
  Sim5320.MqttPingreq(); //
  delay(10000);
  
}

void loop() {

  // put your main code here, to run repeatedly:

}
