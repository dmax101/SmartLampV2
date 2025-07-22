#include "wallpaper.h"
#include "display.h"
#include <SPIFFS.h>

bool WallpaperManager::wallpaperLoaded = false;

int base64_decode(const char* input, uint8_t* output, int outputLen) {
    const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int inputLen = strlen(input);
    int outputIndex = 0;
    
    for (int i = 0; i < inputLen; i += 4) {
        if (outputIndex >= outputLen - 3) break;
        
        uint32_t value = 0;
        for (int j = 0; j < 4; j++) {
            if (i + j < inputLen) {
                char c = input[i + j];
                if (c == '=') break;
                
                const char* pos = strchr(chars, c);
                if (pos) {
                    value = (value << 6) | (pos - chars);
                }
            }
        }
        
        output[outputIndex++] = (value >> 16) & 0xFF;
        output[outputIndex++] = (value >> 8) & 0xFF;
        output[outputIndex++] = value & 0xFF;
    }
    
    return outputIndex;
}

void WallpaperManager::displayBMP(uint8_t* bmpData, int dataSize) {
    // Verifica se é um arquivo BMP válido
    if (dataSize < 54 || bmpData[0] != 'B' || bmpData[1] != 'M') {
        Serial.println("Arquivo BMP inválido");
        return;
    }
    
    // Lê header BMP
    uint32_t imageOffset = *(uint32_t*)(bmpData + 10);
    uint32_t width = *(uint32_t*)(bmpData + 18);
    uint32_t height = *(uint32_t*)(bmpData + 22);
    uint16_t bitsPerPixel = *(uint16_t*)(bmpData + 28);
    
    Serial.printf("BMP Info: %dx%d, %d bits\n", width, height, bitsPerPixel);
    
    // Suporte apenas para 24-bit BMP
    if (bitsPerPixel != 24) {
        Serial.println("Suporte apenas para BMP 24-bit");
        return;
    }
    
    // Redimensiona para caber no display (240x135)
    float scaleX = (float)240 / width;
    float scaleY = (float)135 / height;
    float scale = min(scaleX, scaleY);
    
    int newWidth = (int)(width * scale);
    int newHeight = (int)(height * scale);
    
    // Centraliza a imagem
    int offsetX = (240 - newWidth) / 2;
    int offsetY = (135 - newHeight) / 2;
    
    // Limpa o display
    lcd.fillScreen(ST77XX_BLACK);
    
    Serial.println("Renderizando wallpaper...");
    
    // Desenha a imagem pixel por pixel
    for (int y = 0; y < newHeight; y++) {
        for (int x = 0; x < newWidth; x++) {
            int origX = (int)(x / scale);
            int origY = (int)(y / scale);
            
            // BMP é armazenado de baixo para cima
            int bmpY = height - 1 - origY;
            
            // Calcula posição no buffer (3 bytes por pixel)
            int rowSize = ((width * 3 + 3) & ~3);
            int pixelOffset = imageOffset + (bmpY * rowSize) + (origX * 3);
            
            if (pixelOffset + 2 < dataSize) {
                // Lê pixels BGR (BMP format)
                uint8_t b = bmpData[pixelOffset];
                uint8_t g = bmpData[pixelOffset + 1];
                uint8_t r = bmpData[pixelOffset + 2];
                
                // Converte para RGB565
                uint16_t color = lcd.color565(r, g, b);
                
                // Desenha pixel no display
                lcd.drawPixel(offsetX + x, offsetY + y, color);
            }
        }
        
        // Progresso a cada 20 linhas
        if (y % 20 == 0) {
            Serial.printf("Renderizando: %d%%\n", (y * 100) / newHeight);
        }
    }
    
    wallpaperLoaded = true;
    Serial.println("Wallpaper carregado com sucesso!");
}

bool WallpaperManager::initializeWallpaper() {
    Serial.println("Inicializando sistema de wallpaper...");
    return true;
}

bool WallpaperManager::loadWallpaperFromFile(const char* filename) {
    if (!SPIFFS.begin()) {
        Serial.println("Erro ao inicializar SPIFFS");
        return false;
    }
    
    File file = SPIFFS.open(filename, "r");
    if (!file) {
        Serial.printf("Erro ao abrir arquivo %s\n", filename);
        return false;
    }
    
    String base64Data = file.readString();
    file.close();
    
    // Remove o prefixo "data:image/bmp;base64," se presente
    int commaIndex = base64Data.indexOf(',');
    if (commaIndex != -1) {
        base64Data = base64Data.substring(commaIndex + 1);
    }
    
    // Remove quebras de linha e espaços
    base64Data.replace("\n", "");
    base64Data.replace("\r", "");
    base64Data.replace(" ", "");
    
    Serial.println("Decodificando imagem...");
    
    // Buffer para dados decodificados
    const int bufferSize = 8192;
    uint8_t* imageBuffer = (uint8_t*)malloc(bufferSize);
    if (!imageBuffer) {
        Serial.println("Erro ao alocar memória");
        return false;
    }
    
    int decodedSize = base64_decode(base64Data.c_str(), imageBuffer, bufferSize);
    
    if (decodedSize > 0) {
        displayBMP(imageBuffer, decodedSize);
    }
    
    free(imageBuffer);
    return decodedSize > 0;
}

void WallpaperManager::displayWallpaperWithOverlay() {
    // Primeiro desenha o papel de parede
    if (!loadWallpaperFromFile("/img.txt")) {
        Serial.println("Erro ao carregar wallpaper - usando fundo preto");
        lcd.fillScreen(ST77XX_BLACK);
    }
    
    // Aguarda um momento para o wallpaper ser exibido
    delay(1000);
}

bool WallpaperManager::isWallpaperLoaded() {
    return wallpaperLoaded;
}