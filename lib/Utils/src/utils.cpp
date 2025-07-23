#include "utils.h"
#include <WiFi.h>

// Variáveis estáticas da classe TimeManager
struct tm TimeManager::lastTime = {};
bool TimeManager::timeInitialized = false;

// Implementação WiFiManager
bool WiFiManager::connectToWiFi() {
    Serial.printf("Conectando ao WiFi: %s\n", ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\nWiFi conectado! IP: %s\n", WiFi.localIP().toString().c_str());
        return true;
    } else {
        Serial.println("\nFalha na conexão WiFi!");
        return false;
    }
}

void WiFiManager::reconnect() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Reconectando WiFi...");
        WiFi.disconnect();
        delay(1000);
        connectToWiFi();
    }
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::printStatus() {
    Serial.printf("WiFi Status: %d\n", WiFi.status());
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
    }
}

// Implementação TimeManager
bool TimeManager::initNTP() {
    Serial.println("Inicializando NTP...");
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi não conectado - não é possível sincronizar NTP");
        return false;
    }
    
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    
    // Aguarda sincronização
    int attempts = 0;
    while (!getLocalTime(&lastTime) && attempts < 10) {
        Serial.print(".");
        delay(1000);
        attempts++;
    }
    
    if (attempts >= 10) {
        Serial.println("\nFalha na sincronização NTP!");
        timeInitialized = false;
        return false;
    }
    
    Serial.printf("\nNTP sincronizado: %02d:%02d:%02d %02d/%02d/%04d\n", 
                  lastTime.tm_hour, lastTime.tm_min, lastTime.tm_sec,
                  lastTime.tm_mday, lastTime.tm_mon + 1, lastTime.tm_year + 1900);
    
    timeInitialized = true;
    return true;
}

bool TimeManager::initialize() {
    Serial.println("Inicializando TimeManager...");
    return initNTP();
}

bool TimeManager::initializeTime() {
    return initialize();
}

bool TimeManager::hasMinuteChanged() {
    if (!timeInitialized) {
        return false;
    }
    
    struct tm currentTime;
    if (!getLocalTime(&currentTime)) {
        return false;
    }
    
    // Verifica se o minuto mudou
    bool changed = (currentTime.tm_min != lastTime.tm_min) || 
                   (currentTime.tm_hour != lastTime.tm_hour) ||
                   (currentTime.tm_mday != lastTime.tm_mday);
    
    if (changed) {
        lastTime = currentTime;
        return true;
    }
    
    return false;
}

String TimeManager::getCurrentTimeString() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "--:--"; // Corrigido: removido trigraphs
    }
    
    char timeStr[8];
    sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    return String(timeStr);
}

String TimeManager::getCurrentDateString() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "--/--/----"; // Corrigido: removido trigraphs
    }
    
    char dateStr[12];
    sprintf(dateStr, "%02d/%02d/%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
    return String(dateStr);
}