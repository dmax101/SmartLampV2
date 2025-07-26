#include <Arduino.h>
#include <SPIFFS.h>
#include "config.h"
#include "display.h"
#include "weather.h"
#include "wallpaper.h"
#include "utils.h"
#include "sensors.h"
#include "lampControl.h" // Adiciona controle das lâmpadas

// Variáveis de controle
WeatherManager weatherManager;
unsigned long lastDisplayUpdate = 0;
bool forceFullUpdate = true;
bool systemInitialized = false;

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.println("=== SMART LAMP INICIANDO ===");

    // Inicializa controle das lâmpadas PRIMEIRO (executa mensagem especial)
    Serial.println("Inicializando controle das lâmpadas...");
    LampControl::begin();

    // Inicializa SPIFFS primeiro
    Serial.println("Inicializando SPIFFS...");
    if (!SPIFFS.begin(true))
    {
        Serial.println("Erro ao montar SPIFFS!");
        return;
    }
    else
    {
        Serial.println("SPIFFS montado com sucesso");
    }

    // Inicializa display
    Serial.println("Inicializando display...");
    initDisplay();

    // Mostra mensagem inicial
    showStatusMessage("Smart Lamp v2.0", ST77XX_WHITE);
    delay(2000);

    // Conecta WiFi
    Serial.println("Conectando WiFi...");
    showStatusMessage("Conectando WiFi...", ST77XX_YELLOW);
    if (WiFiManager::connectToWiFi())
    {
        showStatusMessage("WiFi Conectado!", ST77XX_GREEN);
        delay(1000);

        // Inicializa NTP
        Serial.println("Sincronizando horário...");
        showStatusMessage("Sincronizando horário...", ST77XX_CYAN);
        TimeManager::initNTP();
        delay(1000);
    }
    else
    {
        showStatusMessage("Falha WiFi!", ST77XX_RED);
        delay(2000);
    }

    // Inicializa weather manager
    Serial.println("Inicializando gerenciador de clima...");
    weatherManager.begin(); // Agora deve funcionar

    // Inicializa e testa wallpaper
    Serial.println("Inicializando wallpaper...");
    WallpaperManager::initializeWallpaper();

    Serial.println("Inicializando sensores...");
    Sensors::begin();

    systemInitialized = true;
    forceFullUpdate = true;
    Serial.println("=== INICIALIZAÇÃO COMPLETA ===");
}

void loop()
{
    // Verifica se houve mudança no estado do display pelo botão
    static bool lastDisplayState = true;
    if (displayState != lastDisplayState)
    {
        toggleDisplay();
        lastDisplayState = displayState;

        // Se o display foi ligado, força atualização completa e recarrega wallpaper
        if (displayState)
        {
            Serial.println("Display ligado - forçando atualização completa");
            forceFullUpdate = true;

            // Pequeno delay para estabilizar o display
            delay(100);

            // Força recarregamento do wallpaper
            WallpaperManager::displayWallpaperWithOverlay();
        }
    }

    // Se o display está desligado, não precisa fazer mais processamento
    if (!displayState)
    {
        delay(10);
        return;
    }

    // Verifica conexão WiFi
    if (!WiFiManager::isConnected())
    {
        if (systemInitialized)
        {
            Serial.println("WiFi desconectado, tentando reconectar...");
            WiFiManager::reconnect();

            if (WiFiManager::isConnected())
            {
                TimeManager::initialize();
            }
        }
        delay(5000);
        return;
    }

    // Controla frequência de atualização
    if (millis() - lastDisplayUpdate < displayUpdateInterval)
    {
        delay(10);
        return;
    }
    lastDisplayUpdate = millis();

    // Detecta mudança de minuto
    bool minuteChanged = TimeManager::hasMinuteChanged();
    if (minuteChanged)
    {
        forceFullUpdate = true;
    }

    // Atualiza clima se necessário
    if (weatherManager.needsUpdate())
    {
        Serial.println("Atualizando dados do clima...");
        if (weatherManager.updateWeatherData())
        {
            forceFullUpdate = true;
        }
    }

    // Redesenha interface
    if (forceFullUpdate || minuteChanged)
    {
        // Só atualiza se o display estiver ligado
        if (isDisplayOn())
        {
            // Verifica se wallpaper precisa ser recarregado
            if (!WallpaperManager::isWallpaperLoaded() || forceFullUpdate)
            {
                Serial.println("Wallpaper não carregado ou atualização forçada, recarregando...");
                WallpaperManager::displayWallpaperWithOverlay();
            }

            WeatherData currentWeather = weatherManager.getCurrentWeather();
            currentWeather.temperatura = Sensors::getTemperature();
            currentWeather.umidade = Sensors::getHumidity();

            drawInterfaceElements(currentWeather);

            Serial.printf("Display atualizado - Temp: %.1f°C | Umidade: %.0f%%\n",
                          currentWeather.temperatura, currentWeather.umidade);
        }

        forceFullUpdate = false;
    }
}