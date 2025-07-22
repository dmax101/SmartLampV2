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
    delay(2000); // Aguarda serial monitor
    
    Serial.println("=== SMART LAMP INICIANDO ===");
    
    // Inicializa SPIFFS primeiro
    Serial.println("Inicializando SPIFFS...");
    if (!SPIFFS.begin(true)) { // true = format if failed
        Serial.println("ERRO: Falha ao inicializar SPIFFS");
    } else {
        Serial.println("SPIFFS inicializado com sucesso");
        
        // Lista arquivos
        File root = SPIFFS.open("/");
        File file = root.openNextFile();
        Serial.println("Arquivos no SPIFFS:");
        while (file) {
            Serial.printf("  %s (%d bytes)\n", file.name(), file.size());
            file = root.openNextFile();
        }
    }
    
    // Inicializa display
    Serial.println("Inicializando display...");
    initDisplay();
    
    // Mostra mensagem inicial
    showStatusMessage("Smart Lamp v2.0", ST77XX_WHITE);
    delay(2000);
    
    // Inicializa e testa wallpaper
    Serial.println("Inicializando wallpaper...");
    WallpaperManager::initializeWallpaper();
    
    // Testa o wallpaper imediatamente
    Serial.println("Testando carregamento de wallpaper...");
    clearStatusMessages();
    WallpaperManager::displayWallpaperWithOverlay();
    
    // Aguarda para ver o resultado
    delay(5000);
    
    // Conecta ao WiFi
    if (WiFiManager::connectToWiFi()) {
        clearStatusMessages();
        TimeManager::initNTP();
        
        // Obtém dados do clima
        Serial.println("Obtendo dados do clima...");
        bool climaOK = weatherManager.updateWeatherData();
        if (!climaOK) {
            weatherManager.setDefaultValues();
        }
        
        // Recarrega wallpaper após configurações
        Serial.println("Recarregando wallpaper...");
        WallpaperManager::displayWallpaperWithOverlay();
        
        systemInitialized = true;
    } else {
        showErrorMessage("Erro WiFi!");
        delay(5000);
    }
    
    forceFullUpdate = true;
}

void loop() {
    // Se não inicializado, tenta novamente
    if (!systemInitialized) {
        if (WiFiManager::connectToWiFi()) {
            clearStatusMessages();
            TimeManager::initNTP();
            WallpaperManager::displayWallpaperWithOverlay();
            systemInitialized = true;
            forceFullUpdate = true;
        } else {
            delay(5000);
            return;
        }
    }
    
    // Controla frequência de atualização
    if (millis() - lastDisplayUpdate < displayUpdateInterval) {
        delay(10);
        return;
    }
    lastDisplayUpdate = millis();
    
    // Verifica WiFi
    if (!WiFiManager::isConnected()) {
        if (forceFullUpdate) {
            clearStatusMessages();
            WiFiManager::showConnectionStatus();
            forceFullUpdate = false;
            systemInitialized = false;
        }
        delay(1000);
        return;
    }
    
    // Detecta mudança de minuto
    bool minuteChanged = TimeManager::hasMinuteChanged();
    if (minuteChanged) {
        forceFullUpdate = true;
    }
    
    // Atualiza clima se necessário
    if (weatherManager.needsUpdate()) {
        Serial.println("Atualizando dados do clima...");
        if (weatherManager.updateWeatherData()) {
            forceFullUpdate = true;
        }
    }
    
    // Redesenha interface
    if (forceFullUpdate || minuteChanged) {
        // Verifica se wallpaper precisa ser recarregado
        if (!WallpaperManager::isWallpaperLoaded()) {
            Serial.println("Wallpaper não carregado, recarregando...");
            WallpaperManager::displayWallpaperWithOverlay();
        }
        
        WeatherData currentWeather = weatherManager.getCurrentWeather();
        drawInterfaceElements(currentWeather);
        forceFullUpdate = false;
        
        Serial.printf("Display atualizado - Temp: %.1f°C | Umidade: %.0f%%\n", 
                      currentWeather.temperatura, currentWeather.umidade);
    }
}