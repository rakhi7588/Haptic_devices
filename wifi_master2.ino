#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <esp_wifi.h>

// ============================================================
//  >>>>>  PUT YOUR 4 SLAVE MAC ADDRESSES HERE  <<<
// ============================================================
uint8_t DEVICE1_MAC[6] = {0xD8, 0x13, 0x2A, 0x25, 0x31, 0xAC};
uint8_t DEVICE2_MAC[6] = {0x68, 0x25, 0xDD, 0xE1, 0x51, 0x4C};
uint8_t DEVICE3_MAC[6] = {0x14, 0x33, 0x5C, 0x31, 0xC4, 0xAC};
uint8_t DEVICE4_MAC[6] = {0x14, 0x33, 0x5C, 0x31, 0x61, 0x64};

// ============================================================
//  >>>>>  YOUR WIFI NAME AND PASSWORD  <<<
// ============================================================
const char* WIFI_SSID     = "TP-LINK_***C";
const char* WIFI_PASSWORD = "********";

#define UDP_PORT 4210

WiFiUDP udp;
char udpBuffer[100];

typedef struct __attribute__((packed)) {
  uint8_t device_id;
  float   spring_impact;
  float   damping;
  float   vibration_amplitude;
  float   vibration_frequency;
  int     enable_brake;
} HapticPacket;

void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {}

uint8_t* getMacForDevice(uint8_t id) {
  switch (id) {
    case 1: return DEVICE1_MAC;
    case 2: return DEVICE2_MAC;
    case 3: return DEVICE3_MAC;
    case 4: return DEVICE4_MAC;
    default: return nullptr;
  }
}

void addPeer(uint8_t *mac, uint8_t channel) {
  esp_now_peer_info_t peer;
  memset(&peer, 0, sizeof(peer));
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = channel;    // use the router's channel
  peer.encrypt = false;
  esp_now_add_peer(&peer);
}

void processCommand(char *cmdBuffer) {
  HapticPacket pkt;

  char *token = strtok(cmdBuffer, ";");
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

  uint8_t *mac = getMacForDevice(pkt.device_id);
  if (mac != nullptr) {
    esp_now_send(mac, (uint8_t*)&pkt, sizeof(HapticPacket));
    Serial.printf("Sent to Device %d\n", pkt.device_id);
  } else {
    Serial.printf("ERROR: Unknown device ID %d\n", pkt.device_id);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);

  Serial.printf("Connecting to Wi-Fi: %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  uint8_t wifiChannel = 1;   // default

  if (WiFi.status() == WL_CONNECTED) {
    wifiChannel = WiFi.channel();   // get the router's channel AFTER connecting
    Serial.println("\nWi-Fi connected!");
    Serial.print("Master IP address: ");
    Serial.println(WiFi.localIP());
    Serial.printf(">>> Wi-Fi CHANNEL: %d  <<<  (use this number in slaves)\n", wifiChannel);
    Serial.printf("Listening for UDP on port %d\n", UDP_PORT);
    udp.begin(UDP_PORT);
  } else {
    Serial.println("\nWi-Fi FAILED. Serial still works.");
  }

  Serial.print("Master MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: ESP-NOW init failed!");
    delay(1000);
    ESP.restart();
  }
  esp_now_register_send_cb(onDataSent);

  // Add peers on the SAME channel as Wi-Fi
  addPeer(DEVICE1_MAC, wifiChannel);
  addPeer(DEVICE2_MAC, wifiChannel);
  addPeer(DEVICE3_MAC, wifiChannel);
  addPeer(DEVICE4_MAC, wifiChannel);

  Serial.println("MASTER_READY");
  Serial.println("Accepts commands via USB Serial AND Wi-Fi UDP");
}

void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) {
      char buf[100];
      line.toCharArray(buf, sizeof(buf));
      processCommand(buf);
    }
  }

  int packetSize = udp.parsePacket();
  if (packetSize) {
    int len = udp.read(udpBuffer, sizeof(udpBuffer) - 1);
    if (len > 0) {
      udpBuffer[len] = '\0';
      for (int i = 0; i < len; i++) {
        if (udpBuffer[i] == '\n' || udpBuffer[i] == '\r') udpBuffer[i] = '\0';
      }
      Serial.printf("UDP received: %s\n", udpBuffer);
      processCommand(udpBuffer);
    }
  }
}