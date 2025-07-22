#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <WiFi.h>

// Classe para gerenciar WiFi
class WiFiManager {
public:
    static bool connectToWiFi();
    static bool isConnected();
    static void showConnectionStatus();
    static void reconnect();
};

// Classe para gerenciar tempo/NTP
class TimeManager {
private:
    static int lastMinute;
    
public:
    static void initNTP();
    static bool hasMinuteChanged();
    static String getCurrentTime();
    static String getCurrentDate();
};

#endif