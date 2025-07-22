#include "weather.h"
#include "config.h"
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
  currentWeather.descricaoClima = "Sem dados";
  currentWeather.iconeClima = "01d";
}

bool WeatherManager::needsUpdate() const {
  return (millis() - ultimaAtualizacao > intervaloAtualizacao);
}

WeatherData WeatherManager::getCurrentWeather() const {
  return currentWeather;
}

bool WeatherManager::updateWeatherData() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi não conectado");
    return false;
  }
  
  Serial.println("Iniciando requisição HTTP...");
  Serial.println("URL: " + apiURL);
  
  HTTPClient http;
  http.begin(apiURL);
  http.setTimeout(10000);
  
  int httpResponseCode = http.GET();
  Serial.printf("Código de resposta HTTP: %d\n", httpResponseCode);
  
  if (httpResponseCode > 0) {
    String payload = http.getString();
    Serial.println("Resposta da API:");
    Serial.println(payload);
    
    if (payload.length() < 10) {
      Serial.println("Resposta muito pequena");
      http.end();
      return false;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
      Serial.print("Erro ao analisar JSON: ");
      Serial.println(error.c_str());
      http.end();
      return false;
    }
    
    // Usando nova sintaxe do ArduinoJson
    if (doc["main"].is<JsonObject>() && doc["main"]["temp"].is<float>()) {
      currentWeather.temperatura = doc["main"]["temp"];
      currentWeather.umidade = doc["main"]["humidity"];
      
      if (doc["weather"].is<JsonArray>() && doc["weather"].size() > 0) {
        currentWeather.descricaoClima = doc["weather"][0]["description"].as<String>();
        currentWeather.iconeClima = doc["weather"][0]["icon"].as<String>();
      }
      
      Serial.printf("Dados extraídos com sucesso:\n");
      Serial.printf("Temperatura: %.1f°C\n", currentWeather.temperatura);
      Serial.printf("Umidade: %.0f%%\n", currentWeather.umidade);
      Serial.printf("Descrição: %s\n", currentWeather.descricaoClima.c_str());
      Serial.printf("Ícone: %s\n", currentWeather.iconeClima.c_str());
      
      ultimaAtualizacao = millis();
      http.end();
      return true;
    } else {
      Serial.println("Dados de temperatura não encontrados no JSON");
      
      if (doc["message"].is<const char*>()) {
        Serial.println("Mensagem de erro da API: " + doc["message"].as<String>());
      }
      
      http.end();
      return false;
    }
  } else {
    Serial.printf("Erro na requisição HTTP: %d\n", httpResponseCode);
    
    if (httpResponseCode == -1) {
      Serial.println("Erro: Timeout ou falha de conexão");
    } else if (httpResponseCode == 401) {
      Serial.println("Erro: API Key inválida");
    } else if (httpResponseCode == 404) {
      Serial.println("Erro: Cidade não encontrada");
    }
    
    http.end();
    return false;
  }
}