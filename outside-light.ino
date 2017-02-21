const int feather = 115200;
const int uno = 9600;
const int baudRate = uno;
const int piMotionInputPin = 8;
const int piMotionOutputPin = 7;
const int photocellInputPin = 0;

// Light Sensitive Motion Activated Wifi Switch

class LsMawSwitch{

  public: //public otherwise error: test::test private

  String todStatus;
  String lightStatus;

  bool lightOn;
  bool daytime;
  bool motionDetected;
  bool timerActive;  
  bool remoteOn;
    
  int lightDetected;
  int PIRSensor;
  int timer;
  int switchOffDelay; 
   
  unsigned long lastMotionTime;

  LsMawSwitch(){ //the constructor
    daytime = false;
    todStatus = "Nighttime";
    lightDetected = 0;
    lightOn = false;
    lightStatus = "Off";
    PIRSensor = 0;
    motionDetected = false;
    lastMotionTime = 0;
    timer = 0;
    timerActive = false;
    switchOffDelay = 10000;
    remoteOn = false;
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
    lightDetected = (analogRead(0));
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
    if ( lightDetected < 10 ) {
      setNighttime();
    } else {
      setDaytime();
    }
  }
 
  void checkForMotion() {
    PIRSensor = digitalRead(piMotionInputPin);
    if ( PIRSensor == 1 ) {
      motionDetected = true;
    } else {
      motionDetected = false;
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
  
};

LsMawSwitch sw = LsMawSwitch();

void setup() {
  Serial.begin(baudRate);
  pinMode(piMotionInputPin, INPUT);
  pinMode(piMotionOutputPin, OUTPUT);
  pinMode(A0, INPUT);
}

char *boolstring( _Bool b ) { return b ? "true" : "false"; }

void loop() {
  
  // Program loop starts here

  // Check for PIRSensor for motion
  
  sw.checkForMotion();

  // Check for remote override
  if ( sw.remoteOn ) {
    
    sw.turnOnTheLight();
    
  } else {
    
    // Check if security light is on or off
    
    if ( ! sw.lightOn ) {
      
      // security light is off 
  
      // try to detect and update time of day transition
      // by reading photo led sensor and set value of
      // daytime(true or false)
      sw.updateDayNightTransition();
      
    }
  
    // Check if it's Daytime or Nighttime
    
    if ( ! sw.daytime ) {
      
      // It's Nighttime
      
      if ( sw.motionDetected ) {
        
        // Movement detected
  
        sw.turnOnTheLight();
  
        sw.startTheTimer();
  
      } else {
        
        // wait before turning off the light
        sw.delayShutOff();
        
      }
      
    } else {
      
      // It's Daytime!! 
      
      sw.turnOffTheLight();
  
    }
    
  }
  
  String message =  String( "[" + sw.todStatus + "] " + "Light:" + sw.lightStatus );
  message +=  String( " Timer:" + String(sw.timer, DEC) + " MovementDetected:" + String(boolstring(sw.motionDetected)));
  message +=  String( " LightDetected:" + String(sw.lightDetected, DEC) + " RemoteOn:" + String(boolstring(sw.remoteOn)) );
  Serial.println(message);
}



// http://arduino.sundh.com/2013/02/photoresistor-controlling-led/
// https://community.thinger.io/t/esp8266-analog-read/37
// https://learn.adafruit.com/adafruit-feather-huzzah-esp8266/using-arduino-ide

