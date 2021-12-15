//WIFI MODULE CODE FOR MODULE B
#include <math.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

//VARIABLES FOR REGULATORS: (Patterns)
const int toRegulatorA = 14; //D5
const int toRegulatorB = 12; //D6
const int toRegulatorC = 13; //D7
const int toRegulatorD = 15; //D8
  int state_PatternA = 0;
  int statePrevious_PatternA = 0;
  int state_PatternB = 0;
  int statePrevious_PatternB = 0;
  int state_PatternC = 0;
  int statePrevious_PatternC = 0;
  int state_PatternD = 0;
  int statePrevious_PatternD = 0;
  unsigned long timer_pattern = 0;
  unsigned long timerStart_pattern = 0;

//CONTROL VARIABLES:
const bool DEBUG = true;
  unsigned long inputDelayTime;
  int inputPattern;
  bool inputIsActive;
  bool inputIsSync;
  int count = 0;
const int loopPatternA = 3;
const int loopPatternB = 2;
const int loopPatternC[2] = {2,3};
const int numberPatterns = 4; //number of patterns that currently exist

//STRUCTURE TO BE SENT OUT:
typedef struct moduleAData { //module outside
  float temperature;
  float humidity;
} moduleAData;

//STRUCTURE TO BE RECEIVED:
typedef struct moduleBData { //module within home
  unsigned long delayTime;
  int pattern;
  bool isActive;
  bool isSync;
} moduleBData;

//VARIABLES DEALING WITH WIFI:
uint8_t broadcastAddress[] = {0x40,0xF5,0x20,0x3E,0x28,0x35}; //MAC of wifi module B
  moduleAData outputData; //package to be shipped to module B
  moduleBData inputData;

void setup() {
  pinMode(toRegulatorA, OUTPUT);
  pinMode(toRegulatorB, OUTPUT);
  pinMode(toRegulatorC, OUTPUT);
  pinMode(toRegulatorD, OUTPUT);
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (DEBUG == true) {
    Serial.begin(115200);
    if (esp_now_init() != 0) {
      Serial.println("Error initializing ESP-NOW");
      return;
    }
  }
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
}

void loop() {
  if (inputIsActive == true) {
    if (inputIsSync == true) {
      if (inputPattern == 1){
        if (state_PatternA == 1){ //is in wait state, run it
          state_PatternA == 2;
        }
        FSM_patternA();        
        if(DEBUG == true){
          if(state_PatternA != statePrevious_PatternA){
            Serial.print("State of A: ");
            Serial.println(state_PatternA);
          }
        }
      }
      else if (inputPattern == 2){
        if (state_PatternB == 1){ //is in wait state, run it
          state_PatternB == 2;
        }
        FSM_patternB();        
        if(DEBUG == true){
          if(state_PatternB != statePrevious_PatternB){
            Serial.print("State of B: ");
            Serial.println(state_PatternB);
          }
        }        
      }
      else if (inputPattern == 3){
        if (state_PatternC == 1){ //is in wait state, run it
          state_PatternC == 2;
        }
        FSM_patternC();        
        if(DEBUG == true){
          if(state_PatternC != statePrevious_PatternC){
            Serial.print("State of C: ");
            Serial.println(state_PatternC);
          }
        }         
      }
      else if (inputPattern == 4){
        if (state_PatternD == 1){ //is in wait state, run it
          state_PatternD == 2;
        }
        FSM_patternD();        
        if(DEBUG == true){
          if(state_PatternD != statePrevious_PatternD){
            Serial.print("State of D: ");
            Serial.println(state_PatternD);
          }
        }         
      }
      else {
        FSM_default();
      }            
    }
    else { //isSync == false, not synced together
      //get data from temp/humidity sensor and decide what to do from there
    }
  }
  else {
      digitalWrite(toRegulatorA, LOW);
      digitalWrite(toRegulatorB, LOW);
      digitalWrite(toRegulatorC, LOW);
      digitalWrite(toRegulatorD, LOW);
      state_PatternA = 0; //put everything into standby
      state_PatternB = 0;
      state_PatternC = 0;
      state_PatternD = 0;
      count = 0; 
  }
}


void FSM_default(){
    digitalWrite(toRegulatorA, HIGH);
    digitalWrite(toRegulatorB, HIGH);
    digitalWrite(toRegulatorC, HIGH);
    digitalWrite(toRegulatorD, HIGH);
}

void FSM_patternA(){
    statePrevious_PatternA = state_PatternA;
     
    switch(state_PatternA){
      case(0): //RESET
        count = 0;
        state_PatternA = 1;
      break;
      case(1): //WAIT
        //Do nothing, let main program initiate
      case(2): //ITERATION 1 (Setup)
        state_PatternA = 3;
        timerStart_pattern = millis();
      break;
      case(3): //ITERATION 2
        digitalWrite(toRegulatorA, HIGH);
        digitalWrite(toRegulatorB, LOW);
        digitalWrite(toRegulatorC, LOW);
        digitalWrite(toRegulatorD, LOW);
        timer_pattern = millis();
        if (timer_pattern - timerStart_pattern > inputDelayTime) {
          state_PatternA = 4;
          timerStart_pattern = millis();
        }
      break;
      case(4): //ITERATION 3
        digitalWrite(toRegulatorA, HIGH);
        digitalWrite(toRegulatorB, HIGH);
        digitalWrite(toRegulatorC, LOW);
        digitalWrite(toRegulatorD, LOW);
        timer_pattern = millis();
        if (timer_pattern - timerStart_pattern > inputDelayTime) {
          state_PatternA = 5;
          timerStart_pattern = millis();
          }
      break;
      case(5): //ITERATION 4
        digitalWrite(toRegulatorA, HIGH);
        digitalWrite(toRegulatorB, HIGH);
        digitalWrite(toRegulatorC, HIGH);
        digitalWrite(toRegulatorD, LOW);
        timer_pattern = millis();
        if (timer_pattern - timerStart_pattern > inputDelayTime) {
          state_PatternA = 6;
          timerStart_pattern = millis();
          }
      break;
      case(6): //ITERATION 5
        digitalWrite(toRegulatorA, HIGH);
        digitalWrite(toRegulatorB, HIGH);
        digitalWrite(toRegulatorC, HIGH);
        digitalWrite(toRegulatorD, HIGH);
        timer_pattern = millis();
        if (timer_pattern - timerStart_pattern > inputDelayTime) {
          state_PatternA = 7;
          timerStart_pattern = millis();
          }
      break;
      case(7): //ITERATION 6
        digitalWrite(toRegulatorA, HIGH);
        digitalWrite(toRegulatorB, LOW);
        digitalWrite(toRegulatorC, LOW);
        digitalWrite(toRegulatorD, LOW);
        timer_pattern = millis();
        if (timer_pattern - timerStart_pattern > inputDelayTime) {
          count++;
          state_PatternA = 6;
          timerStart_pattern = millis();
          }
        if (count >= loopPatternA) {
          state_PatternA = 8;
        }
      break;
      case(8): //DONE
          if (inputPattern == 1) {
            state_PatternA = 2;
            count = 0;
          }
          else {
            state_PatternA = 0; 
          }
      break;
    }
}

void FSM_patternB(){
  statePrevious_PatternB = state_PatternB;
    switch(state_PatternB){
      case(0): //RESET
        count = 0;
        state_PatternB = 1;
      break;
      case(1): //WAIT
        //Do nothing, let main program initiate
      case(2): //ITERATION 1 (Setup)
        state_PatternB = 3;
        timerStart_pattern = millis();
      break;
      case(3): //ITERATION 2
        digitalWrite(toRegulatorA, HIGH);
        digitalWrite(toRegulatorB, LOW);
        digitalWrite(toRegulatorC, LOW);
        digitalWrite(toRegulatorD, LOW);
        timer_pattern = millis();
        if (timer_pattern - timerStart_pattern > inputDelayTime) {
          state_PatternB = 4;
          timerStart_pattern = millis();
        }
      break;
      case(4): //ITERATION 3
        digitalWrite(toRegulatorA, LOW);
        digitalWrite(toRegulatorB, HIGH);
        digitalWrite(toRegulatorC, LOW);
        digitalWrite(toRegulatorD, LOW);
        timer_pattern = millis();
        if (timer_pattern - timerStart_pattern > inputDelayTime) {
          state_PatternB = 5;
          timerStart_pattern = millis();
          }
      break;
      case(5): //ITERATION 4
        digitalWrite(toRegulatorA, LOW);
        digitalWrite(toRegulatorB, LOW);
        digitalWrite(toRegulatorC, HIGH);
        digitalWrite(toRegulatorD, LOW);
        timer_pattern = millis();
        if (timer_pattern - timerStart_pattern > inputDelayTime) {
          state_PatternB = 6;
          timerStart_pattern = millis();
          }
      break;
      case(6): //ITERATION 5
        digitalWrite(toRegulatorA, LOW);
        digitalWrite(toRegulatorB, LOW);
        digitalWrite(toRegulatorC, LOW);
        digitalWrite(toRegulatorD, HIGH);
        timer_pattern = millis();
        if (timer_pattern - timerStart_pattern > inputDelayTime) {
          count++;
          state_PatternB = 2;
          timerStart_pattern = millis();
          }
        if (count >= loopPatternB) {
          state_PatternB = 7;
        }
      break;
      case(7): //ITERATION 6
        digitalWrite(toRegulatorA, HIGH);
        digitalWrite(toRegulatorB, LOW);
        digitalWrite(toRegulatorC, LOW);
        digitalWrite(toRegulatorD, LOW);
        timer_pattern = millis();
        if (timer_pattern - timerStart_pattern > inputDelayTime) {
          state_PatternB = 8;
          timerStart_pattern = millis();
          }
      break;
      case(8): //ITERATION 7
        digitalWrite(toRegulatorA, HIGH);
        digitalWrite(toRegulatorB, HIGH);
        digitalWrite(toRegulatorC, HIGH);
        digitalWrite(toRegulatorD, HIGH);
        timer_pattern = millis();
        if (timer_pattern - timerStart_pattern > inputDelayTime) {
          state_PatternB = 9;
          timerStart_pattern = millis();
          }
      break;
      case(9): //ITERATION 8
        digitalWrite(toRegulatorA, LOW);
        digitalWrite(toRegulatorB, LOW);
        digitalWrite(toRegulatorC, LOW);
        digitalWrite(toRegulatorD, LOW);
        timer_pattern = millis();
        if (timer_pattern - timerStart_pattern > inputDelayTime) {
          state_PatternB = 10;
          timerStart_pattern = millis();
          }
      break;
      case(10): //DONE
          if (inputPattern == 2) {
            state_PatternB = 2;
            count = 0;
          }
          else {
            state_PatternB = 0; 
          }
      break;
    }
}

void FSM_patternC(){
   unsigned long temp_inputDelayTime = inputDelayTime;
   inputDelayTime = inputDelayTime / 2;
   statePrevious_PatternC = state_PatternC;
   switch(state_PatternC){
      case(0): //RESET
        count = 0;
        state_PatternC = 1;
      break;
      case(1): //WAIT
        //Do nothing, let main program initiate
      case(2): //ITERATION 1 (Setup)
        state_PatternC = 3;
        timerStart_pattern = millis();
      break;
      case(3): //ITERATION 2
        digitalWrite(toRegulatorA, LOW);
        digitalWrite(toRegulatorB, HIGH);
        digitalWrite(toRegulatorC, LOW);
        digitalWrite(toRegulatorD, HIGH);
        timer_pattern = millis();
        if (timer_pattern - timerStart_pattern > inputDelayTime) {
          state_PatternC = 4;
          timerStart_pattern = millis();
        }
      break;
      case(4): //ITERATION 3
        digitalWrite(toRegulatorA, HIGH);
        digitalWrite(toRegulatorB, LOW);
        digitalWrite(toRegulatorC, HIGH);
        digitalWrite(toRegulatorD, LOW);
        timer_pattern = millis();
        if (timer_pattern - timerStart_pattern > inputDelayTime) {
          count++;
          state_PatternC = 3;
          timerStart_pattern = millis();
          }
        if (count >= loopPatternC[0]) {
          state_PatternC = 5;
          count = 0;
        }
      break;
      case(5): //ITERATION 4
        digitalWrite(toRegulatorA, LOW);
        digitalWrite(toRegulatorB, HIGH);
        digitalWrite(toRegulatorC, LOW);
        digitalWrite(toRegulatorD, LOW);
        timer_pattern = millis();
        if (timer_pattern - timerStart_pattern > inputDelayTime) {
          state_PatternC = 6;
          timerStart_pattern = millis();
        }
      break;
      case(6): //ITERATION 5
        digitalWrite(toRegulatorA, LOW);
        digitalWrite(toRegulatorB, LOW);
        digitalWrite(toRegulatorC, HIGH);
        digitalWrite(toRegulatorD, LOW);
        timer_pattern = millis();
        if (timer_pattern - timerStart_pattern > inputDelayTime) {
          state_PatternC = 7;
          timerStart_pattern = millis();
        }
      break;
      case(7): //ITERATION 6
        digitalWrite(toRegulatorA, LOW);
        digitalWrite(toRegulatorB, LOW);
        digitalWrite(toRegulatorC, LOW);
        digitalWrite(toRegulatorD, HIGH);
        timer_pattern = millis();
        if (timer_pattern - timerStart_pattern > inputDelayTime) {
          state_PatternC = 8;
          timerStart_pattern = millis();
        }
      break;
      case(8): //ITERATION 7
        digitalWrite(toRegulatorA, HIGH);
        digitalWrite(toRegulatorB, LOW);
        digitalWrite(toRegulatorC, LOW);
        digitalWrite(toRegulatorD, LOW);
        timer_pattern = millis();
        if (timer_pattern - timerStart_pattern > inputDelayTime) {
          state_PatternC = 9;
          timerStart_pattern = millis();
        }
      break;
      case(9): //ITERATION 8
        digitalWrite(toRegulatorA, LOW);
        digitalWrite(toRegulatorB, HIGH);
        digitalWrite(toRegulatorC, HIGH);
        digitalWrite(toRegulatorD, HIGH);
        timer_pattern = millis();
        if (timer_pattern - timerStart_pattern > inputDelayTime) {
          count++;
          state_PatternC = 8;
          timerStart_pattern = millis();
          }
        if (count >= loopPatternC[1]) {
          state_PatternC = 10;
        }
      break;
      case(10): //DONE
          if (inputPattern == 3) {
            state_PatternC = 2;
            count = 0;
          }
          else {
            state_PatternC = 0; 
          }
      break;
    }
    inputDelayTime = temp_inputDelayTime;
}

void FSM_patternD(){
  
}

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&inputData, incomingData, sizeof(inputData));
  inputDelayTime = inputData.delayTime;
  inputPattern = inputData.pattern;
  inputIsActive = inputData.isActive;
  inputIsSync = inputData.isSync;
  digitalWrite(toRegulatorA, HIGH);
}

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  if (DEBUG == true) {
    char macStr[18];
    Serial.print("Packet to:");
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
         mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.print(macStr);
    Serial.print(" send status: ");
    if (sendStatus == 0){
      Serial.println("Delivery success");
    }
    else{
      Serial.println("Delivery fail");
    } 
  }
}
