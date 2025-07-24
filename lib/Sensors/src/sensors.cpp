#include "sensors.h"
#include <AHT10.h>
#include <Wire.h>

static AHT10 aht10;

void Sensors::begin() {
    Wire.begin(21, 22); // SDA = GPO21, SCL = GPO22
    delay(100);       // Aguarda estabilização do barramento I2C
    aht10.begin();
    Serial.println("[AHT10] Inicializado");
}

float Sensors::getTemperature() {
    float temp = aht10.readTemperature();
    Serial.printf("[AHT10] Temperatura lida: %.2f\n", temp);
    if (isnan(temp)) {
        Serial.println("[AHT10] Erro ao ler temperatura!");
        return -127.0; // Valor de erro
    }
    return temp;
}

float Sensors::getHumidity() {
    float hum = aht10.readHumidity();
    Serial.printf("[AHT10] Umidade lida: %.2f\n", hum);
    if (isnan(hum)) {
        Serial.println("[AHT10] Erro ao ler umidade!");
        return -1.0; // Valor de erro
    }
    return hum;
}