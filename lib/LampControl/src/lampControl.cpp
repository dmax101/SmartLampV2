#include "lampControl.h"

// Variável estática para controlar o estado da lâmpada
bool LampControl::lampState = false;

// Variáveis para controle não-bloqueante da mensagem contínua
bool LampControl::continuousMode = false;
unsigned long LampControl::lastMorseTime = 0;
int LampControl::currentMessageIndex = 0;
int LampControl::currentLetterIndex = 0;
int LampControl::currentSymbolIndex = 0;
bool LampControl::inPause = false;
unsigned long LampControl::pauseStartTime = 0;
int LampControl::pauseType = 0;
const char *LampControl::continuousMessage = "SOFIA E LINDA";
bool LampControl::lampOnForMorse = false;

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

    // Inicia a mensagem contínua em background
    Serial.println("Iniciando transmissão contínua da mensagem especial...");
    startContinuousMessage();
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

void LampControl::startContinuousMessage()
{
    continuousMode = true;
    currentMessageIndex = 0;
    currentLetterIndex = 0;
    currentSymbolIndex = 0;
    inPause = false;
    lastMorseTime = millis();
    lampOnForMorse = false;

    Serial.println("💝 Modo contínuo ATIVADO - Sofia é Linda em loop");
}

void LampControl::stopContinuousMessage()
{
    continuousMode = false;
    if (lampOnForMorse)
    {
        digitalWrite(LAMP_PIN, LOW);
        lampOnForMorse = false;
    }

    Serial.println("💝 Modo contínuo DESATIVADO");
}

bool LampControl::isContinuousModeActive()
{
    return continuousMode;
}

void LampControl::updateContinuousMessage()
{
    if (!continuousMode)
        return;

    unsigned long currentTime = millis();

    // Se estamos em pausa
    if (inPause)
    {
        unsigned long pauseDuration = 0;

        switch (pauseType)
        {
        case 0:
            pauseDuration = SYMBOL_PAUSE;
            break; // Entre símbolos
        case 1:
            pauseDuration = LETTER_PAUSE;
            break; // Entre letras
        case 2:
            pauseDuration = WORD_PAUSE;
            break; // Entre palavras
        case 3:
            pauseDuration = 1000;
            break; // Fim da mensagem (1 segundo) - era 3 segundos
        }

        if (currentTime - pauseStartTime >= pauseDuration)
        {
            inPause = false;

            // Se foi pausa de fim de mensagem, reinicia
            if (pauseType == 3)
            {
                currentMessageIndex = 0;
                currentLetterIndex = 0;
                currentSymbolIndex = 0;
                Serial.println("💝 Reiniciando mensagem contínua...");
            }
        }
        return;
    }

    // Se a lâmpada está acesa para Morse, verifica se deve apagar
    if (lampOnForMorse)
    {
        unsigned long duration = (currentTime - lastMorseTime);
        unsigned long expectedDuration = 0;

        // Determina duração esperada baseada no símbolo atual
        char currentChar = continuousMessage[currentMessageIndex];
        if (currentChar == ' ')
        {
            // Espaço - já tratado na lógica abaixo
            return;
        }
        else if (currentChar >= 'A' && currentChar <= 'Z')
        {
            const char *morse = morseTable[currentChar - 'A'];
            char symbol = morse[currentSymbolIndex];
            expectedDuration = (symbol == '.') ? DOT_DURATION : DASH_DURATION;
        }

        if (duration >= expectedDuration)
        {
            // Apaga a lâmpada
            digitalWrite(LAMP_PIN, LOW);
            lampOnForMorse = false;

            // Avança para próximo símbolo
            currentSymbolIndex++;

            // Inicia pausa entre símbolos
            inPause = true;
            pauseStartTime = currentTime;
            pauseType = 0; // Pausa entre símbolos
        }
        return;
    }

    // Processa próximo símbolo
    char currentChar = continuousMessage[currentMessageIndex];

    // Se chegou ao fim da mensagem
    if (currentChar == '\0')
    {
        inPause = true;
        pauseStartTime = currentTime;
        pauseType = 3; // Pausa de fim de mensagem
        return;
    }

    // Se é espaço (separador de palavras)
    if (currentChar == ' ')
    {
        currentMessageIndex++;
        currentLetterIndex = 0;
        currentSymbolIndex = 0;

        inPause = true;
        pauseStartTime = currentTime;
        pauseType = 2; // Pausa entre palavras
        return;
    }

    // Se é uma letra válida
    if (currentChar >= 'A' && currentChar <= 'Z')
    {
        const char *morse = morseTable[currentChar - 'A'];

        // Se chegou ao fim da letra atual
        if (morse[currentSymbolIndex] == '\0')
        {
            currentMessageIndex++;
            currentLetterIndex++;
            currentSymbolIndex = 0;

            inPause = true;
            pauseStartTime = currentTime;
            pauseType = 1; // Pausa entre letras
            return;
        }

        // Transmite o símbolo atual
        char symbol = morse[currentSymbolIndex];
        if (symbol == '.' || symbol == '-')
        {
            digitalWrite(LAMP_PIN, HIGH);
            lampOnForMorse = true;
            lastMorseTime = currentTime;

            // Debug: mostra o que está sendo transmitido
            if (currentSymbolIndex == 0)
            {
                Serial.printf("[%c: %s] ", currentChar, morse);
            }
            Serial.print(symbol);
        }
    }
    else
    {
        // Caractere inválido, pula
        currentMessageIndex++;
    }
}
