#include "display.h"
#include "config.h"
#include <time.h>

// Inicializa o display
Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);

void initDisplay() {
    Serial.println("Iniciando display...");
    
    // Configura o pino do backlight
    pinMode(LCD_BLK, OUTPUT);
    digitalWrite(LCD_BLK, HIGH);
    
    // Inicializa o display
    lcd.init(135, 240);
    lcd.fillScreen(ST77XX_BLACK);
    Serial.println("Display inicializado");
    
    // Define rotação final
    lcd.setRotation(3);
}

int16_t calcularPosicaoX(const char* texto, uint8_t tamanho, int16_t margemDireita) {
    int16_t x1, y1;
    uint16_t w, h;
    lcd.setTextSize(tamanho);
    lcd.getTextBounds(texto, 0, 0, &x1, &y1, &w, &h);
    return 240 - w - margemDireita;
}

void clearDisplayArea(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t color) {
    lcd.fillRect(x, y, w, h, color);
}

void clearStatusMessages() {
    // Limpa toda a tela
    lcd.fillScreen(ST77XX_BLACK);
}

void desenharIconeUmidade(int16_t x, int16_t y) {
    // Desenha uma gota d'água
    lcd.fillCircle(x + 5, y + 8, 3, ST77XX_BLUE);
    lcd.fillTriangle(x + 5, y + 2, x + 2, y + 8, x + 8, y + 8, ST77XX_BLUE);
    
    // Adiciona brilho na gota
    lcd.drawPixel(x + 4, y + 6, ST77XX_CYAN);
    lcd.drawPixel(x + 3, y + 7, ST77XX_CYAN);
}

void desenharIconeClima(int16_t x, int16_t y, String icone) {
    if (icone == "01d" || icone == "01n") { // Sol
        lcd.fillCircle(x + 15, y + 15, 8, ST77XX_YELLOW);
        // Raios do sol
        for (int i = 0; i < 8; i++) {
            float angle = i * PI / 4;
            int x1 = x + 15 + cos(angle) * 12;
            int y1 = y + 15 + sin(angle) * 12;
            int x2 = x + 15 + cos(angle) * 15;
            int y2 = y + 15 + sin(angle) * 15;
            lcd.drawLine(x1, y1, x2, y2, ST77XX_YELLOW);
        }
    } 
    else if (icone == "02d" || icone == "02n" || icone == "03d" || icone == "03n") { // Nuvens
        lcd.fillCircle(x + 8, y + 18, 6, ST77XX_WHITE);
        lcd.fillCircle(x + 15, y + 15, 8, ST77XX_WHITE);
        lcd.fillCircle(x + 22, y + 18, 6, ST77XX_WHITE);
        lcd.fillRect(x + 8, y + 18, 14, 8, ST77XX_WHITE);
    }
    else if (icone == "04d" || icone == "04n") { // Nuvens escuras
        uint16_t grayColor = 0x39C7; // Cinza escuro
        lcd.fillCircle(x + 8, y + 18, 6, grayColor);
        lcd.fillCircle(x + 15, y + 15, 8, grayColor);
        lcd.fillCircle(x + 22, y + 18, 6, grayColor);
        lcd.fillRect(x + 8, y + 18, 14, 8, grayColor);
    }
    else if (icone.startsWith("09") || icone.startsWith("10")) { // Chuva
        // Nuvem
        lcd.fillCircle(x + 8, y + 15, 5, ST77XX_BLUE);
        lcd.fillCircle(x + 15, y + 12, 7, ST77XX_BLUE);
        lcd.fillCircle(x + 22, y + 15, 5, ST77XX_BLUE);
        lcd.fillRect(x + 8, y + 15, 14, 6, ST77XX_BLUE);
        // Gotas de chuva
        for (int i = 0; i < 5; i++) {
            lcd.drawLine(x + 10 + i * 3, y + 22, x + 8 + i * 3, y + 28, ST77XX_CYAN);
        }
    }
    else if (icone.startsWith("11")) { // Tempestade
        // Nuvem escura
        lcd.fillCircle(x + 8, y + 15, 5, ST77XX_PURPLE);
        lcd.fillCircle(x + 15, y + 12, 7, ST77XX_PURPLE);
        lcd.fillCircle(x + 22, y + 15, 5, ST77XX_PURPLE);
        lcd.fillRect(x + 8, y + 15, 14, 6, ST77XX_PURPLE);
        // Raio
        lcd.drawLine(x + 15, y + 20, x + 12, y + 25, ST77XX_YELLOW);
        lcd.drawLine(x + 12, y + 25, x + 18, y + 30, ST77XX_YELLOW);
    }
    else if (icone.startsWith("13")) { // Neve
        // Nuvem
        lcd.fillCircle(x + 8, y + 15, 5, ST77XX_WHITE);
        lcd.fillCircle(x + 15, y + 12, 7, ST77XX_WHITE);
        lcd.fillCircle(x + 22, y + 15, 5, ST77XX_WHITE);
        lcd.fillRect(x + 8, y + 15, 14, 6, ST77XX_WHITE);
        // Flocos de neve
        for (int i = 0; i < 4; i++) {
            int px = x + 10 + i * 4;
            int py = y + 25 + (i % 2) * 3;
            lcd.drawPixel(px, py, ST77XX_WHITE);
            lcd.drawPixel(px - 1, py, ST77XX_WHITE);
            lcd.drawPixel(px + 1, py, ST77XX_WHITE);
            lcd.drawPixel(px, py - 1, ST77XX_WHITE);
            lcd.drawPixel(px, py + 1, ST77XX_WHITE);
        }
    }
    else { // Padrão - nuvem
        lcd.fillCircle(x + 8, y + 18, 6, ST77XX_WHITE);
        lcd.fillCircle(x + 15, y + 15, 8, ST77XX_WHITE);
        lcd.fillCircle(x + 22, y + 18, 6, ST77XX_WHITE);
        lcd.fillRect(x + 8, y + 18, 14, 8, ST77XX_WHITE);
    }
}

void drawInterfaceElements(const WeatherData& weather) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Falha ao obter o tempo");
        return;
    }
    
    // Prepara strings para exibição (sem segundos)
    char timeStr[8];
    char dateStr[12];
    char tempStr[20];
    char umidadeStr[15];
    
    sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    sprintf(dateStr, "%02d/%02d/%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
    sprintf(tempStr, "%.1f°C", weather.temperatura);
    sprintf(umidadeStr, "%.0f%%", weather.umidade);
    
    // Área da hora com fundo semi-transparente
    int xTime = calcularPosicaoX(timeStr, 3);
    lcd.fillRect(xTime - 5, 0, 240 - xTime + 5, 30, 0x0000); // Fundo preto
    lcd.setTextColor(ST77XX_CYAN);
    lcd.setTextSize(3);
    lcd.setCursor(xTime, 5);
    lcd.print(timeStr);
    
    // Área da data
    int xDate = calcularPosicaoX(dateStr, 2);
    lcd.fillRect(xDate - 5, 30, 240 - xDate + 5, 25, 0x0000);
    lcd.setTextColor(ST77XX_YELLOW);
    lcd.setTextSize(2);
    lcd.setCursor(xDate, 35);
    lcd.print(dateStr);
    
    // Área do clima
    lcd.fillRect(0, 60, 240, 35, 0x0000);
    
    // Ícone do clima
    desenharIconeClima(10, 65, weather.iconeClima);
    
    // Temperatura
    int xTemp = calcularPosicaoX(tempStr, 2);
    lcd.setTextColor(ST77XX_GREEN);
    lcd.setTextSize(2);
    lcd.setCursor(xTemp, 70);
    lcd.print(tempStr);
    
    // Área da umidade
    lcd.fillRect(0, 90, 240, 20, 0x0000);
    
    // Ícone de umidade
    desenharIconeUmidade(10, 95);
    
    // Umidade
    int xUmid = calcularPosicaoX(umidadeStr, 1);
    lcd.setTextColor(ST77XX_BLUE);
    lcd.setTextSize(1);
    lcd.setCursor(xUmid, 95);
    lcd.print(umidadeStr);
    
    // Área da cidade
    lcd.fillRect(0, 105, 240, 15, 0x0000);
    char cidadeStr[] = "Pouso Alegre/MG";
    lcd.setTextColor(ST77XX_WHITE);
    lcd.setTextSize(1);
    int xCidade = calcularPosicaoX(cidadeStr, 1);
    lcd.setCursor(xCidade, 110);
    lcd.print(cidadeStr);
}

void clearArea(int16_t x, int16_t y, uint16_t w, uint16_t h) {
    lcd.fillRect(x, y, w, h, ST77XX_BLACK);
}

void showErrorMessage(const char* message) {
    lcd.setTextColor(ST77XX_RED);
    lcd.setTextSize(1);
    int16_t x = calcularPosicaoX(message, 1);
    lcd.setCursor(x, 50);
    lcd.println(message);
}

void showStatusMessage(const char* message, uint16_t color) {
    lcd.setTextColor(color);
    lcd.setTextSize(1);
    lcd.println(message);
}