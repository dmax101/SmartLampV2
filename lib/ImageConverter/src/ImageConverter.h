#ifndef IMAGE_CONVERTER_H
#define IMAGE_CONVERTER_H

#include <Arduino.h>
#include <SPIFFS.h>

class ImageConverter {
private:
    static int base64_decode(const char* input, uint8_t* output, int outputLen);
    static bool validateBMP(uint8_t* bmpData, int dataSize);
    
public:
    static bool loadAndConvertImage(const char* filename, uint8_t** outputBuffer, int* outputSize);
    static bool saveConvertedBMP(const char* base64File, const char* bmpFile);
    static void cleanup(uint8_t* buffer);
};

#endif