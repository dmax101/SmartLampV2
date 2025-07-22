#include "wallpaper.h"
#include <display.h>
#include <SPIFFS.h>

bool WallpaperManager::wallpaperLoaded = false;

// Função de debug para verificar arquivo
void debugFile(const char* filename) {
    if (!SPIFFS.begin()) {
        Serial.println("SPIFFS não inicializado");
        return;
    }
    
    File file = SPIFFS.open(filename, "r");
    if (!file) {
        Serial.printf("Arquivo %s não encontrado\n", filename);
        
        // Lista arquivos disponíveis
        File root = SPIFFS.open("/");
        File f = root.openNextFile();
        Serial.println("Arquivos disponíveis:");
        while (f) {
            Serial.printf("  %s (%d bytes)\n", f.name(), f.size());
            f = root.openNextFile();
        }
        return;
    }
    
    Serial.printf("Arquivo %s encontrado: %d bytes\n", filename, file.size());
    
    // Lê primeiros 100 caracteres para debug
    String preview = file.readString();
    file.close();
    
    Serial.println("Primeiros 100 caracteres:");
    Serial.println(preview.substring(0, 100));
    Serial.printf("Total de caracteres: %d\n", preview.length());
}

// Função simples de decodificação Base64
static int base64_decode_simple(const char* input, uint8_t* output, int outputLen) {
    const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int inputLen = strlen(input);
    int outputIndex = 0;
    
    for (int i = 0; i < inputLen && outputIndex < outputLen - 3; i += 4) {
        uint32_t value = 0;
        int padding = 0;
        
        // Processa 4 caracteres por vez
        for (int j = 0; j < 4; j++) {
            if (i + j < inputLen) {
                char c = input[i + j];
                if (c == '=') {
                    padding++;
                    break;
                }
                
                const char* pos = strchr(chars, c);
                if (pos) {
                    value = (value << 6) | (pos - chars);
                }
            }
        }
        
        // Converte para bytes
        if (padding < 3) output[outputIndex++] = (value >> 16) & 0xFF;
        if (padding < 2) output[outputIndex++] = (value >> 8) & 0xFF;
        if (padding < 1) output[outputIndex++] = value & 0xFF;
    }
    
    return outputIndex;
}

// Desenha uma imagem de teste colorida
void drawTestImage() {
    Serial.println("Desenhando imagem de teste...");
    
    // Limpa display
    lcd.fillScreen(ST77XX_BLACK);
    
    // Desenha gradiente colorido
    for (int y = 0; y < 135; y++) {
        for (int x = 0; x < 240; x++) {
            uint16_t color;
            
            if (x < 80) {
                // Vermelho para amarelo
                uint8_t green = (x * 255) / 80;
                color = lcd.color565(255, green, 0);
            } else if (x < 160) {
                // Amarelo para verde
                uint8_t red = 255 - ((x - 80) * 255) / 80;
                color = lcd.color565(red, 255, 0);
            } else {
                // Verde para azul
                uint8_t green = 255 - ((x - 160) * 255) / 80;
                uint8_t blue = ((x - 160) * 255) / 80;
                color = lcd.color565(0, green, blue);
            }
            
            lcd.drawPixel(x, y, color);
        }
        
        // Progresso
        if (y % 20 == 0) {
            Serial.printf("Teste: %d%%\n", (y * 100) / 135);
        }
    }
    
    Serial.println("Imagem de teste concluída");
}

void WallpaperManager::displayBMP(uint8_t* bmpData, int dataSize) {
    Serial.printf("Processando BMP: %d bytes\n", dataSize);
    
    // Verifica se é um arquivo BMP válido
    if (dataSize < 54) {
        Serial.println("Arquivo muito pequeno para ser BMP");
        return;
    }
    
    if (bmpData[0] != 'B' || bmpData[1] != 'M') {
        Serial.printf("Header inválido: %c%c (esperado: BM)\n", bmpData[0], bmpData[1]);
        return;
    }
    
    // Lê header BMP (formato little-endian)
    uint32_t fileSize = *(uint32_t*)(bmpData + 2);
    uint32_t imageOffset = *(uint32_t*)(bmpData + 10);
    uint32_t width = *(uint32_t*)(bmpData + 18);
    uint32_t height = *(uint32_t*)(bmpData + 22);
    uint16_t bitsPerPixel = *(uint16_t*)(bmpData + 28);
    
    Serial.printf("BMP Info:\n");
    Serial.printf("  Tamanho do arquivo: %d bytes\n", fileSize);
    Serial.printf("  Offset da imagem: %d\n", imageOffset);
    Serial.printf("  Dimensões: %dx%d\n", width, height);
    Serial.printf("  Bits por pixel: %d\n", bitsPerPixel);
    
    // Suporte apenas para 24-bit BMP
    if (bitsPerPixel != 24) {
        Serial.printf("BMP de %d bits não suportado (apenas 24-bit)\n", bitsPerPixel);
        return;
    }
    
    if (imageOffset >= dataSize) {
        Serial.println("Offset da imagem inválido");
        return;
    }
    
    // Calcula escala para caber no display (240x135)
    float scaleX = (float)240 / width;
    float scaleY = (float)135 / height;
    float scale = min(scaleX, scaleY);
    
    int newWidth = (int)(width * scale);
    int newHeight = (int)(height * scale);
    
    // Centraliza a imagem
    int offsetX = (240 - newWidth) / 2;
    int offsetY = (135 - newHeight) / 2;
    
    Serial.printf("Escala: %.2f, Nova dimensão: %dx%d, Offset: %d,%d\n", 
                  scale, newWidth, newHeight, offsetX, offsetY);
    
    // Limpa o display
    lcd.fillScreen(ST77XX_BLACK);
    
    Serial.println("Renderizando wallpaper...");
    
    // Desenha a imagem
    for (int y = 0; y < newHeight; y++) {
        for (int x = 0; x < newWidth; x++) {
            // Calcula posição original
            int origX = (int)(x / scale);
            int origY = (int)(y / scale);
            
            // BMP é armazenado de baixo para cima
            int bmpY = height - 1 - origY;
            
            // Calcula posição no buffer (3 bytes por pixel, alinhado a 4 bytes)
            int rowSize = ((width * 3 + 3) & ~3);
            int pixelOffset = imageOffset + (bmpY * rowSize) + (origX * 3);
            
            if (pixelOffset + 2 < dataSize) {
                // Lê pixels BGR (formato BMP)
                uint8_t b = bmpData[pixelOffset];
                uint8_t g = bmpData[pixelOffset + 1];
                uint8_t r = bmpData[pixelOffset + 2];
                
                // Converte para RGB565
                uint16_t color = lcd.color565(r, g, b);
                
                // Desenha pixel no display
                lcd.drawPixel(offsetX + x, offsetY + y, color);
            }
        }
        
        // Progresso a cada 10 linhas
        if (y % 10 == 0) {
            Serial.printf("Renderizando: %d%%\n", (y * 100) / newHeight);
        }
    }
    
    wallpaperLoaded = true;
    Serial.println("Wallpaper carregado com sucesso!");
}

bool WallpaperManager::initializeWallpaper() {
    Serial.println("=== Inicializando sistema de wallpaper ===");
    
    // Verifica se SPIFFS está funcionando
    if (!SPIFFS.begin()) {
        Serial.println("ERRO: Não foi possível inicializar SPIFFS");
        return false;
    }
    
    Serial.println("SPIFFS inicializado com sucesso");
    
    // Debug do arquivo
    debugFile("/img.txt");
    
    return true;
}

bool WallpaperManager::loadWallpaperFromFile(const char* filename) {
    Serial.printf("=== Carregando wallpaper de %s ===\n", filename);
    
    if (!SPIFFS.begin()) {
        Serial.println("ERRO: SPIFFS não inicializado");
        return false;
    }
    
    File file = SPIFFS.open(filename, "r");
    if (!file) {
        Serial.printf("ERRO: Não foi possível abrir %s\n", filename);
        
        // Desenha imagem de teste em caso de erro
        Serial.println("Desenhando imagem de teste...");
        drawTestImage();
        return false;
    }
    
    String base64Data = file.readString();
    file.close();
    
    Serial.printf("Arquivo lido: %d caracteres\n", base64Data.length());
    
    if (base64Data.length() < 100) {
        Serial.println("ERRO: Arquivo muito pequeno");
        drawTestImage();
        return false;
    }
    
    // Remove prefixo se presente
    int commaIndex = base64Data.indexOf(',');
    if (commaIndex != -1) {
        Serial.printf("Removendo prefixo até posição %d\n", commaIndex);
        base64Data = base64Data.substring(commaIndex + 1);
    }
    
    // Remove caracteres inválidos
    base64Data.replace("\n", "");
    base64Data.replace("\r", "");
    base64Data.replace(" ", "");
    base64Data.replace("\t", "");
    
    Serial.printf("Base64 limpo: %d caracteres\n", base64Data.length());
    
    // Aloca buffer para decodificação
    int estimatedSize = (base64Data.length() * 3) / 4 + 100; // +100 para margem
    uint8_t* imageBuffer = (uint8_t*)malloc(estimatedSize);
    
    if (!imageBuffer) {
        Serial.printf("ERRO: Não foi possível alocar %d bytes\n", estimatedSize);
        drawTestImage();
        return false;
    }
    
    Serial.printf("Buffer alocado: %d bytes\n", estimatedSize);
    
    // Decodifica Base64
    int decodedSize = base64_decode_simple(base64Data.c_str(), imageBuffer, estimatedSize);
    
    Serial.printf("Decodificado: %d bytes\n", decodedSize);
    
    if (decodedSize > 54) {
        Serial.println("Tentando exibir BMP...");
        displayBMP(imageBuffer, decodedSize);
    } else {
        Serial.println("ERRO: Decodificação falhou, usando imagem de teste");
        drawTestImage();
    }
    
    free(imageBuffer);
    return decodedSize > 54;
}

void WallpaperManager::displayWallpaperWithOverlay() {
    Serial.println("=== Exibindo wallpaper com overlay ===");
    
    // Primeiro tenta carregar o wallpaper
    if (!loadWallpaperFromFile("/img.txt")) {
        Serial.println("Falha ao carregar wallpaper");
    }
    
    // Aguarda um momento
    delay(2000);
}

bool WallpaperManager::isWallpaperLoaded() {
    return wallpaperLoaded;
}