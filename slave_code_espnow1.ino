#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#define DEVICE_ID 4


// Shared packet — must match master exactly
typedef struct __attribute__((packed)) {
  uint8_t device_id;
  float   spring_impact;
  float   damping;
  float   vibration_amplitude;
  float   vibration_frequency;
  int     enable_brake;
} HapticPacket;

// These are the values the motor code will use later
float spring_impact = 0;
float damping = 0;
float vibration_amplitude = 0;
float vibration_frequency = 0;
int   enable_brake = 0;

unsigned long time_last_action_ms = 0;

// ESP-NOW receive callback — fires when master sends data
void onDataReceived(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(HapticPacket)) return;

  HapticPacket pkt;
  memcpy(&pkt, data, sizeof(HapticPacket));

  // Only act if this packet is for THIS device
  if (pkt.device_id != DEVICE_ID) return;

  // Store the values
  spring_impact       = pkt.spring_impact;
  damping             = pkt.damping;
  vibration_amplitude = pkt.vibration_amplitude;
  vibration_frequency = pkt.vibration_frequency;
  enable_brake        = pkt.enable_brake;

  time_last_action_ms = millis();

  // ===== TEST VERSION: print instead of driving motor =====
  // Later, replace this block with the supervisor's motor code
  Serial.printf("Device %d received: spring=%.2f damping=%.2f vib_amp=%.2f vib_freq=%.2f brake=%d\n",
    DEVICE_ID, spring_impact, damping, vibration_amplitude, vibration_frequency, enable_brake);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(500);

  Serial.printf("Haptic Device %d starting\n", DEVICE_ID);
  Serial.print("My MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: ESP-NOW init failed!");
    delay(1000);
    ESP.restart();
  }

  esp_now_register_recv_cb(onDataReceived);

  Serial.printf("Device %d READY. Waiting for commands...\n", DEVICE_ID);
}

void loop() {
  // In the real version, the motor control code runs here
  // For now, nothing needed- the callback handles received data
  delay(10);
}