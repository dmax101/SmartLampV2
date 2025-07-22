#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "config.h"

// Declaração do objeto display
extern Adafruit_ST7789 lcd;

// Estrutura para dados do clima
struct WeatherData {
    float temperatura;
    float umidade;
    String descricaoClima;
    String iconeClima;
};

// Funções do display
void initDisplay();
void drawInterfaceElements(const WeatherData& weather);
void desenharIconeClima(int16_t x, int16_t y, String icone);
void desenharIconeUmidade(int16_t x, int16_t y);
void clearArea(int16_t x, int16_t y, uint16_t w, uint16_t h);
void clearDisplayArea(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t color = ST77XX_BLACK);
void clearStatusMessages();
int16_t calcularPosicaoX(const char* texto, uint8_t tamanho, int16_t margemDireita = 10);
void showErrorMessage(const char* message);
void showStatusMessage(const char* message, uint16_t color);

#endif