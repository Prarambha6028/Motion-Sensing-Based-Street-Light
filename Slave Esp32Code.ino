#include <WiFi.h>
#include <esp_now.h>

#define SMALL_LED 2       // Small indicator LED (on/off)
#define PIR_PIN 27         // PIR sensor pin


const char* WIFI_NAME = "your wifi ssid";
const char* WIFI_PASSWORD = "password";

// Master's MAC address
uint8_t masterAddress[] = { 0x00, 0x4B, 0x12, 0x33, 0x7E, 0x00 };

// Structure to receive ESP-NOW data
typedef struct struct_message {
  char msbsl[16];  // expects "MOTION"
} struct_message;

struct_message myData;

bool lastPIR = false;
unsigned long ledStart = 0;
bool ledActive = false;

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  pinMode(SMALL_LED, OUTPUT);

  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[Slave] WiFi connected");
  Serial.println("[Slave] Channel: " + String(WiFi.channel()));

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    ESP.restart();
  }
   esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, masterAddress, 6);
  peerInfo.channel = WiFi.channel();  // dynamic and correct  
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add master as peer");
  }

  Serial.println("[Slave] ESP-NOW setup complete");
}

void loop() {
  bool pir = digitalRead(PIR_PIN);

  // Only trigger on rising edge (LOW to HIGH)
  if (pir && !lastPIR) {
    Serial.println("[Slave] PIR motion detected");

    // Turn on status LED for feedback
    digitalWrite(SMALL_LED, HIGH);
    ledStart = millis();
    ledActive = true;

    // Send motion message
    strcpy(myData.msbsl, "MOTION");
    esp_err_t result = esp_now_send(masterAddress, (uint8_t *)&myData, sizeof(myData));
    if (result == ESP_OK) {
      Serial.println("[Slave] Motion data sent to master");
    } else {
      Serial.println("[Slave] Failed to send motion data");
    }
  }

  lastPIR = pir;

  // Turn off LED after 5 seconds
  if (ledActive && (millis() - ledStart > 5000)) {
    digitalWrite(SMALL_LED, LOW);
    ledActive = false;
  }

  delay(20); // Light loop
}

    strcpy(myData.msbsl, "MOTION");
    esp_err_t result = esp_now_send(masterAddress, (uint8_t *)&myData, sizeof(myData));
    Serial.println(result == ESP_OK ? "[Slave] Motion data sent" : "[Slave] Send failed");
  }
  lastPIR = pir;

  if (ledActive && (millis() - ledStart > 5000)) {
    digitalWrite(STATUS_LED, LOW);
    ledActive = false;
  }

  delay(20);
}

