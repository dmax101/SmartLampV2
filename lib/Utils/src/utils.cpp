#include "utils.h"
#include <config.h>
#include <display.h>
#include <time.h>

// Inicialização de variáveis estáticas
int TimeManager::lastMinute = -1;

// ========== WiFiManager ==========

bool WiFiManager::connectToWiFi() {
    Serial.println("Conectando ao WiFi...");
    showStatusMessage("Conectando WiFi...", ST77XX_YELLOW);
    
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi conectado!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        showStatusMessage("WiFi OK!", ST77XX_GREEN);
        delay(1000);
        return true;
    } else {
        Serial.println("\nFalha na conexão WiFi");
        showStatusMessage("Erro WiFi!", ST77XX_RED);
        return false;
    }
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::showConnectionStatus() {
    if (isConnected()) {
        showStatusMessage("WiFi Conectado", ST77XX_GREEN);
    } else {
        showStatusMessage("WiFi Desconectado", ST77XX_RED);
    }
}

void WiFiManager::reconnect() {
    if (!isConnected()) {
        Serial.println("Tentando reconectar WiFi...");
        WiFi.reconnect();
    }
}

// ========== TimeManager ==========

void TimeManager::initNTP() {
    Serial.println("Configurando NTP...");
    showStatusMessage("Configurando hora...", ST77XX_CYAN);
    
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    
    // Aguarda sincronização
    struct tm timeinfo;
    int attempts = 0;
    while (!getLocalTime(&timeinfo) && attempts < 10) {
        Serial.println("Aguardando sincronização NTP...");
        delay(1000);
        attempts++;
    }
    
    if (getLocalTime(&timeinfo)) {
        Serial.println("NTP sincronizado!");
        showStatusMessage("Hora OK!", ST77XX_GREEN);
        lastMinute = timeinfo.tm_min;
    } else {
        Serial.println("Falha na sincronização NTP");
        showStatusMessage("Erro NTP!", ST77XX_RED);
    }
    
    delay(1000);
}

bool TimeManager::hasMinuteChanged() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return false;
    }
    
    if (timeinfo.tm_min != lastMinute) {
        lastMinute = timeinfo.tm_min;
        return true;
    }
    
    return false;
}

String TimeManager::getCurrentTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "--:--";
    }
    
    char timeStr[10];
    sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    return String(timeStr);
}

String TimeManager::getCurrentDate() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "--/--/----";
    }
    
    char dateStr[12];
    sprintf(dateStr, "%02d/%02d/%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
    return String(dateStr);
}