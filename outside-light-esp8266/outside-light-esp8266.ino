const int feather = 115200;
const int uno = 9600;
const int baudRate = feather;
const int piMotionInputPin = 13;
const int piMotionOutputPin = 16;
const int photocellInputPin = 0;
//the time we give the sensor to calibrate (10-60 secs according to the datasheet)
int calibrationTime = 30;
int reportFrequency = 1000;
unsigned long t_time;
unsigned long DayNightTransition_time;
char message_buff[100];
char DayNightStatus_buff[100];
char LightDetected_buff[100];
char MovementDetected_buff[100];

// Light Sensitive Motion Activated Wifi Switch

#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>


//Wifi network config
#define MQTT_SERVER "---"
const char* ssid = "---";
const char* password = "---";

char* garageDoorOutsideLight1 = "/garage/outside/light1";
char* garageLight1Threashold = "/garage/outside/light1/threashold";
char* garageLight1DayNightTransitionDelay = "/garage/outside/light1/dayNightTransitionDelay";
char* reportFrequencyTopic = "/garage/outside/light1/frequency";
void callback(char* topic, byte* payload, unsigned int length);

WiFiClient wifiClient;
PubSubClient client(MQTT_SERVER, 1883, callback, wifiClient);

class LsMawSwitch{

  public: 

  String todStatus;
  const char* lightStatus;
  const char* lightTopic;
  const char* lightMessage;
  const char* motionTopic;
  const char* motionMessage;


  bool lightOn;
  bool daytime;
  bool motionDetected;
  String motionDetectedStr;
  bool timerActive;  
  bool remoteOn;
  String remoteOnStr;
    
  int lightDetected;
  int lightThreashold_int;
  int dayNightTransition_delay;
  int PIRSensor;
  int timer;
  int switchOffDelay; 
   
  unsigned long lastMotionTime;

  LsMawSwitch(){ 
    daytime = false;
    todStatus = "Nighttime";
    lightDetected = 0;
    lightThreashold_int = 90;
    dayNightTransition_delay = 10000;
    lightOn = false;
    lightStatus = "Off";
    PIRSensor = 0;
    motionDetected = false;
    motionDetectedStr = "false";
    lastMotionTime = 0;
    timer = 0;
    timerActive = false;
    switchOffDelay = 10000;
    remoteOn = false;
    remoteOnStr = "false";
    lightTopic = "/garage/outside/light1/confirm";
    lightMessage = "0";
  }
  
  void setNighttime(){ 
    daytime = false;
    todStatus = "Nighttime";
  }
  
  void setDaytime(){ 
    daytime = true;
    todStatus = "Daytime";
  }
  
  void readLedPhotocell(){
    lightDetected = (analogRead(A0));
  }

  void manualOn(){
    remoteOn = true;
    remoteOnStr = "true";
    lightMessage = "1";
  }

  void manualOff(){
    remoteOn = false;
    remoteOnStr = "false";
    lightMessage = "0";
  }

  void turnOnTheLight(){
    lightOn = true;
    lightStatus = "On";
    lightMessage = "1";
    digitalWrite(piMotionOutputPin, HIGH);
  }
  
  void turnOffTheLight(){
    lightOn = false;
    lightStatus = "Off";
    lightMessage = "0";
    digitalWrite(piMotionOutputPin, LOW);
  }
  
  void updateDayNightTransition() {
    readLedPhotocell();
    if ( lightDetected < lightThreashold_int ) {
      setNighttime();
    } else {
      setDaytime();
    }
  }
 
  void checkForMotion() {
    PIRSensor = digitalRead(piMotionInputPin);
    if ( PIRSensor == 1 ) {
      motionDetected = true;
      motionDetectedStr = "true";
    } else {
      motionDetected = false;
      motionDetectedStr = "false";
    }
  }

  void startTheTimer() {
    timer = 0;
    lastMotionTime = millis();
  }

  void resetTheTimer() {
    timer = 0;
  }

  void updateTheTimer() {
    if ( lightOn ) {
      timer = (millis() - lastMotionTime);
    } else {
      timer = 0;
    }
  }

  void delayShutOff() {
    updateTheTimer();
    if( timer >= switchOffDelay) {
      turnOffTheLight();
      resetTheTimer();
    } 
  }

  void statusReport() {
    String message =  String( "[" + todStatus + "] " + "Light:" + lightStatus );
    message +=  String( " Timer:" + String(timer, DEC) + " MovementDetected:" + motionDetectedStr );
    message +=  String( " LightDetected:" + String(lightDetected, DEC) + " RemoteOn:" + remoteOnStr );
    message +=  String( " x " + String(lightThreashold_int, DEC) + " x " + String(dayNightTransition_delay, DEC) + " x " + String(reportFrequency, DEC));

    Serial.println(message);
  }

  void updateSwitch() {
    // Check if security light is on or off
    
    if ( ! lightOn ) {
      
      // security light is off 
  
      // try to detect and update time of day transition
      // by reading photo led sensor and set value of
      // daytime(true or false)
      // and DayNightTransition_time seconds have passed
      
      if (millis() > (DayNightTransition_time + dayNightTransition_delay)) {
        updateDayNightTransition();
        DayNightTransition_time = millis();
      }
      
    }
  
    // Check if it's Daytime or Nighttime
    
    if ( ! daytime ) {
      
      // Nighttime
      
      if ( motionDetected ) {
        
        // Movement detected
  
        turnOnTheLight();
  
        startTheTimer();
  
      } else {
        
        // wait before turning off the light
        delayShutOff();
        
      }
      
    } else {
      
      // Daytime 
      
      turnOffTheLight();
  
    }
  }
};

LsMawSwitch sw = LsMawSwitch();

void callback(char* topic, byte* payload, unsigned int length) {

  //convert topic to string to make it easier to work with
  String topicStr = topic; 

  //Print out some debugging info
  Serial.println("Callback update.");
  Serial.print("Topic: ");
  Serial.println(topicStr);

//char* garageDoorOutsideLight1 = "/garage/outside/light1";
//char* garageLight1Threashold = "/garage/outside/light1/threashold";
//char* garageLight1DayNightTransitionDelay = "/garage/outside/light1/dayNightTransitionDelay";

  if(topicStr == garageDoorOutsideLight1){
    if(payload[0] == '1'){
      sw.manualOn();
      client.publish(sw.lightTopic, sw.lightStatus);
    }
  
    else if (payload[0] == '0'){
      sw.manualOff();
      client.publish(sw.lightTopic, sw.lightStatus);
    }
  }

//  // Dynamicly change light threashold
//  if(topicStr == garageLight1Threashold){
//    int multiplyer = int(payload[0]);
//    sw.lightThreashold_int = multiplyer * 1000;
//  }
//
//  // Dynamicly change light threashold timer
//  if(topicStr == garageLight1DayNightTransitionDelay){
//    int multiplyer = int(payload[0]);
//    sw.dayNightTransition_delay = multiplyer * 1000;
//    Serial.println(payload[0]);
//    Serial.println(int(payload[0]));
//    Serial.println(String(payload[0], DEC));
//    Serial.println(multiplyer);
//    Serial.println(sw.dayNightTransition_delay); 
//  }
//
//  // Dynamicly change reporting frequency
//  if(topicStr == reportFrequencyTopic){
//    int multiplyer = int(payload[0]);
//    reportFrequency = multiplyer * 1000;
//  }

}

void reconnect() {

  //attempt to connect to the wifi if connection is lost
  if(WiFi.status() != WL_CONNECTED){
    //debug printing
    Serial.print("Connecting to ");
    Serial.println(ssid);

    //loop while we wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    //print out some more debug once connected
    Serial.println("");
    Serial.println("WiFi connected");  
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  //make sure we are connected to WIFI before attemping to reconnect to MQTT
  if(WiFi.status() == WL_CONNECTED){
  // Loop until we're reconnected to the MQTT server
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");

      // Generate client name based on MAC address and last 8 bits of microsecond counter
      String clientName;
      clientName += "esp8266-";
      uint8_t mac[6];
      WiFi.macAddress(mac);
      clientName += macToStr(mac);

      //if connected, subscribe to the topic(s) we want to be notified about
      if (client.connect((char*) clientName.c_str())) {
        Serial.print("\tMTQQ Connected\n");
        client.subscribe(garageDoorOutsideLight1);
        client.subscribe(garageLight1Threashold);
        client.subscribe(garageLight1DayNightTransitionDelay);
        client.subscribe(reportFrequencyTopic);
        overTheAir();
      }

      //otherwise print failed for debugging
      else{Serial.println("\tFailed."); abort();}
    }
  }
}

void overTheAir(){
  // Port defaults to 8266
  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("sendit69");

  // No authentication by default
  //ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA ----> > > Ready");
  Serial.print("     IP address: ");
  Serial.println(WiFi.localIP());
}

//generate unique name from MAC addr
String macToStr(const uint8_t* mac){

  String result;

  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);

    if (i < 5){
      result += ':';
    }
  }

  return result;
}

void setup() {
  Serial.begin(baudRate);
  delay(100);
  pinMode(piMotionInputPin, INPUT);
  pinMode(piMotionOutputPin, OUTPUT);
  pinMode(A0, INPUT);

//  ESP.eraseConfig();
  //start wifi subsystem
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  //attempt to connect to the WIFI network and then connect to the MQTT server
  reconnect();

  //wait a bit before starting the main loop
  delay(2000);
}

void loop() {
  ArduinoOTA.handle();
  
  // reconnect if connection is lost
  if (!client.connected() && WiFi.status() == 3) {reconnect();}

  // maintain MQTT connection
  client.loop();

  // MUST delay to allow ESP8266 WIFI functions to run
  delay(100);
  
  // main program loop starts here
  sw.checkForMotion();

  if ( sw.remoteOn ) {
    sw.turnOnTheLight();
  } else {
    sw.updateSwitch();
  }
  sw.statusReport();
  
  // publish every 5 seconds
  if (millis() > (t_time + 1000)) {
    String message =  String( "[" + sw.todStatus + "] " + "Light:" + sw.lightStatus );
    message +=  String( " Timer:" + String(sw.timer, DEC) + " MovementDetected:" + sw.motionDetectedStr );
    message +=  String( " LightDetected:" + String(sw.lightDetected, DEC) + " RemoteOn:" + sw.remoteOnStr );
    message.toCharArray(message_buff, message.length()+1);
    client.publish("/garage/LsMawSwitch/output", message_buff);
    
    String DayNightStatus = sw.todStatus;
    DayNightStatus.toCharArray(DayNightStatus_buff, DayNightStatus.length()+1);
    client.publish("/garage/LsMawSwitch/DayNightStatus", DayNightStatus_buff);

    String LightDetected = String(sw.lightDetected, DEC);
    LightDetected.toCharArray(LightDetected_buff, DayNightStatus.length()+1);
    client.publish("/garage/LsMawSwitch/LightDetected", LightDetected_buff);

    String MovementDetected = sw.motionDetectedStr;
    MovementDetected.toCharArray(MovementDetected_buff, DayNightStatus.length()+1);
    client.publish("/garage/LsMawSwitch/MovementDetected", MovementDetected_buff);

    client.publish(sw.lightTopic, sw.lightStatus);

    
    t_time = millis();

  }


}



// http://arduino.sundh.com/2013/02/photoresistor-controlling-led/
// https://community.thinger.io/t/esp8266-analog-read/37
// https://learn.adafruit.com/adafruit-feather-huzzah-esp8266/using-arduino-ide
// https://www.youtube.com/watch?v=PgsH43Tpqjc
// https://raw.githubusercontent.com/ItKindaWorks/ESP8266/master/Home%20Automation/Part%201/ESP8266_SimpleMQTT/ESP8266_SimpleMQTT.ino
