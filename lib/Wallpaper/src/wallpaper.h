#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <Arduino.h>

class WallpaperManager {
private:
    static bool wallpaperLoaded;
    
    // Novos métodos para BMP direto
    static bool loadBMPFile(const char* filename);
    static bool processBMP(uint8_t* bmpData, size_t dataSize);
    static void renderBMPToDisplay(uint8_t* imageData, uint32_t width, uint32_t height);

public:
    static bool initializeWallpaper();
    static bool loadWallpaperFromFile(const char* filename = nullptr);
    static void displayWallpaperWithOverlay();
    static bool isWallpaperLoaded();
    static void displayBMP(uint8_t* bmpData, int dataSize); // Método existente
};

#endif