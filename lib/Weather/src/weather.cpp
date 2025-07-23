#include "weather.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <config.h>

WeatherManager::WeatherManager() {
    ultimaAtualizacao = 0;
    setDefaultValues();
}

void WeatherManager::begin() {
    Serial.println("WeatherManager inicializado");
    setDefaultValues();
    // Força primeira atualização
    ultimaAtualizacao = 0;
}

bool WeatherManager::updateWeatherData() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi não conectado para atualizar clima");
        return false;
    }
    
    return fetchWeatherFromAPI();
}

WeatherData WeatherManager::getCurrentWeather() const {
    return currentWeather;
}

bool WeatherManager::needsUpdate() const {
    return (millis() - ultimaAtualizacao) > intervaloAtualizacao;
}

void WeatherManager::setDefaultValues() {
    currentWeather.temperatura = 25.0;
    currentWeather.umidade = 60.0;
    currentWeather.descricaoClima = "Ensolarado";
    currentWeather.iconeClima = "01d";
}

bool WeatherManager::fetchWeatherFromAPI() {
    HTTPClient http;
    http.begin(apiURL);
    
    int httpResponseCode = http.GET();
    
    if (httpResponseCode == 200) {
        String payload = http.getString();
        
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload);
        
        currentWeather.temperatura = doc["main"]["temp"];
        currentWeather.umidade = doc["main"]["humidity"];
        currentWeather.descricaoClima = doc["weather"][0]["description"].as<String>();
        currentWeather.iconeClima = doc["weather"][0]["icon"].as<String>();
        
        ultimaAtualizacao = millis();
        
        Serial.printf("Clima atualizado: %.1f°C, %.0f%%, %s\n", 
                      currentWeather.temperatura, currentWeather.umidade, 
                      currentWeather.descricaoClima.c_str());
        
        http.end();
        return true;
    } else {
        Serial.printf("Erro HTTP: %d\n", httpResponseCode);
        http.end();
        return false;
    }
}