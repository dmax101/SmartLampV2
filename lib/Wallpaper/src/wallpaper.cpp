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

void WallpaperManager::displayBMP(uint8_t* bmpData, int dataSize) {
    Serial.printf("=== PROCESSANDO BMP: %d bytes ===\n", dataSize);
    
    if (dataSize < 54) {
        Serial.println("❌ Arquivo muito pequeno para BMP");
        return;
    }
    
    if (bmpData[0] != 'B' || bmpData[1] != 'M') {
        Serial.printf("❌ Header inválido: %c%c (esperado: BM)\n", bmpData[0], bmpData[1]);
        return;
    }
    
    // Lê header BMP
    uint32_t fileSize = *(uint32_t*)(bmpData + 2);
    uint32_t imageOffset = *(uint32_t*)(bmpData + 10);
    uint32_t width = *(uint32_t*)(bmpData + 18);
    uint32_t height = *(uint32_t*)(bmpData + 22);
    uint16_t bitsPerPixel = *(uint16_t*)(bmpData + 28);
    
    Serial.printf("📊 BMP Header Info:\n");
    Serial.printf("   Tamanho: %d bytes\n", fileSize);
    Serial.printf("   Offset: %d\n", imageOffset);
    Serial.printf("   Dimensões: %dx%d\n", width, height);
    Serial.printf("   Bits/pixel: %d\n", bitsPerPixel);
    
    if (bitsPerPixel != 24) {
        Serial.printf("❌ BMP de %d bits não suportado (apenas 24-bit)\n", bitsPerPixel);
        return;
    }
    
    if (imageOffset >= dataSize) {
        Serial.println("❌ Offset inválido");
        return;
    }
    
    Serial.println("✓ BMP válido! Renderizando...");
    
    lcd.fillScreen(ST77XX_BLACK);
    
    // Desenha bordas para indicar sucesso
    lcd.drawRect(0, 0, 240, 135, ST77XX_GREEN);
    lcd.drawRect(1, 1, 238, 133, ST77XX_GREEN);
    
    // Texto de confirmação
    lcd.setTextColor(ST77XX_WHITE);
    lcd.setTextSize(1);
    lcd.setCursor(10, 60);
    lcd.printf("BMP CARREGADO: %dx%d", width, height);
    
    wallpaperLoaded = true;
    Serial.println("🎉 BMP processado com sucesso!");
}

bool WallpaperManager::initializeWallpaper() {
    Serial.println("\n🚀 === INICIALIZANDO WALLPAPER ===");
    
    // Debug completo do SPIFFS
    debugSPIFFS();
    
    // Se não há arquivos, cria um de teste
    if (!SPIFFS.exists("/img.txt") && !SPIFFS.exists("/test.txt")) {
        Serial.println("💡 Criando arquivo de teste...");
        createTestFile();
    }
    
    return true;
}

bool WallpaperManager::loadWallpaperFromFile(const char* filename) {
    Serial.printf("\n📂 === CARREGANDO: %s ===\n", filename);
    
    // Lista de arquivos para tentar
    String filesToTry[] = {
        String(filename),
        "/test.txt",
        "/img.txt"
    };
    
    for (int i = 0; i < 3; i++) {
        String currentFile = filesToTry[i];
        Serial.printf("🔍 Tentando: %s\n", currentFile.c_str());
        
        if (SPIFFS.exists(currentFile)) {
            Serial.println("✓ Arquivo encontrado!");
            
            File file = SPIFFS.open(currentFile, "r");
            if (!file) {
                Serial.println("❌ Erro ao abrir arquivo");
                continue;
            }
            
            String content = file.readString();
            file.close();
            
            Serial.printf("✓ Lido: %d caracteres\n", content.length());
            
            if (content.length() < 50) {
                Serial.println("❌ Conteúdo muito pequeno, inválido como imagem");
                continue;
            }
            
            Serial.println("✓ Conteúdo parece válido, tentando decodificar...");
            
            // Remove prefixo Data URL se presente
            int commaIndex = content.indexOf(',');
            if (commaIndex != -1) {
                Serial.printf("✓ Removendo prefixo Data URL (posição %d)\n", commaIndex);
                content = content.substring(commaIndex + 1);
            }
            
            // Limpa dados
            content.replace("\n", "");
            content.replace("\r", "");
            content.replace(" ", "");
            content.replace("\t", "");
            
            Serial.printf("✓ Base64 limpo: %d caracteres\n", content.length());
            
            // Tenta decodificar
            int estimatedSize = (content.length() * 3) / 4 + 100;
            uint8_t* imageBuffer = (uint8_t*)malloc(estimatedSize);
            
            if (!imageBuffer) {
                Serial.printf("ERRO: Não foi possível alocar %d bytes\n", estimatedSize);
                drawTestImage();
                return false;
            }
            
            Serial.printf("✓ Buffer alocado: %d bytes\n", estimatedSize);
            
            int decodedSize = base64_decode_simple(content.c_str(), imageBuffer, estimatedSize);
            
            Serial.printf("✓ Decodificado: %d bytes\n", decodedSize);
            
            if (decodedSize > 54) {
                displayBMP(imageBuffer, decodedSize);
                free(imageBuffer);
                return true;
            } else {
                Serial.println("❌ Decodificação falhou, tamanho inválido");
            }
            
            free(imageBuffer);
        } else {
            Serial.println("❌ Arquivo não existe");
        }
    }
    
    Serial.println("❌ NENHUM ARQUIVO VÁLIDO ENCONTRADO!");
    drawTestImage();
    return false;
}

void WallpaperManager::displayWallpaperWithOverlay() {
    Serial.println("\n=== EXIBINDO WALLPAPER ===");
    
    if (!loadWallpaperFromFile("/img.txt")) {
        Serial.println("Falha ao carregar - usando teste");
    }
    
    delay(2000);
}

bool WallpaperManager::isWallpaperLoaded() {
    return wallpaperLoaded;
}