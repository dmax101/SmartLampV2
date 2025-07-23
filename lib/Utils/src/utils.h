#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <config.h>

// Classe para gerenciar WiFi
class WiFiManager {
public:
    static bool connectToWiFi();
    static void reconnect();
    static bool isConnected();
    static void printStatus();
};

// Classe para gerenciar tempo/NTP
class TimeManager {
private:
    static struct tm lastTime;
    static bool timeInitialized;
    
public:
    static bool initializeTime();
    static bool initNTP();           // Adicionar método initNTP
    static bool initialize();        // Adicionar método initialize
    static bool hasMinuteChanged();
    static String getCurrentTimeString();
    static String getCurrentDateString();
};

#endif