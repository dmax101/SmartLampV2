#include "utils.h"
#include "config.h"
#include "display.h"
#include <WiFi.h>
#include <time.h>

int TimeManager::lastMinute = -1;

bool WiFiManager::connectToWiFi() {
  Serial.println("Conectando ao WiFi...");
  
  WiFi.begin(ssid, password);
  showStatusMessage("Conectando WiFi...", ST77XX_YELLOW);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    showStatusMessage("WiFi OK!", ST77XX_GREEN);
    return true;
  } else {
    Serial.println("Falha na conexão WiFi!");
    showErrorMessage("WiFi ERRO!");
    return false;
  }
}

bool WiFiManager::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::showConnectionStatus() {
  if (!isConnected()) {
    showErrorMessage("WiFi desconectado!");
  }
}

void TimeManager::initNTP() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Aguardando sincronização do tempo...");
  delay(3000);
}

bool TimeManager::hasMinuteChanged() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return false;
  }
  
  bool changed = (timeinfo.tm_min != lastMinute);
  if (changed) {
    lastMinute = timeinfo.tm_min;
    Serial.printf("Minuto mudou para: %02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min);
  }
  
  return changed;
}

void TimeManager::getCurrentTime(struct tm* timeinfo) {
  getLocalTime(timeinfo);
}