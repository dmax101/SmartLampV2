#include "wallpaper.h"
#include "display.h"
#include "ImageConverter.h"

bool WallpaperManager::wallpaperLoaded = false;

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
    extern Adafruit_ST7789 lcd;
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
        
        // Progresso a cada 20 linhas para não sobrecarregar o Serial
        if (y % 20 == 0) {
            Serial.printf("Renderizando: %d%%\n", (y * 100) / newHeight);
        }
    }
    
    wallpaperLoaded = true;
    Serial.println("Wallpaper carregado com sucesso!");
}

bool WallpaperManager::initializeWallpaper() {
    Serial.println("Inicializando sistema de wallpaper...");
    
    // Converte e salva BMP se necessário
    if (!SPIFFS.exists("/wallpaper.bmp")) {
        Serial.println("Convertendo imagem Base64 para BMP...");
        if (!ImageConverter::saveConvertedBMP("/img.txt", "/wallpaper.bmp")) {
            Serial.println("Erro na conversão da imagem");
            return false;
        }
    }
    
    return true;
}

bool WallpaperManager::loadWallpaperFromFile(const char* filename) {
    uint8_t* imageData;
    int imageSize;
    
    if (ImageConverter::loadAndConvertImage(filename, &imageData, &imageSize)) {
        displayBMP(imageData, imageSize);
        ImageConverter::cleanup(imageData);
        return true;
    }
    
    return false;
}

void WallpaperManager::displayWallpaperWithOverlay() {
    // Carrega o wallpaper
    if (!loadWallpaperFromFile("/img.txt")) {
        Serial.println("Erro ao carregar wallpaper - usando fundo preto");
        extern Adafruit_ST7789 lcd;
        lcd.fillScreen(ST77XX_BLACK);
    }
    
    // Aguarda um momento para o wallpaper ser exibido
    delay(1000);
}

bool WallpaperManager::isWallpaperLoaded() {
    return wallpaperLoaded;
}