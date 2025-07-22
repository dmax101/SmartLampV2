#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <Arduino.h>

class WallpaperManager {
private:
    static void displayBMP(uint8_t* bmpData, int dataSize);
    static bool wallpaperLoaded;
    
public:
    static bool initializeWallpaper();
    static bool loadWallpaperFromFile(const char* filename = "/img.txt");
    static void displayWallpaperWithOverlay();
    static bool isWallpaperLoaded();
};

#endif