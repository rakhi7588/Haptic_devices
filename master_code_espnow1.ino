#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>


// ============================================================
uint8_t DEVICE1_MAC[6] = {0xD8, 0x13, 0x2A, 0x25, 0x31, 0xAC};   // <-- Device 1 MAC
uint8_t DEVICE2_MAC[6] = {0x68, 0x25, 0xDD, 0xE1, 0x51, 0x4C};   // <-- Device 2 MAC
uint8_t DEVICE3_MAC[6] = {0x14, 0x33, 0x5C, 0x31, 0xC4, 0xAC};   // <-- Device 3 MAC
uint8_t DEVICE4_MAC[6] = {0x14, 0x33, 0x5C, 0x31, 0x61, 0x64};   // <-- Device 4 MAC
// ============================================================

// Shared packet — must match slave exactly
typedef struct __attribute__((packed)) {
  uint8_t device_id;
  float   spring_impact;
  float   damping;
  float   vibration_amplitude;
  float   vibration_frequency;
  int     enable_brake;
} HapticPacket;

void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {}

// Get the MAC pointer for a given device ID
uint8_t* getMacForDevice(uint8_t id) {
  switch (id) {
    case 1: return DEVICE1_MAC;
    case 2: return DEVICE2_MAC;
    case 3: return DEVICE3_MAC;
    case 4: return DEVICE4_MAC;
    default: return nullptr;
  }
}

void addPeer(uint8_t *mac) {
  esp_now_peer_info_t peer;
  memset(&peer, 0, sizeof(peer));
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(500);

  Serial.print("Master MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: ESP-NOW init failed!");
    delay(1000);
    ESP.restart();
  }

  esp_now_register_send_cb(onDataSent);

  // Register all 4 devices as peers
  addPeer(DEVICE1_MAC);
  addPeer(DEVICE2_MAC);
  addPeer(DEVICE3_MAC);
  addPeer(DEVICE4_MAC);

  Serial.println("MASTER_READY");
  Serial.println("Send commands: deviceID;spring;damping;vibAmp;vibFreq;brake");
}

void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) return;

    // Parse: deviceID;spring;damping;vibAmp;vibFreq;brake
    HapticPacket pkt;
    int idx = 0;
    char buf[100];
    line.toCharArray(buf, sizeof(buf));

    char *token = strtok(buf, ";");
    if (token == NULL) return;
    pkt.device_id = (uint8_t)atoi(token);

    token = strtok(NULL, ";"); if (token == NULL) return;
    pkt.spring_impact = atof(token);

    token = strtok(NULL, ";"); if (token == NULL) return;
    pkt.damping = atof(token);

    token = strtok(NULL, ";"); if (token == NULL) return;
    pkt.vibration_amplitude = atof(token);

    token = strtok(NULL, ";"); if (token == NULL) return;
    pkt.vibration_frequency = atof(token);

    token = strtok(NULL, ";");
    pkt.enable_brake = (token != NULL) ? atoi(token) : 0;

    // Send to the correct device
    uint8_t *mac = getMacForDevice(pkt.device_id);
    if (mac != nullptr) {
      esp_now_send(mac, (uint8_t*)&pkt, sizeof(HapticPacket));
      Serial.printf("Sent to Device %d\n", pkt.device_id);
    } else {
      Serial.printf("ERROR: Unknown device ID %d\n", pkt.device_id);
    }
  }
}