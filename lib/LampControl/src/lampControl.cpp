#include "lampControl.h"

// Variável estática para controlar o estado da lâmpada
bool LampControl::lampState = false;

// Tabela de código Morse para letras A-Z
const char *morseTable[26] = {
    ".-",   // A
    "-...", // B
    "-.-.", // C
    "-..",  // D
    ".",    // E
    "..-.", // F
    "--.",  // G
    "....", // H
    "..",   // I
    ".---", // J
    "-.-",  // K
    ".-..", // L
    "--",   // M
    "-.",   // N
    "---",  // O
    ".--.", // P
    "--.-", // Q
    ".-.",  // R
    "...",  // S
    "-",    // T
    "..-",  // U
    "...-", // V
    ".--",  // W
    "-..-", // X
    "-.--", // Y
    "--.."  // Z
};

void LampControl::begin()
{
    Serial.println("Inicializando controle das lâmpadas...");

    // Configura o pino como saída
    pinMode(LAMP_PIN, OUTPUT);

    // Inicializa com lâmpada desligada
    turnOff();

    Serial.println("Controle das lâmpadas inicializado no pino D13");

    // Executa a mensagem especial na inicialização
    Serial.println("Transmitindo mensagem especial em código Morse...");
    delay(1000); // Aguarda 1 segundo antes de começar
    transmitSofiaMessage();
}

void LampControl::turnOn()
{
    digitalWrite(LAMP_PIN, HIGH);
    lampState = true;
    Serial.println("💡 Lâmpadas LIGADAS");
}

void LampControl::turnOff()
{
    digitalWrite(LAMP_PIN, LOW);
    lampState = false;
    Serial.println("💡 Lâmpadas DESLIGADAS");
}

void LampControl::toggle()
{
    if (lampState)
    {
        turnOff();
    }
    else
    {
        turnOn();
    }
}

bool LampControl::isOn()
{
    return lampState;
}

void LampControl::blink(int times, int onTime, int offTime)
{
    Serial.printf("💡 Piscando %d vezes (ON:%dms, OFF:%dms)\n", times, onTime, offTime);

    for (int i = 0; i < times; i++)
    {
        turnOn();
        delay(onTime);
        turnOff();
        delay(offTime);
    }
}

void LampControl::dot()
{
    turnOn();
    delay(DOT_DURATION);
    turnOff();
    delay(SYMBOL_PAUSE);
    Serial.print(".");
}

void LampControl::dash()
{
    turnOn();
    delay(DASH_DURATION);
    turnOff();
    delay(SYMBOL_PAUSE);
    Serial.print("-");
}

void LampControl::letterPause()
{
    delay(LETTER_PAUSE);
    Serial.print(" ");
}

void LampControl::wordPause()
{
    delay(WORD_PAUSE);
    Serial.print(" / ");
}

void LampControl::transmitLetter(char letter)
{
    // Converte para maiúscula
    if (letter >= 'a' && letter <= 'z')
    {
        letter = letter - 'a' + 'A';
    }

    // Verifica se é uma letra válida
    if (letter >= 'A' && letter <= 'Z')
    {
        const char *morse = morseTable[letter - 'A'];
        Serial.printf("[%c: %s] ", letter, morse);

        // Transmite cada símbolo da letra
        for (int i = 0; morse[i] != '\0'; i++)
        {
            if (morse[i] == '.')
            {
                dot();
            }
            else if (morse[i] == '-')
            {
                dash();
            }
        }

        letterPause();
    }
    else if (letter == ' ')
    {
        wordPause();
    }
}

void LampControl::morseMessage(const char *message)
{
    Serial.printf("📡 Transmitindo em Morse: \"%s\"\n", message);
    Serial.print("🔤 Código: ");

    int len = strlen(message);
    for (int i = 0; i < len; i++)
    {
        transmitLetter(message[i]);
    }

    Serial.println("\n✅ Transmissão completa!");
}

void LampControl::transmitSofiaMessage()
{
    Serial.println("💝 === TRANSMITINDO MENSAGEM ESPECIAL ===");
    Serial.println("💝 Mensagem: \"Sofia é Linda\"");

    // Pisca 3 vezes antes de começar para indicar início
    Serial.println("💡 Sinal de início...");
    blink(3, 300, 300);
    delay(1000);

    // Transmite a mensagem em código Morse
    morseMessage("SOFIA E LINDA");

    // Pisca 5 vezes no final para indicar fim
    Serial.println("💡 Sinal de fim...");
    delay(1000);
    blink(5, 200, 200);

    Serial.println("💝 === MENSAGEM ESPECIAL TRANSMITIDA ===");
}
