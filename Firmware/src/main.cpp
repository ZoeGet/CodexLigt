#include <Arduino.h>
#include <cstring>

#include <Preferences.h>

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
Preferences preferences;
String serialBuffer;
unsigned long lastWirelessPacketMs = 0;
String controlToken;
bool pairingMode = false;
unsigned long pairingModeUntilMs = 0;

#if CODEXLIGHT_WIFI_AVAILABLE
WiFiUDP udp;
bool udpStarted = false;
unsigned long lastWifiAttemptMs = 0;
unsigned long lastHelloMs = 0;
#endif

enum class LightState {
  Red,
  Green,
  Yellow,
  Unknown,
};

void saveControlToken(const String& token) {
  preferences.begin("codexlight", false);
  preferences.putString("token", token);
  preferences.end();
  controlToken = token;
}

void loadControlToken() {
  preferences.begin("codexlight", true);
  controlToken = preferences.getString("token", "");
  preferences.end();
}

void clearControlToken() {
  preferences.begin("codexlight", false);
  preferences.remove("token");
  preferences.end();
  controlToken = "";
}

void enterPairingMode(unsigned long durationMs) {
  pairingMode = true;
  pairingModeUntilMs = millis() + durationMs;
  leds.showYellow();
}

void maintainPairingMode() {
  if (pairingMode && static_cast<long>(millis() - pairingModeUntilMs) >= 0) {
    pairingMode = false;
  }
}

String getValueAfter(String command, const String& key) {
  String upperCommand = command;
  String upperKey = key;
  upperCommand.toUpperCase();
  upperKey.toUpperCase();

  const int keyStart = upperCommand.indexOf(upperKey);
  if (keyStart < 0) {
    return "";
  }

  const int valueStart = keyStart + key.length();
  int valueEnd = command.indexOf(' ', valueStart);
  if (valueEnd < 0) {
    valueEnd = command.length();
  }
  return command.substring(valueStart, valueEnd);
}

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

LightState parseAuthenticatedState(String command, bool requireAuth) {
  command.trim();

  String upperCommand = command;
  upperCommand.toUpperCase();

  constexpr const char* prefix = "CODEXLIGHT/1 ";
  if (!upperCommand.startsWith(prefix)) {
    return requireAuth ? LightState::Unknown : parseState(command);
  }

  command = command.substring(strlen(prefix));
  command.trim();
  upperCommand = command;
  upperCommand.toUpperCase();

  if (controlToken.length() == 0) {
    return requireAuth ? LightState::Unknown : parseState(command);
  }

  const String receivedToken = getValueAfter(command, "token=");
  if (receivedToken != controlToken) {
    return LightState::Unknown;
  }

  if (upperCommand.endsWith(" GREEN")) {
    return LightState::Green;
  }
  if (upperCommand.endsWith(" RED")) {
    return LightState::Red;
  }
  if (upperCommand.endsWith(" YELLOW")) {
    return LightState::Yellow;
  }
  return LightState::Unknown;
}

bool handlePairCommand(String command) {
  command.trim();
  String upperCommand = command;
  upperCommand.toUpperCase();

  if (upperCommand == "PAIR") {
    enterPairingMode(60000UL);
    return true;
  }

  if (upperCommand == "CLEAR_TOKEN") {
    clearControlToken();
    enterPairingMode(120000UL);
    return true;
  }

  constexpr const char* pairPrefix = "CODEXLIGHT/1 PAIR_SET ";
  if (!upperCommand.startsWith(pairPrefix)) {
    return false;
  }

  if (!pairingMode && controlToken.length() > 0) {
    return true;
  }

  const String newToken = getValueAfter(command, "token=");
  if (newToken.length() < 16) {
    return true;
  }

  saveControlToken(newToken);
  pairingMode = false;
  leds.showGreen();

#if CODEXLIGHT_WIFI_AVAILABLE
  if (udpStarted) {
    const String response = "CODEXLIGHT/1 PAIR_OK mac=" + WiFi.macAddress();
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.print(response);
    udp.endPacket();
  }
#endif

  return true;
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

void handleCommand(String command, bool requireAuth) {
  if (handlePairCommand(command)) {
    return;
  }

  const LightState state = parseAuthenticatedState(command, requireAuth);
  if (state != LightState::Unknown) {
    applyState(state);
  }
}

void handleSerialInput() {
  while (Serial.available() > 0) {
    const char ch = static_cast<char>(Serial.read());
    if (ch == '\n' || ch == '\r') {
      if (serialBuffer.length() > 0) {
        handleCommand(serialBuffer, false);
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
void sendHello() {
  if (!udpStarted) {
    return;
  }

  const String type = pairingMode ? "PAIR_HELLO" : "HELLO";
  const String message = "CODEXLIGHT/1 " + type + " mac=" + WiFi.macAddress();
  udp.beginPacket(IPAddress(255, 255, 255, 255), UDP_PORT);
  udp.print(message);
  udp.endPacket();
}

void maintainWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!udpStarted) {
      udpStarted = udp.begin(UDP_PORT) == 1;
      if (udpStarted) {
        lastWirelessPacketMs = millis();
        sendHello();
      }
    }

    const unsigned long now = millis();
    const unsigned long helloIntervalMs = pairingMode ? 1000UL : 5000UL;
    if (now - lastHelloMs >= helloIntervalMs) {
      lastHelloMs = now;
      sendHello();
    }
    return;
  }

  udpStarted = false;
  lastHelloMs = 0;
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
  handleCommand(String(packet), true);
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
  loadControlToken();
  leds.showGreen();

  if (controlToken.length() == 0) {
    enterPairingMode(120000UL);
  }

#if CODEXLIGHT_WIFI_AVAILABLE
  WiFi.mode(WIFI_STA);
  WiFi.begin(CODEXLIGHT_WIFI_SSID, CODEXLIGHT_WIFI_PASSWORD);
#endif
}

void loop() {
  maintainPairingMode();
  handleSerialInput();

#if CODEXLIGHT_WIFI_AVAILABLE
  maintainWifi();
  handleUdpInput();
  handleWirelessTimeout();
#endif
}
