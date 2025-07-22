#include "ImageConverter.h"

int ImageConverter::base64_decode(const char* input, uint8_t* output, int outputLen) {
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

bool ImageConverter::validateBMP(uint8_t* bmpData, int dataSize) {
    if (dataSize < 54) return false;
    if (bmpData[0] != 'B' || bmpData[1] != 'M') return false;
    return true;
}

bool ImageConverter::loadAndConvertImage(const char* filename, uint8_t** outputBuffer, int* outputSize) {
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
    
    Serial.println("Convertendo imagem Base64 para BMP...");
    
    // Calcula tamanho necessário para o buffer
    int estimatedSize = (base64Data.length() * 3) / 4;
    *outputBuffer = (uint8_t*)malloc(estimatedSize);
    
    if (!*outputBuffer) {
        Serial.println("Erro ao alocar memória para conversão");
        return false;
    }
    
    *outputSize = base64_decode(base64Data.c_str(), *outputBuffer, estimatedSize);
    
    if (*outputSize > 0 && validateBMP(*outputBuffer, *outputSize)) {
        Serial.printf("Imagem convertida com sucesso: %d bytes\n", *outputSize);
        return true;
    } else {
        Serial.println("Erro na conversão ou arquivo BMP inválido");
        free(*outputBuffer);
        *outputBuffer = nullptr;
        return false;
    }
}

bool ImageConverter::saveConvertedBMP(const char* base64File, const char* bmpFile) {
    uint8_t* bmpData;
    int bmpSize;
    
    if (!loadAndConvertImage(base64File, &bmpData, &bmpSize)) {
        return false;
    }
    
    File file = SPIFFS.open(bmpFile, "w");
    if (!file) {
        Serial.printf("Erro ao criar arquivo %s\n", bmpFile);
        free(bmpData);
        return false;
    }
    
    file.write(bmpData, bmpSize);
    file.close();
    free(bmpData);
    
    Serial.printf("Arquivo BMP salvo: %s\n", bmpFile);
    return true;
}

void ImageConverter::cleanup(uint8_t* buffer) {
    if (buffer) {
        free(buffer);
    }
}