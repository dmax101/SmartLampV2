#include "config.h"

// Configurações WiFi
const char* ssid = "VIVOFIBRA-DD21";
const char* password = "hswVP74823";

// Configurações de tempo (NTP)
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -3 * 3600; // GMT-3 (Brasil)
const int daylightOffset_sec = 0;

// API OpenWeatherMap
const char* apiKey = "da1bf1f33a51a2bfa3ce1745bf65fd7f";
const char* cidade = "Pouso%20Alegre";
const char* pais = "BR";
String apiURL = "http://api.openweathermap.org/data/2.5/weather?q=Pouso%20Alegre,BR&appid=" + String(apiKey) + "&units=metric&lang=pt";

// Intervalos de atualização
const unsigned long intervaloAtualizacao = 600000; // 10 minutos
const unsigned long displayUpdateInterval = 1000; // 1 segundo