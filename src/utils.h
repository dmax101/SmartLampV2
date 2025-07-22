#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>

// Classe para gerenciar conexões WiFi
class WiFiManager {
public:
  static bool connectToWiFi();
  static bool isConnected();
  static void showConnectionStatus();
};

// Classe para gerenciar tempo
class TimeManager {
private:
  static int lastMinute;
  
public:
  static void initNTP();
  static bool hasMinuteChanged();
  static void getCurrentTime(struct tm* timeinfo);
};

#endif