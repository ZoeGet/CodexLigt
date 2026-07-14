#include <Arduino.h>
#include <cstring>

#include "config.h"
#include "led.h"

#if __has_include("wifi_secrets.h")
#include <WiFi.h>
#include <WiFiUdp.h>
#include "wifi_secrets.h"
#define CODEXLIGHT_WIFI_AVAILABLE 1
#else
#define CODEXLIGHT_WIFI_AVAILABLE 0
#endif

namespace {

using namespace CodexLightConfig;

LedController leds;
String serialBuffer;
unsigned long lastWirelessPacketMs = 0;

#if CODEXLIGHT_WIFI_AVAILABLE
WiFiUDP udp;
bool udpStarted = false;
unsigned long lastWifiAttemptMs = 0;
#endif

enum class LightState {
  Red,
  Green,
  Yellow,
  Unknown,
};

String normalizeCommand(String command) {
  command.trim();
  command.toUpperCase();

  constexpr const char* prefix = "CODEXLIGHT/1 ";
  if (command.startsWith(prefix)) {
    command = command.substring(strlen(prefix));
    command.trim();
  }

  return command;
}

LightState parseState(String command) {
  command = normalizeCommand(command);

  if (command == "RED") {
    return LightState::Red;
  }
  if (command == "GREEN") {
    return LightState::Green;
  }
  if (command == "YELLOW") {
    return LightState::Yellow;
  }
  return LightState::Unknown;
}

void applyState(LightState state) {
  switch (state) {
    case LightState::Red:
      leds.showRed();
      break;
    case LightState::Green:
      leds.showGreen();
      break;
    case LightState::Yellow:
      leds.showYellow();
      break;
    case LightState::Unknown:
      break;
  }
}

void handleCommand(String command) {
  const LightState state = parseState(command);
  if (state != LightState::Unknown) {
    applyState(state);
  }
}

void handleSerialInput() {
  while (Serial.available() > 0) {
    const char ch = static_cast<char>(Serial.read());
    if (ch == '\n' || ch == '\r') {
      if (serialBuffer.length() > 0) {
        handleCommand(serialBuffer);
        serialBuffer = "";
      }
      continue;
    }

    if (serialBuffer.length() < 64) {
      serialBuffer += ch;
    } else {
      serialBuffer = "";
    }
  }
}

#if CODEXLIGHT_WIFI_AVAILABLE
void maintainWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!udpStarted) {
      udpStarted = udp.begin(UDP_PORT) == 1;
      if (udpStarted) {
        lastWirelessPacketMs = millis();
      }
    }
    return;
  }

  udpStarted = false;
  const unsigned long now = millis();
  if (now - lastWifiAttemptMs < 5000UL) {
    return;
  }

  lastWifiAttemptMs = now;
  WiFi.mode(WIFI_STA);
  WiFi.begin(CODEXLIGHT_WIFI_SSID, CODEXLIGHT_WIFI_PASSWORD);
}

void handleUdpInput() {
  if (!udpStarted) {
    return;
  }

  const int packetSize = udp.parsePacket();
  if (packetSize <= 0) {
    return;
  }

  char packet[80];
  const int len = udp.read(packet, sizeof(packet) - 1);
  if (len <= 0) {
    return;
  }

  packet[len] = '\0';
  lastWirelessPacketMs = millis();
  handleCommand(String(packet));
}

void handleWirelessTimeout() {
  if (!udpStarted || lastWirelessPacketMs == 0) {
    return;
  }

  if (millis() - lastWirelessPacketMs > WIRELESS_TIMEOUT_MS) {
    leds.showYellow();
    lastWirelessPacketMs = millis();
  }
}
#endif

}  // namespace

void setup() {
  Serial.begin(SERIAL_BAUD);
  leds.begin();
  leds.showGreen();

#if CODEXLIGHT_WIFI_AVAILABLE
  WiFi.mode(WIFI_STA);
  WiFi.begin(CODEXLIGHT_WIFI_SSID, CODEXLIGHT_WIFI_PASSWORD);
#endif
}

void loop() {
  handleSerialInput();

#if CODEXLIGHT_WIFI_AVAILABLE
  maintainWifi();
  handleUdpInput();
  handleWirelessTimeout();
#endif
}
