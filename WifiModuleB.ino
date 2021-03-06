//WIFI MODULE CODE FOR MODULE B
#include <math.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

//VARIABLES FOR ACTIVATION BUTTON:
const int activationButton = 5; //D1 (activation/reset button)
  int value_activation = 0;
  unsigned long timer_activation = 0;
  unsigned long timerStart_activation = 0;
  unsigned long bounce_delay_activation = 5;
  int state_activation = 0;
  int statePrevious_activation = 0;

//VARIABLES FOR SYNCHRONIZATION BUTTON:
const int syncButton = 0; //D3 (synchronization button)
  int value_sync = 0;
  unsigned long timer_sync = 0;
  unsigned long timerStart_sync = 0;
  unsigned long bounce_delay_sync = 5;
  int state_sync = 0;
  int statePrevious_sync = 0;

//VARIABLES FOR PROGRAM INCREMENTOR:
const int selectProgramButton = 4; //D2 (incrememts till 4 then resets to 0)
  int value_program = 0;
  unsigned long timer_program = 0;
  unsigned long timerStart_program = 0;
  unsigned long bounce_delay_program = 5;
  int state_program = 0;
  int statePrevious_program = 0;
  int selectedProgramValue = 0;

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

//VARIABLES FOR DELAY TIMER: (TIMINGS FOR DELAYS)
const int frequencySelector = A0; //A0 (for analog read)
  unsigned long delay_pattern = 5000;
  float delay_pattern_threshold = 0.0025; //must be exceeding 0.25% to be registered as a change
  float value_delay_pattern = 0;
  
//CONTROL VARIABLES:
const bool DEBUG = true;
  bool systemRunning = false;
  bool modulesSynced = true;
  int count = 0;
const int loopPatternA = 3;
const int loopPatternB = 2;
const int loopPatternC[2] = {2,3};
const int numberPatterns = 4; //number of patterns that currently exist
unsigned long systemRunTimeMax = 14400000; //(milliseconds) = 4 hours runtime
unsigned long currentRunTime = 0;
unsigned long lastRunTime = 0;


//STRUCTURE TO BE SENT OUT:
typedef struct moduleBData { //module within home
  unsigned long delayTime = delay_pattern;
  int pattern = 0;
  bool isActive = false;
  bool isSync = true;
  int statePatternA = 0;
  int statePatternB = 0;
  int statePatternC = 0;
  int statePatternD = 0;
} moduleBData;

//STRUCTURE TO BE RECEIVED:
typedef struct moduleAData { //module outside
  float temperature;
  float humidity;
} moduleAData;

//VARIABLES DEALING WITH WIFI:
uint8_t broadcastAddress[] = {0x50, 0x02, 0x91, 0xE0, 0x53, 0xE1}; //MAC of wifi module A
  moduleBData outputData; //package to be shipped to module A
  moduleAData inputData;
  bool isSent = false;

void setup() {
  pinMode(activationButton, INPUT);
  pinMode(selectProgramButton, INPUT);
  pinMode(frequencySelector, INPUT);
  pinMode(syncButton, INPUT);

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

  debounceActivationButton();
  debounceSynchronizationButton();

  if (state_activation == 5){
    systemRunning = !systemRunning;
    outputData.isActive = systemRunning; // change in activation 
    outputData.isSync = modulesSynced;
    esp_now_send(0, (uint8_t *) &outputData, sizeof(outputData));
    if (systemRunning == true) {
      lastRunTime = millis();
    }
  }
  if (state_sync == 5){
    modulesSynced = !modulesSynced;
    outputData.isActive = systemRunning; // change in activation 
    outputData.isSync = modulesSynced;
    esp_now_send(0, (uint8_t *) &outputData, sizeof(outputData));
  }
  
  if (systemRunning == true) {
      debounceProgramButton();
      updateDelayTimer();
      if (modulesSynced == true) {
        updateOutputStates();
        esp_now_send(0, (uint8_t *) &outputData, sizeof(outputData));
        if (DEBUG == true) {
          Serial.println("updateDelayTimer's send was run");        
        }
      }
  runPatterns(); 
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
  currentRunTime = millis();
  if (currentRunTime - lastRunTime > systemRunTimeMax) {
        digitalWrite(toRegulatorA, LOW);
        digitalWrite(toRegulatorB, LOW);
        digitalWrite(toRegulatorC, LOW);
        digitalWrite(toRegulatorD, LOW);
        state_PatternA = 0; //put everything into standby
        state_PatternB = 0;
        state_PatternC = 0;
        state_PatternD = 0;
        count = 0; 
        systemRunning = false;
        outputData.isActive = systemRunning;
        outputData.statePatternA = state_PatternA;
        outputData.statePatternB = state_PatternB;
        outputData.statePatternC = state_PatternC;
        outputData.statePatternD = state_PatternD;
        esp_now_send(0, (uint8_t *) &outputData, sizeof(outputData));
  }
}

void debounceProgramButton(){//state machine
  statePrevious_program = state_program;
  switch(state_program){
    case(0): //RESET
      state_program = 1;
    break;
    case(1): //START
      value_program = digitalRead(selectProgramButton);
      if (value_program == HIGH) {state_program = 2;}
    break;
    case(2): //GO
      timerStart_program = millis();
      state_program = 3;
    break;
    case(3): //WAIT
      value_program = digitalRead(selectProgramButton);
      timer_program = millis();
      if (value_program == LOW) {state_program = 0;}
      if (timer_program - timerStart_program > bounce_delay_program) {state_program = 4;}
    break;
    case(4): //ARMED
      value_program = digitalRead(selectProgramButton);
      if (value_program == LOW) {state_program = 5;}
    break;
    case(5): //TRIGGERED
      if (DEBUG == true){
        Serial.println("Program Button Triggered");
      }
      selectedProgramValue++;
      if (selectedProgramValue > numberPatterns) {
        selectedProgramValue = 0;
      }
      outputData.pattern = selectedProgramValue;
      if (modulesSynced == true) {
        esp_now_send(0, (uint8_t *) &outputData, sizeof(outputData));
        if (DEBUG == true) {
          Serial.println("SelectPattern's send was run");        
        }        
      }
      state_program = 0;
    break;
  }
}

void debounceActivationButton(){//state machine
  statePrevious_activation = state_activation;
  switch(state_activation){
    case(0): //RESET
      state_activation = 1;
    break;
    case(1): //START
      value_activation = digitalRead(activationButton);
      if (value_activation == HIGH) {state_activation = 2;}
    break;
    case(2): //GO
      timerStart_activation = millis();
      state_activation = 3;
    break;
    case(3): //WAIT
      value_activation = digitalRead(activationButton);
      timer_activation = millis();
      if (value_activation == LOW) {state_activation = 0;}
      if (timer_activation - timerStart_activation > bounce_delay_activation) {state_activation = 4;}
    break;
    case(4): //ARMED
      value_activation = digitalRead(activationButton);
      if (value_activation == LOW) {state_activation = 5;}
    break;
    case(5): //TRIGGERED
      if (DEBUG == true){
        Serial.println("Activation Triggered");
      }
      state_activation = 0;
    break;
  }
}

void debounceSynchronizationButton(){//state machine
  statePrevious_sync = state_sync;
  switch(state_sync){
    case(0): //RESET
      state_sync = 1;
    break;
    case(1): //START
      value_sync = digitalRead(syncButton);
      if (value_sync == HIGH) {state_sync = 2;}
    break;
    case(2): //GO
      timerStart_sync = millis();
      state_sync = 3;
    break;
    case(3): //WAIT
      value_sync = digitalRead(syncButton);
      timer_sync = millis();
      if (value_sync == LOW) {state_sync = 0;}
      if (timer_sync - timerStart_sync > bounce_delay_sync) {state_sync = 4;}
    break;
    case(4): //ARMED
      value_sync = digitalRead(syncButton);
      if (value_sync == LOW) {state_sync = 5;}
    break;
    case(5): //TRIGGERED
      if (DEBUG == true){
        Serial.println("Synchronization Triggered");
      }
      state_sync = 0;
    break;
  }
}

void updateDelayTimer(){
    value_delay_pattern = log(static_cast<float>(analogRead(frequencySelector)+ 1023)/1023); // downscales frequency as a multiplier
    delay_pattern = 5000; 
    delay_pattern = delay_pattern + (delay_pattern * value_delay_pattern);
    outputData.delayTime = delay_pattern;
}

void updateOutputStates(){
    outputData.statePatternA = state_PatternA;
    outputData.statePatternB = state_PatternB;
    outputData.statePatternC = state_PatternC;
    outputData.statePatternD = state_PatternD;
}

void runPatterns(){
      if (selectedProgramValue == 1){
        if (state_PatternA == 1){ //is in wait state, run it
          state_PatternA == 2;
          state_PatternB = 0;
          state_PatternC = 0;
          state_PatternD = 0;
        }
        FSM_patternA();        
        if(DEBUG == true){
          if(state_PatternA != statePrevious_PatternA){
            Serial.print("State of A: ");
            Serial.println(state_PatternA);
          }
        }
      }
      else if (selectedProgramValue == 2){
        if (state_PatternB == 1){ //is in wait state, run it
          state_PatternB == 2;
          state_PatternA = 0;
          state_PatternC = 0;
          state_PatternD = 0;
        }
        FSM_patternB();        
        if(DEBUG == true){
          if(state_PatternB != statePrevious_PatternB){
            Serial.print("State of B: ");
            Serial.println(state_PatternB);
          }
        }        
      }
      else if (selectedProgramValue == 3){
        if (state_PatternC == 1){ //is in wait state, run it
          state_PatternC == 2;
          state_PatternA = 0;
          state_PatternB = 0;
          state_PatternD = 0;
        }
        FSM_patternC();        
        if(DEBUG == true){
          if(state_PatternC != statePrevious_PatternC){
            Serial.print("State of C: ");
            Serial.println(state_PatternC);
          }
        }         
      }
      else if (selectedProgramValue == 4){
        if (state_PatternD == 1){ //is in wait state, run it
          state_PatternD == 2;
          state_PatternA = 0;
          state_PatternB = 0;
          state_PatternC = 0;
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
        if (timer_pattern - timerStart_pattern > delay_pattern) {
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
        if (timer_pattern - timerStart_pattern > delay_pattern) {
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
        if (timer_pattern - timerStart_pattern > delay_pattern) {
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
        if (timer_pattern - timerStart_pattern > delay_pattern) {
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
        if (timer_pattern - timerStart_pattern > delay_pattern) {
          count++;
          state_PatternA = 6;
          timerStart_pattern = millis();
          }
        if (count >= loopPatternA) {
          state_PatternA = 8;
        }
      break;
      case(8): //DONE
          if (selectedProgramValue == 1) {
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
        if (timer_pattern - timerStart_pattern > delay_pattern) {
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
        if (timer_pattern - timerStart_pattern > delay_pattern) {
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
        if (timer_pattern - timerStart_pattern > delay_pattern) {
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
        if (timer_pattern - timerStart_pattern > delay_pattern) {
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
        if (timer_pattern - timerStart_pattern > delay_pattern) {
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
        if (timer_pattern - timerStart_pattern > delay_pattern) {
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
        if (timer_pattern - timerStart_pattern > delay_pattern) {
          state_PatternB = 10;
          timerStart_pattern = millis();
          }
      break;
      case(10): //DONE
          if (selectedProgramValue == 2) {
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
   unsigned long temp_delay_pattern = delay_pattern;
   delay_pattern = delay_pattern / 2;
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
        if (timer_pattern - timerStart_pattern > delay_pattern) {
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
        if (timer_pattern - timerStart_pattern > delay_pattern) {
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
        if (timer_pattern - timerStart_pattern > delay_pattern) {
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
        if (timer_pattern - timerStart_pattern > delay_pattern) {
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
        if (timer_pattern - timerStart_pattern > delay_pattern) {
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
        if (timer_pattern - timerStart_pattern > delay_pattern) {
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
        if (timer_pattern - timerStart_pattern > delay_pattern) {
          count++;
          state_PatternC = 8;
          timerStart_pattern = millis();
          }
        if (count >= loopPatternC[1]) {
          state_PatternC = 10;
        }
      break;
      case(10): //DONE
          if (selectedProgramValue == 3) {
            state_PatternC = 2;
            count = 0;
          }
          else {
            state_PatternC = 0; 
          }
      break;
    }
    delay_pattern = temp_delay_pattern;
}

void FSM_patternD(){
    unsigned long temp_delay_pattern = delay_pattern;
    delay_pattern = delay_pattern / 3;
    statePrevious_PatternD = state_PatternD;
    switch(state_PatternD){
      case(0): //RESET
        count = 0;
        state_PatternD = 1;
      break;
      case(1): //WAIT
        //Do nothing, let main program initiate
      case(2): //ITERATION 1 (Setup)
        state_PatternD = 3;
        timerStart_pattern = millis();
      break;
      case(3): //ITERATION 2
        digitalWrite(toRegulatorA, HIGH);
        digitalWrite(toRegulatorB, LOW);
        digitalWrite(toRegulatorC, LOW);
        digitalWrite(toRegulatorD, LOW);
        timer_pattern = millis();
        if (timer_pattern - timerStart_pattern > delay_pattern) {
          state_PatternD = 4;
          timerStart_pattern = millis();
        }
      break;
      case(4): //ITERATION 3
        digitalWrite(toRegulatorA, HIGH);
        digitalWrite(toRegulatorB, HIGH);
        digitalWrite(toRegulatorC, HIGH);
        digitalWrite(toRegulatorD, HIGH);
        timer_pattern = millis();
        if (timer_pattern - timerStart_pattern > delay_pattern) {
          state_PatternD = 5;
          timerStart_pattern = millis();
          }
      break;
      case(5): //DONE
        if (selectedProgramValue == 4) {
          state_PatternD = 2;
        }
        else {
          state_PatternD = 0; 
        }
      break;
    }
    delay_pattern = temp_delay_pattern;
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
  if (sendStatus == 0){
    isSent = true;
  }
  else {
    isSent = false;
  }
}

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&inputData, incomingData, sizeof(inputData));
}
