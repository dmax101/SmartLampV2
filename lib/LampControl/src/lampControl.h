#ifndef LAMP_CONTROL_H
#define LAMP_CONTROL_H

#include <Arduino.h>

// Pino de controle das lâmpadas (conectado à base do transistor BC547)
#define LAMP_PIN 13

// Tempos para código Morse (em milissegundos) - Versão RÁPIDA
#define DOT_DURATION 100  // Duração do ponto (.) - era 200ms
#define DASH_DURATION 300 // Duração do traço (-) - era 600ms
#define SYMBOL_PAUSE 50   // Pausa entre símbolos da mesma letra - era 200ms
#define LETTER_PAUSE 150  // Pausa entre letras - era 600ms
#define WORD_PAUSE 400    // Pausa entre palavras - era 1400ms

class LampControl
{
private:
    static bool lampState;
    static void dot();
    static void dash();
    static void letterPause();
    static void wordPause();
    static void transmitLetter(char letter);

    // Variáveis para controle não-bloqueante da mensagem contínua
    static bool continuousMode;
    static unsigned long lastMorseTime;
    static int currentMessageIndex;
    static int currentLetterIndex;
    static int currentSymbolIndex;
    static bool inPause;
    static unsigned long pauseStartTime;
    static int pauseType; // 0=symbol, 1=letter, 2=word, 3=message_end
    static const char *continuousMessage;

    static bool lampOnForMorse;

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

    // Controle da mensagem contínua
    static void startContinuousMessage();
    static void stopContinuousMessage();
    static void updateContinuousMessage(); // Deve ser chamada no loop principal
    static bool isContinuousModeActive();

    // Sincronização com display
    static void syncWithDisplay(bool displayOn);
};

#endif
