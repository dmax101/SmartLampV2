#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Configurações do Display ST7789 240x135
#define LCD_MOSI  23  // ESP32 D23
#define LCD_SCLK  18  // ESP32 D18
#define LCD_CS    15  // ESP32 D15
#define LCD_DC     2  // ESP32 D2
#define LCD_RST    4  // ESP32 D4
#define LCD_BLK   32  // ESP32 D32 (backlight)

// Configurações WiFi
extern const char* ssid;
extern const char* password;

// Configurações de tempo (NTP)
extern const char* ntpServer;
extern const long gmtOffset_sec;
extern const int daylightOffset_sec;

// API OpenWeatherMap
extern const char* apiKey;
extern const char* cidade;
extern const char* pais;
extern String apiURL;

// Intervalos de atualização
extern const unsigned long intervaloAtualizacao;
extern const unsigned long displayUpdateInterval;

// Definições de cores personalizadas (apenas as que não existem na biblioteca)
#ifndef ST77XX_PURPLE
#define ST77XX_PURPLE   0x780F  // RGB565: Purple
#endif

#ifndef ST77XX_PINK
#define ST77XX_PINK     0xF81F  // RGB565: Pink
#endif

#ifndef ST77XX_BROWN
#define ST77XX_BROWN    0x9A60  // RGB565: Brown
#endif

// Não redefinir ST77XX_ORANGE pois já existe na biblioteca

#endif