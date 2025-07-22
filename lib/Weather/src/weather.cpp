#include "weather.h"
#include <config.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

WeatherManager::WeatherManager() {
    ultimaAtualizacao = 0;
    setDefaultValues();
}

void WeatherManager::setDefaultValues() {
    currentWeather.temperatura = 25.0;
    currentWeather.umidade = 60.0;
    currentWeather.descricaoClima = "Clima indisponível";
    currentWeather.iconeClima = "01d";
}

bool WeatherManager::fetchWeatherFromAPI() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi não conectado para buscar clima");
        return false;
    }
    
    HTTPClient http;
    http.begin(apiURL);
    http.setTimeout(10000); // 10 segundos timeout
    
    Serial.println("Buscando dados do clima...");
    Serial.println("URL: " + apiURL);
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("Resposta recebida:");
        Serial.println(payload);
        
        // Parse JSON
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);
        
        if (error) {
            Serial.print("Erro no parse JSON: ");
            Serial.println(error.c_str());
            http.end();
            return false;
        }
        
        // Extrai dados
        currentWeather.temperatura = doc["main"]["temp"];
        currentWeather.umidade = doc["main"]["humidity"];
        currentWeather.descricaoClima = doc["weather"][0]["description"].as<String>();
        currentWeather.iconeClima = doc["weather"][0]["icon"].as<String>();
        
        Serial.printf("Clima atualizado: %.1f°C, %.0f%% umidade\n", 
                      currentWeather.temperatura, currentWeather.umidade);
        
        http.end();
        return true;
        
    } else {
        Serial.printf("Erro HTTP: %d\n", httpCode);
        if (httpCode > 0) {
            Serial.println("Resposta: " + http.getString());
        }
        http.end();
        return false;
    }
}

bool WeatherManager::updateWeatherData() {
    bool success = fetchWeatherFromAPI();
    if (success) {
        ultimaAtualizacao = millis();
    }
    return success;
}

WeatherData WeatherManager::getCurrentWeather() const {
    return currentWeather;
}

bool WeatherManager::needsUpdate() const {
    return (millis() - ultimaAtualizacao) > intervaloAtualizacao;
}