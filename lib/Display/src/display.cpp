#include "display.h"
#include <config.h>
#include <time.h>
#include "wallpaper.h"  // Adicionar include do wallpaper

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
    // REMOVIDO: Não limpa mais áreas, preserva o wallpaper
    Serial.printf("🚫 clearDisplayArea chamado mas ignorado para preservar wallpaper\n");
}

void clearStatusMessages() {
    // REMOVIDO: Não limpa mais a tela toda, preserva o wallpaper
    Serial.println("🚫 clearStatusMessages chamado mas ignorado para preservar wallpaper");
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
    // Ícones menores e proporcionais à temperatura
    if (icone == "01d" || icone == "01n") { // Sol
        lcd.fillCircle(x + 10, y + 10, 5, ST77XX_YELLOW); // Raio menor
        // Raios do sol
        for (int i = 0; i < 8; i++) {
            float angle = i * PI / 4;
            int x1 = x + 10 + cos(angle) * 8;
            int y1 = y + 10 + sin(angle) * 8;
            int x2 = x + 10 + cos(angle) * 11;
            int y2 = y + 10 + sin(angle) * 11;
            lcd.drawLine(x1, y1, x2, y2, ST77XX_YELLOW);
        }
    } 
    else if (icone == "02d" || icone == "02n" || icone == "03d" || icone == "03n") { // Nuvens
        lcd.fillCircle(x + 5, y + 13, 4, ST77XX_WHITE);
        lcd.fillCircle(x + 10, y + 10, 5, ST77XX_WHITE);
        lcd.fillCircle(x + 15, y + 13, 4, ST77XX_WHITE);
        lcd.fillRect(x + 5, y + 13, 10, 6, ST77XX_WHITE);
    }
    else { // Padrão - nuvem
        lcd.fillCircle(x + 5, y + 13, 4, ST77XX_WHITE);
        lcd.fillCircle(x + 10, y + 10, 5, ST77XX_WHITE);
        lcd.fillCircle(x + 15, y + 13, 4, ST77XX_WHITE);
        lcd.fillRect(x + 5, y + 13, 10, 6, ST77XX_WHITE);
    }
}

void drawInterfaceElements(const WeatherData& weather) {
    Serial.println("🎨 === REDESENHANDO INTERFACE COMPLETA ===");

    // Variáveis para cálculo de texto
    int16_t x1, y1;
    uint16_t w, h;

    animacaoChuvaLimpeza();

    // Redesenha o wallpaper
    Serial.println("🖼️ Redesenhando wallpaper...");
    if (!WallpaperManager::loadWallpaperFromFile()) {
        Serial.println("❌ Falha ao carregar wallpaper - usando fundo preto");
        lcd.fillScreen(ST77XX_BLACK);
    }

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("❌ Falha ao obter o tempo");
        return;
    }

    // Prepara strings para exibição
    char timeStr[8];
    char dateStr[12];
    char tempStr[20];
    char umidadeStr[15];

    sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    sprintf(dateStr, "%02d/%02d/%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
    sprintf(tempStr, "%.1f %cC", weather.temperatura, 0xF7);
    sprintf(umidadeStr, "%.0f%%", weather.umidade);

    Serial.printf("📊 Dados: %s | %s | %s | %s\n", timeStr, dateStr, tempStr, umidadeStr);

    // DESENHA HORA
    int xTime = calcularPosicaoX(timeStr, 3);
    lcd.setTextColor(ST77XX_CYAN);
    lcd.setTextSize(3);
    lcd.setCursor(xTime, 5);
    lcd.print(timeStr);
    Serial.printf("⏰ Hora desenhada em x=%d\n", xTime);

    // DESENHA DATA
    int xDate = calcularPosicaoX(dateStr, 2);
    lcd.setTextColor(ST77XX_YELLOW);
    lcd.setTextSize(2);
    lcd.setCursor(xDate, 35);
    lcd.print(dateStr);
    Serial.printf("📅 Data desenhada em x=%d\n", xDate);

    // ALINHAMENTO À DIREITA PARA TEMPERATURA E UMIDADE
    // Temperatura alinhada à direita na metade superior
    lcd.setTextSize(2);
    lcd.getTextBounds(tempStr, 0, 0, &x1, &y1, &w, &h);
    int xTemp = 240 - w - 10; // 10px de margem direita
    int yTemp = 80;           // Aproxima da umidade (antes era 70)
    lcd.setTextColor(ST77XX_GREEN);
    lcd.setCursor(xTemp, yTemp);
    lcd.print(tempStr);
    Serial.printf("🌡️ Temperatura desenhada em x=%d\n", xTemp);

    // Ícone do clima à direita da temperatura, com espaço extra
    int iconClimaX = xTemp - 28;
    desenharIconeClima(iconClimaX, yTemp - 5, weather.iconeClima);
    Serial.printf("🌤️ Ícone clima desenhado em x=%d\n", iconClimaX);

    // Umidade alinhada à direita na metade inferior
    lcd.setTextSize(2);
    lcd.getTextBounds(umidadeStr, 0, 0, &x1, &y1, &w, &h);
    int xUmid = 240 - w - 10;
    int yUmid = 105;          // Mantém posição da umidade
    lcd.setTextColor(ST77XX_BLUE);
    lcd.setCursor(xUmid, yUmid);
    lcd.print(umidadeStr);
    Serial.printf("💧 Umidade desenhada em x=%d\n", xUmid);

    // Ícone de umidade à direita da umidade, com espaço extra
    int iconUmidX = xUmid - 20;
    desenharIconeUmidade(iconUmidX, yUmid);
    Serial.printf("💧 Ícone umidade desenhado em x=%d\n", iconUmidX);

    // DESENHA CIDADE
    char cidadeStr[] = "Pouso Alegre/MG";
    lcd.setTextColor(ST77XX_WHITE);
    lcd.setTextSize(1);
    int xCidade = calcularPosicaoX(cidadeStr, 1);
    lcd.setCursor(xCidade, 125);
    lcd.print(cidadeStr);
    Serial.printf("🏙️ Cidade desenhada em x=%d\n", xCidade);

    Serial.println("✅ Interface completa redesenhada!");
}

void clearArea(int16_t x, int16_t y, uint16_t w, uint16_t h) {
    // REMOVIDO: Não limpa mais áreas, preserva o wallpaper
    Serial.printf("🚫 clearArea chamado mas ignorado para preservar wallpaper\n");
}

void showErrorMessage(const char* message) {
    // Desenha mensagem de erro SEM fundo preto
    lcd.setTextColor(ST77XX_RED);
    lcd.setTextSize(1);
    lcd.setCursor(10, 60);
    lcd.println(message);
}

void showStatusMessage(const char* message, uint16_t color) {
    // Limpa área do texto antes de mostrar a mensagem
    lcd.fillRect(0, 55, 240, 20, ST77XX_BLACK); // Ajuste altura/largura conforme necessário
    lcd.setTextColor(color);
    lcd.setTextSize(1);
    lcd.setCursor(10, 60);
    lcd.println(message);
}

void animacaoChuvaLimpeza() {
    const int numPingos = 30; // Quantidade de ondas
    int pingoX[numPingos];
    int pingoY[numPingos];

    // Inicializa posições X e Y aleatórias para os pingos
    for (int i = 0; i < numPingos; i++) {
        pingoX[i] = random(10, 230);
        pingoY[i] = random(10, 125);
    }

    // Animação: ondas circulares crescendo pela tela
    for (int frame = 0; frame < 20; frame++) {
        // Não limpa o fundo, desenha sobre o conteúdo anterior
        for (int i = 0; i < numPingos; i++) {
            int raio = 2 + frame * 7;
            lcd.drawCircle(pingoX[i], pingoY[i], raio, ST77XX_BLACK);
            if (frame > 4) {
                lcd.drawCircle(pingoX[i], pingoY[i], raio - 4, ST77XX_BLACK);
            }
        }
        delay(35);
    }
}