#include "wallpaper.h"
#include <display.h>
#include <SPIFFS.h>

bool WallpaperManager::wallpaperLoaded = false;

// Função COMPLETA de debug do SPIFFS
void debugSPIFFS() {
    Serial.println("=== DEBUG COMPLETO SPIFFS ===");
    
    // Tenta diferentes métodos de inicialização
    if (!SPIFFS.begin()) {
        Serial.println("Primeira tentativa SPIFFS falhou, tentando com format...");
        if (!SPIFFS.begin(true)) {
            Serial.println("ERRO CRÍTICO: SPIFFS não pode ser inicializado!");
            return;
        }
    }
    
    Serial.println("✓ SPIFFS inicializado com sucesso");
    
    // Informações do sistema de arquivos
    size_t totalBytes = SPIFFS.totalBytes();
    size_t usedBytes = SPIFFS.usedBytes();
    
    Serial.printf("SPIFFS Total: %zu bytes (%.2f KB)\n", totalBytes, totalBytes/1024.0);
    Serial.printf("SPIFFS Usado: %zu bytes (%.2f KB)\n", usedBytes, usedBytes/1024.0);
    Serial.printf("SPIFFS Livre: %zu bytes (%.2f KB)\n", totalBytes - usedBytes, (totalBytes - usedBytes)/1024.0);
    
    // Lista TODOS os arquivos
    File root = SPIFFS.open("/");
    if (!root) {
        Serial.println("ERRO: Não foi possível abrir diretório raiz");
        return;
    }
    
    Serial.println("\n=== LISTAGEM COMPLETA DE ARQUIVOS ===");
    File file = root.openNextFile();
    int fileCount = 0;
    
    while (file) {
        fileCount++;
        Serial.printf("%d. Nome: '%s'\n", fileCount, file.name());
        Serial.printf("   Path: '%s'\n", file.path());
        Serial.printf("   Tamanho: %d bytes\n", file.size());
        Serial.printf("   É diretório: %s\n", file.isDirectory() ? "Sim" : "Não");
        
        file = root.openNextFile();
    }
    
    if (fileCount == 0) {
        Serial.println("❌ NENHUM ARQUIVO ENCONTRADO!");
        Serial.println("💡 ISSO SIGNIFICA QUE O UPLOAD DO FILESYSTEM NÃO FOI FEITO!");
        Serial.println("💡 Execute: PlatformIO -> Upload Filesystem Image");
    } else {
        Serial.printf("✓ Total de %d arquivo(s) encontrado(s)\n", fileCount);
    }
    
    root.close();
    Serial.println("=== FIM DEBUG SPIFFS ===\n");
}

// Cria arquivo de teste no SPIFFS
void createTestFile() {
    Serial.println("=== CRIANDO ARQUIVO DE TESTE ===");
    
    // Dados Base64 de um BMP 1x1 pixel vermelho
    String testBMP = "Qk0+AAAAAAAAADYAAAAoAAAAAQAAAAEAAAABACAAAAAAAAgAAAASCwAAEgsAAAAAAAAAAAAA/wAA";
    
    File file = SPIFFS.open("/test.txt", "w");
    if (file) {
        file.print("data:image/bmp;base64,");
        file.print(testBMP);
        file.close();
        Serial.println("✓ Arquivo de teste criado: /test.txt");
    } else {
        Serial.println("❌ Erro ao criar arquivo de teste");
    }
}

// Função simples de decodificação Base64
static int base64_decode_simple(const char* input, uint8_t* output, int outputLen) {
    const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int inputLen = strlen(input);
    int outputIndex = 0;
    
    for (int i = 0; i < inputLen && outputIndex < outputLen - 3; i += 4) {
        uint32_t value = 0;
        int padding = 0;
        
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
        
        if (padding < 3) output[outputIndex++] = (value >> 16) & 0xFF;
        if (padding < 2) output[outputIndex++] = (value >> 8) & 0xFF;
        if (padding < 1) output[outputIndex++] = value & 0xFF;
    }
    
    return outputIndex;
}

// Desenha uma imagem de teste colorida
void drawTestImage() {
    Serial.println("🎨 Desenhando imagem de teste colorida...");
    
    lcd.fillScreen(ST77XX_BLACK);
    
    // Desenha retângulos coloridos
    lcd.fillRect(0, 0, 80, 135, ST77XX_RED);
    lcd.fillRect(80, 0, 80, 135, ST77XX_GREEN);
    lcd.fillRect(160, 0, 80, 135, ST77XX_BLUE);
    
    // Adiciona texto
    lcd.setTextColor(ST77XX_WHITE);
    lcd.setTextSize(1);
    lcd.setCursor(10, 10);
    lcd.print("IMG.TXT NAO ENCONTRADO");
    
    lcd.setCursor(10, 25);
    lcd.print("Faca upload filesystem!");
    
    lcd.setCursor(10, 40);
    lcd.print("PlatformIO -> Upload");
    
    lcd.setCursor(10, 55);
    lcd.print("Filesystem Image");
    
    Serial.println("✓ Imagem de teste concluída");
}

bool WallpaperManager::loadBMPFile(const char* filename) {
    Serial.printf("\n📂 === CARREGANDO BMP: %s ===\n", filename);
    
    if (!SPIFFS.exists(filename)) {
        Serial.printf("❌ Arquivo não encontrado: %s\n", filename);
        return false;
    }
    
    File file = SPIFFS.open(filename, "r");
    if (!file) {
        Serial.printf("❌ Erro ao abrir arquivo: %s\n", filename);
        return false;
    }
    
    size_t fileSize = file.size();
    Serial.printf("✓ Arquivo encontrado, tamanho: %zu bytes\n", fileSize);
    
    if (fileSize < 54) {
        Serial.println("❌ Arquivo muito pequeno para BMP");
        file.close();
        return false;
    }
    
    // Aloca buffer para o arquivo
    uint8_t* bmpData = (uint8_t*)malloc(fileSize);
    if (!bmpData) {
        Serial.printf("❌ Erro ao alocar %zu bytes\n", fileSize);
        file.close();
        return false;
    }
    
    // Lê arquivo completo
    size_t bytesRead = file.readBytes((char*)bmpData, fileSize);
    file.close();
    
    if (bytesRead != fileSize) {
        Serial.printf("❌ Erro na leitura: lido %zu de %zu bytes\n", bytesRead, fileSize);
        free(bmpData);
        return false;
    }
    
    Serial.printf("✓ Arquivo lido com sucesso: %zu bytes\n", bytesRead);
    
    // Processa o BMP
    bool success = processBMP(bmpData, fileSize);
    free(bmpData);
    
    return success;
}

bool WallpaperManager::processBMP(uint8_t* bmpData, size_t dataSize) {
    Serial.printf("=== PROCESSANDO BMP: %zu bytes ===\n", dataSize);
    
    if (dataSize < 54) {
        Serial.println("❌ Arquivo muito pequeno para BMP");
        return false;
    }
    
    // Verifica assinatura BMP
    if (bmpData[0] != 'B' || bmpData[1] != 'M') {
        Serial.printf("❌ Header inválido: %c%c (esperado: BM)\n", bmpData[0], bmpData[1]);
        return false;
    }
    
    // Lê header BMP
    uint32_t fileSize = *(uint32_t*)(bmpData + 2);
    uint32_t imageOffset = *(uint32_t*)(bmpData + 10);
    uint32_t width = *(uint32_t*)(bmpData + 18);
    uint32_t height = *(uint32_t*)(bmpData + 22);
    uint16_t bitsPerPixel = *(uint16_t*)(bmpData + 28);
    uint32_t compression = *(uint32_t*)(bmpData + 30);
    
    Serial.printf("📊 BMP Info:\n");
    Serial.printf("   Tamanho: %u bytes\n", fileSize);
    Serial.printf("   Offset: %u\n", imageOffset);
    Serial.printf("   Dimensões: %ux%u\n", width, height);
    Serial.printf("   Bits/pixel: %u\n", bitsPerPixel);
    Serial.printf("   Compressão: %u\n", compression);
    
    // Valida formato
    if (bitsPerPixel != 24) {
        Serial.printf("❌ BMP de %u bits não suportado (apenas 24-bit)\n", bitsPerPixel);
        return false;
    }
    
    if (compression != 0) {
        Serial.println("❌ BMP comprimido não suportado");
        return false;
    }
    
    if (imageOffset >= dataSize) {
        Serial.println("❌ Offset de dados inválido");
        return false;
    }
    
    Serial.println("✓ BMP válido! Renderizando...");
    
    // Renderiza a imagem
    renderBMPToDisplay(bmpData + imageOffset, width, height);
    
    wallpaperLoaded = true;
    Serial.println("🎉 BMP carregado com sucesso!");
    
    return true;
}

void WallpaperManager::renderBMPToDisplay(uint8_t* imageData, uint32_t width, uint32_t height) {
    Serial.printf("🎨 Renderizando BMP %ux%u no display...\n", width, height);
    
    lcd.fillScreen(ST77XX_BLACK);
    
    // Calcula escala e posicionamento para centralizar
    float scaleX = (float)240 / width;
    float scaleY = (float)135 / height;
    float scale = min(scaleX, scaleY);
    
    uint16_t displayWidth = (uint16_t)(width * scale);
    uint16_t displayHeight = (uint16_t)(height * scale);
    uint16_t offsetX = (240 - displayWidth) / 2;
    uint16_t offsetY = (135 - displayHeight) / 2;
    
    Serial.printf("   Escala: %.2f, Display: %ux%u, Offset: %u,%u\n", 
                  scale, displayWidth, displayHeight, offsetX, offsetY);
    
    // BMP está armazenado de baixo para cima, então invertemos
    uint32_t rowSize = ((width * 3 + 3) / 4) * 4; // Row padding to 4-byte boundary
    
    for (uint16_t y = 0; y < displayHeight; y++) {
        for (uint16_t x = 0; x < displayWidth; x++) {
            // Mapeia coordenadas do display para coordenadas da imagem
            uint32_t imgX = (uint32_t)(x / scale);
            uint32_t imgY = height - 1 - (uint32_t)(y / scale); // Inverte Y
            
            if (imgX < width && imgY < height) {
                // Calcula posição no array de dados
                uint32_t pixelOffset = imgY * rowSize + imgX * 3;
                
                // Lê pixel BGR
                uint8_t b = imageData[pixelOffset];
                uint8_t g = imageData[pixelOffset + 1];
                uint8_t r = imageData[pixelOffset + 2];
                
                // Converte para RGB565
                uint16_t color = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
                
                lcd.drawPixel(offsetX + x, offsetY + y, color);
            }
        }
    }
    
    Serial.println("✓ Renderização concluída");
}

bool WallpaperManager::initializeWallpaper() {
    Serial.println("\n🚀 === INICIALIZANDO WALLPAPER ===");
    
    // Debug completo do SPIFFS
    debugSPIFFS();
    
    return true;
}

bool WallpaperManager::loadWallpaperFromFile(const char* filename) {
    // Tenta carregar bkgnd.bmp primeiro
    if (loadBMPFile("/bkgnd2.bmp")) {
        return true;
    }
    
    // Se não encontrar, tenta o arquivo especificado
    if (filename && loadBMPFile(filename)) {
        return true;
    }
    
    // Lista de arquivos alternativos para tentar
    String filesToTry[] = {
        "/wallpaper.bmp",
        "/background.bmp",
        "/img.bmp"
    };
    
    for (const String& file : filesToTry) {
        if (loadBMPFile(file.c_str())) {
            return true;
        }
    }
    
    Serial.println("❌ Nenhum arquivo BMP válido encontrado!");
    drawTestImage();
    return false;
}

void WallpaperManager::displayWallpaperWithOverlay() {
    Serial.println("\n=== EXIBINDO WALLPAPER ===");
    
    if (!loadWallpaperFromFile("/bkgnd2.bmp")) {
        Serial.println("Falha ao carregar wallpaper - usando imagem de teste");
    }
    
    delay(2000);
}

bool WallpaperManager::isWallpaperLoaded() {
    return wallpaperLoaded;
}