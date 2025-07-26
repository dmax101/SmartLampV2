#ifndef LAMP_CONTROL_H
#define LAMP_CONTROL_H

#include <Arduino.h>

// Pino de controle das lâmpadas (conectado à base do transistor BC547)
#define LAMP_PIN 13

// Tempos para código Morse (em milissegundos)
#define DOT_DURATION 200  // Duração do ponto (.)
#define DASH_DURATION 600 // Duração do traço (-)
#define SYMBOL_PAUSE 200  // Pausa entre símbolos da mesma letra
#define LETTER_PAUSE 600  // Pausa entre letras
#define WORD_PAUSE 1400   // Pausa entre palavras

class LampControl
{
private:
    static bool lampState;
    static void dot();
    static void dash();
    static void letterPause();
    static void wordPause();
    static void transmitLetter(char letter);

public:
    // Inicializa o controle das lâmpadas
    static void begin();

    // Controle básico das lâmpadas
    static void turnOn();
    static void turnOff();
    static void toggle();
    static bool isOn();

    // Pisca a lâmpada por um tempo determinado
    static void blink(int times = 1, int onTime = 500, int offTime = 500);

    // Transmite uma mensagem em código Morse
    static void morseMessage(const char *message);

    // Transmite a frase especial "Sofia é Linda"
    static void transmitSofiaMessage();
};

#endif
