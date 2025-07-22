#include <Arduino.h>
#include <SPIFFS.h>
#include "config.h"
#include "display.h"
#include "weather.h"
#include "wallpaper.h"
#include "utils.h"

// Variáveis de controle
WeatherManager weatherManager;
unsigned long lastDisplayUpdate = 0;
bool forceFullUpdate = true;
bool systemInitialized = false;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Inicializa SPIFFS
    if (!SPIFFS.begin()) {
        Serial.println("Erro ao inicializar SPIFFS");
    }
    
    // Inicializa display
    initDisplay();
    
    // Inicializa sistema de wallpaper
    WallpaperManager::initializeWallpaper();
    
    // Conecta ao WiFi
    if (WiFiManager::connectToWiFi()) {
        // Limpa mensagens de status após conexão
        delay(1000);
        clearStatusMessages();
        
        // Configura NTP
        TimeManager::initNTP();
        
        // Obtém dados iniciais do clima
        Serial.println("Obtendo dados do clima...");
        showStatusMessage("Carregando clima...", ST77XX_YELLOW);
        
        bool climaOK = false;
        for (int i = 0; i < 3 && !climaOK; i++) {
            Serial.printf("Tentativa %d de obter dados do clima...\n", i + 1);
            climaOK = weatherManager.updateWeatherData();
            if (!climaOK) {
                delay(2000);
            }
        }
        
        if (climaOK) {
            showStatusMessage("Clima OK!", ST77XX_GREEN);
            delay(1000);
        } else {
            showStatusMessage("Erro no clima!", ST77XX_RED);
            delay(1000);
            weatherManager.setDefaultValues();
        }
        
        // Limpa todas as mensagens de status
        clearStatusMessages();
        delay(500);
        
        // Carrega papel de parede
        Serial.println("Carregando papel de parede...");
        WallpaperManager::displayWallpaperWithOverlay();
        
        systemInitialized = true;
    } else {
        // Erro de conexão - mantém mensagem na tela
        delay(5000);
    }
    
    forceFullUpdate = true;
}

void loop() {
    // Se o sistema não foi inicializado, tenta reconectar
    if (!systemInitialized) {
        if (WiFiManager::connectToWiFi()) {
            clearStatusMessages();
            TimeManager::initNTP();
            WallpaperManager::displayWallpaperWithOverlay();
            systemInitialized = true;
            forceFullUpdate = true;
        } else {
            delay(5000); // Aguarda 5 segundos antes de tentar novamente
            return;
        }
    }
    
    // Controla frequência de atualização
    if (millis() - lastDisplayUpdate < displayUpdateInterval) {
        delay(10);
        return;
    }
    lastDisplayUpdate = millis();
    
    // Verifica conexão WiFi
    if (!WiFiManager::isConnected()) {
        if (forceFullUpdate) {
            clearStatusMessages();
            WiFiManager::showConnectionStatus();
            forceFullUpdate = false;
            systemInitialized = false; // Força reinicialização
        }
        delay(1000);
        return;
    }
    
    // Detecta mudança de minuto
    bool minuteChanged = TimeManager::hasMinuteChanged();
    if (minuteChanged) {
        forceFullUpdate = true;
    }
    
    // Atualiza dados do clima
    if (weatherManager.needsUpdate()) {
        Serial.println("Atualizando dados do clima...");
        if (weatherManager.updateWeatherData()) {
            forceFullUpdate = true;
        }
    }
    
    // Redesenha interface se necessário
    if (forceFullUpdate || minuteChanged) {
        // Se o wallpaper não estiver carregado, carrega novamente
        if (!WallpaperManager::isWallpaperLoaded()) {
            WallpaperManager::displayWallpaperWithOverlay();
        }
        
        WeatherData currentWeather = weatherManager.getCurrentWeather();
        drawInterfaceElements(currentWeather);
        forceFullUpdate = false;
        
        Serial.printf("Display atualizado - Temp: %.1f°C | Umidade: %.0f%%\n", 
                      currentWeather.temperatura, currentWeather.umidade);
    }
}