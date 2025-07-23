#ifndef WEATHER_H
#define WEATHER_H

#include <Arduino.h>
#include <display.h>

// Classe para gerenciar dados do clima
class WeatherManager {
private:
    WeatherData currentWeather;
    unsigned long ultimaAtualizacao;
    bool fetchWeatherFromAPI();
    
public:
    WeatherManager();
    void begin(); // Adicionar este método
    bool updateWeatherData();
    WeatherData getCurrentWeather() const;
    bool needsUpdate() const;
    void setDefaultValues();
};

#endif