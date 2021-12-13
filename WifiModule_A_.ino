//WIFI MODULE CODE FOR MODULE A

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <DHT.h>

#define testPin 5

// REPLACE WITH THE MAC Address of your receiver 
uint8_t broadcastAddress[] = {0x40,0xF5,0x20,0x3E,0x28,0x35}; //MAC of wifi module B

const long interval = 5000; 
unsigned long previousMillis = 0;    // will store last time DHT was updated 
unsigned long lastTime = 0;  
unsigned long timerDelay = 2000;  // send readings timer

String success;

typedef struct moduleBData { //module within home
  float frequency;
  int pattern;
  bool isActive;
} moduleBData;

typedef struct moduleAData { //module outside
  float temperature;
  float humidity;
  int pattern;
} moduleAData;

moduleAData outputData;
moduleBData inputData;

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
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

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&inputData, incomingData, sizeof(inputData));
  if (inputData.isActive == true) {
    digitalWrite(testPin, HIGH);
  }
  else {
    digitalWrite(testPin, LOW);
  }
}

void setup() {
  // Init Serial Monitor
  Serial.begin(115200);

  pinMode(testPin, OUTPUT);
 
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
}
 
void loop() {
    if ((millis() - lastTime) > timerDelay) {
      outputData.temperature = 1;
      outputData.humidity = 1;
      outputData.pattern = 1;
      esp_now_send(0, (uint8_t *) &outputData, sizeof(outputData));

      lastTime = millis();
  }
}
