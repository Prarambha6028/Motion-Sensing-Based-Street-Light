
#define BLYNK_TEMPLATE_ID "TMPL3v-7pd2Yp"
#define BLYNK_TEMPLATE_NAME "Msbsl"
#define BLYNK_AUTH_TOKEN "Y3HBA3h0TsKzUVVoVycvxliDmvvoQbwY"

#include <WiFi.h>
#include <esp_now.h>
#include <BlynkSimpleEsp32.h>


#define led     16   // Main LED pin
#define pirpin  13   // PIR sensor pin (master)
#define ldrpin  36   // LDR sensor pin (digital output)


const char* WIFI_NAME = "realme 14 Pro lite 5G (128)";
const char* WIFI_PASSWORD = "z63ajyfu";


uint8_t SlaveAddress[6] = {0x14, 0x33, 0x5C, 0x04, 0x4A, 0xFC}; //Store MAC Address(helps to knows which esp to send/receive data to)


typedef struct struct_message { //Data container
  char msbsl[16]; //character array here it receives and strores a string 
} struct_message;

struct_message mydata; //this holds the string for further

//Varying Voltage as pulse(On and Off)
const int pwmch = 0, pwmfreq = 2000, pwmreso = 8; //16 channel thatt creates pwm,pwmfreq=how often it goes hign and low,here 2000hz per seconds.


int MaxBrightness = 179;    // Full brightness level
int LowBrightness = 26;     // Dim brightness level
const int dimstep = 26;     // Brightness decrement per step


unsigned long previousMillis = 0; // Last motion or brightness change time
const long timeout = 10000;       // After 10s without motion, start dimming
const long interval = 1000;       // Dim every 1 second


bool pirState = LOW;          // Current PIR state
bool lastPIR = LOW;           // Last PIR state for edge detection
bool lastNight = false;       // Track last night/day state
bool ldrnow;                  // Store LDR reading
int brightness = LowBrightness; // Current LED brightness


bool motionFromSlave = false;         
unsigned long slaveMotionTime = 0;
const unsigned long slaveMotionTimeout = 5000; 


int masterMotionCount = 0;
int slaveMotionCount = 0;
// Callback when data is receive

void onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&mydata, incomingData, sizeof(mydata));
  if (strcmp(mydata.msbsl, "MOTION") == 0) {
    motionFromSlave = true;
    slaveMotionTime = millis();
    slaveMotionCount++;
    Blynk.virtualWrite(V1, slaveMotionCount); // Send to Blynk graph
  }
}

void setup() {
  Serial.begin(115200);

  // Pin setup
  pinMode(pirpin, INPUT);
  pinMode(ldrpin, INPUT);  
  pinMode(led, OUTPUT);

  // PWM setup
  ledcSetup(pwmch, pwmfreq, pwmreso);
  ledcAttachPin(led, pwmch);

  // Connect to WiFi
  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
   
  }

  // Start Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_NAME, WIFI_PASSWORD);

  // Set WiFi to station mode for ESP-NOW
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    
    ESP.restart();
  }

  // Register peer (Slave)
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, SlaveAddress, 6);
  peerInfo.channel = WiFi.channel(); // Same channel as WiFi
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
  }

  // Register receive callback
  esp_now_register_recv_cb(onDataRecv);

  ledcWrite(pwmch, brightness);//set the duty cycle of the PWM signal.
  previousMillis = millis();
}

void loop() {
  Blynk.run(); // Run Blynk tasks

  unsigned long currentMillis = millis();
  bool pirValue = digitalRead(pirpin);
  bool ldrValue = digitalRead(ldrpin);
  bool night = (ldrValue == 1);

  

  // PIR change detection 
  if (pirValue != lastPIR) {
    
    lastPIR = pirValue;
  }

  // Nightfall detection
  if (night && !lastNight) {
    
    brightness = LowBrightness;
    previousMillis = currentMillis;
  }
  lastNight = night;

  // Reset slave motion after timeout
  if (motionFromSlave && (currentMillis - slaveMotionTime > slaveMotionTimeout)) {
    motionFromSlave = false;
  }

  // Motion from master or slave
  if ((pirValue || motionFromSlave) && night) {
    ledcWrite(pwmch, MaxBrightness);
    previousMillis = currentMillis;
    motionFromSlave = false; // Reset after use
    if (pirValue) {
      masterMotionCount++;
      Blynk.virtualWrite(V2, masterMotionCount); // Send to Blynk graph
    }
  }

  // No motion → set to low brightness after 1s
  if (!pirValue && (currentMillis - previousMillis >= interval)) {
    brightness = LowBrightness;
  }

  // Dimming over time if no motion for 'timeout'
  if ((currentMillis - previousMillis > timeout) && night) {
    if (brightness > LowBrightness) {
      brightness -= dimstep;
      if (brightness < LowBrightness) brightness = LowBrightness;
      
      delay(1000); 
    }
  }

  // Daytime → LED OFF
  if (!night) {
    /*if (brightness != 0) {
      
    }*/
    brightness = 0;
  }

  // Apply brightness
  ledcWrite(pwmch, brightness);
  delay(200);
}
