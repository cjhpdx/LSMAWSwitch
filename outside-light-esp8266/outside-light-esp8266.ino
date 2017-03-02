const int feather = 115200;
const int uno = 9600;
const int baudRate = feather;
const int piMotionInputPin = 13;
const int piMotionOutputPin = 16;
const int photocellInputPin = 0;

// Light Sensitive Motion Activated Wifi Switch

class LsMawSwitch{

  public: //public otherwise error: test::test private

  String todStatus;
  String lightStatus;

  bool lightOn;
  bool daytime;
  bool motionDetected;
  String motionDetectedStr;
  bool timerActive;  
  bool remoteOn;
  String remoteOnStr;
    
  int lightDetected;
  int PIRSensor;
  int timer;
  int switchOffDelay; 
   
  unsigned long lastMotionTime;

  LsMawSwitch(){ 
    daytime = false;
    todStatus = "Nighttime";
    lightDetected = 0;
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
  
  void turnOnTheLight(){
    lightOn = true;
    lightStatus = "On";
    digitalWrite(piMotionOutputPin, HIGH);
  }
  
  void turnOffTheLight(){
    lightOn = false;
    lightStatus = "Off";
    digitalWrite(piMotionOutputPin, LOW);
  }
  
  void updateDayNightTransition() {
    readLedPhotocell();
    if ( lightDetected < 35 ) {
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
    Serial.println(message);
  }

  void updateSwitch() {
    // Check if security light is on or off
    
    if ( ! lightOn ) {
      
      // security light is off 
  
      // try to detect and update time of day transition
      // by reading photo led sensor and set value of
      // daytime(true or false)
      updateDayNightTransition();
      
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

void setup() {
  Serial.begin(baudRate);
  pinMode(piMotionInputPin, INPUT);
  pinMode(piMotionOutputPin, OUTPUT);
  pinMode(A0, INPUT);
}

void loop() {

  // Program loop starts here
  sw.checkForMotion();

  if ( sw.remoteOn ) {
    sw.turnOnTheLight();
  } else {
    sw.updateSwitch();
  }
  sw.statusReport();

}



// http://arduino.sundh.com/2013/02/photoresistor-controlling-led/
// https://community.thinger.io/t/esp8266-analog-read/37
// https://learn.adafruit.com/adafruit-feather-huzzah-esp8266/using-arduino-ide
// https://www.youtube.com/watch?v=PgsH43Tpqjc
// https://raw.githubusercontent.com/ItKindaWorks/ESP8266/master/Home%20Automation/Part%201/ESP8266_SimpleMQTT/ESP8266_SimpleMQTT.ino
