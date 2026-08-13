#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
// change the device name to 2,3,4 for other slave devices
#define DEVICE_ID 1


#define WIFI_CHANNEL 6     

typedef struct __attribute__((packed)) {
  uint8_t device_id;
  float   spring_impact;
  float   damping;
  float   vibration_amplitude;
  float   vibration_frequency;
  int     enable_brake;
} HapticPacket;

float spring_impact = 0;
float damping = 0;
float vibration_amplitude = 0;
float vibration_frequency = 0;
int   enable_brake = 0;

void onDataReceived(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(HapticPacket)) return;

  HapticPacket pkt;
  memcpy(&pkt, data, sizeof(HapticPacket));

  if (pkt.device_id != DEVICE_ID) return;

  spring_impact       = pkt.spring_impact;
  damping             = pkt.damping;
  vibration_amplitude = pkt.vibration_amplitude;
  vibration_frequency = pkt.vibration_frequency;
  enable_brake        = pkt.enable_brake;

  // TEST: print. Later replace with motor code.
  Serial.printf("Device %d received: spring=%.2f damping=%.2f vib_amp=%.2f vib_freq=%.2f brake=%d\n",
    DEVICE_ID, spring_impact, damping, vibration_amplitude, vibration_frequency, enable_brake);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(500);

  // Lock this slave to the same channel as the master's Wi-Fi
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

  Serial.printf("Haptic Device %d starting\n", DEVICE_ID);
  Serial.print("My MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.printf("Locked to Wi-Fi channel: %d\n", WIFI_CHANNEL);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: ESP-NOW init failed!");
    delay(1000);
    ESP.restart();
  }

  esp_now_register_recv_cb(onDataReceived);

  Serial.printf("Device %d READY. Waiting for commands...\n", DEVICE_ID);
}

void loop() {
  delay(10);
}