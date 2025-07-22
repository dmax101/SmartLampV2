#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

// Configurações do Display ST7789 240x135
#define LCD_MOSI  23  // ESP32 D23
#define LCD_SCLK  18  // ESP32 D18
#define LCD_CS    15  // ESP32 D15
#define LCD_DC     2  // ESP32 D2
#define LCD_RST    4  // ESP32 D4
#define LCD_BLK   32  // ESP32 D32 (backlight)

// Configurações WiFi
const char* ssid = "VIVOFIBRA-DD21";
const char* password = "hswVP74823";

// Configurações de tempo (NTP)
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -3 * 3600; // GMT-3 (Brasil)
const int daylightOffset_sec = 0;

// API OpenWeatherMap
const char* apiKey = "da1bf1f33a51a2bfa3ce1745bf65fd7f";
const char* cidade = "Pouso%20Alegre";
const char* pais = "BR";
String apiURL = "http://api.openweathermap.org/data/2.5/weather?q=Pouso%20Alegre,BR&appid=" + String(apiKey) + "&units=metric&lang=pt";

// Variáveis para dados do clima
float temperatura = 0.0;
float umidade = 0.0;
String descricaoClima = "";
String iconeClima = "";
unsigned long ultimaAtualizacao = 0;
const unsigned long intervaloAtualizacao = 600000; // 10 minutos

// Variáveis para controle de atualização seletiva
String lastTimeStr = "";
String lastDateStr = "";
String lastTempStr = "";
String lastUmidadeStr = "";
bool forceFullUpdate = true;
unsigned long lastDisplayUpdate = 0;
const unsigned long displayUpdateInterval = 1000; // Atualiza a cada segundo para verificar mudança de minuto
int lastMinute = -1; // Para detectar mudança de minuto

// Inicializa o display
Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);

// Declaração de funções (protótipos)
void displayBMP(uint8_t* bmpData, int dataSize);
void drawInterfaceElements();
void desenharIconeUmidade(int16_t x, int16_t y);
void desenharIconeClima(int16_t x, int16_t y, String icone);
bool loadWallpaperFromFile();
void displayWallpaperWithOverlay();
int base64_decode(const char* input, uint8_t* output, int outputLen);
bool obterDadosClima();
void clearArea(int16_t x, int16_t y, uint16_t w, uint16_t h);

// Definições de cores personalizadas (adicione após as inclusões)
#define ST77XX_PURPLE   0x780F  // RGB565: Purple
#define ST77XX_ORANGE   0xFD20  // RGB565: Orange
#define ST77XX_PINK     0xF81F  // RGB565: Pink
#define ST77XX_BROWN    0x9A60  // RGB565: Brown

// Função para calcular posição X para alinhamento à direita
int16_t calcularPosicaoX(const char* texto, uint8_t tamanho, int16_t margemDireita = 10) {
  int16_t x1, y1;
  uint16_t w, h;
  lcd.setTextSize(tamanho);
  lcd.getTextBounds(texto, 0, 0, &x1, &y1, &w, &h);
  return 240 - w - margemDireita;
}

// Função para desenhar ícone de umidade
void desenharIconeUmidade(int16_t x, int16_t y) {
  // Desenha uma gota d'água
  lcd.fillCircle(x + 5, y + 8, 3, ST77XX_BLUE);
  lcd.fillTriangle(x + 5, y + 2, x + 2, y + 8, x + 8, y + 8, ST77XX_BLUE);
  
  // Adiciona brilho na gota
  lcd.drawPixel(x + 4, y + 6, ST77XX_CYAN);
  lcd.drawPixel(x + 3, y + 7, ST77XX_CYAN);
}

// Função para desenhar ícones do tempo - versão corrigida
void desenharIconeClima(int16_t x, int16_t y, String icone) {
  if (icone == "01d" || icone == "01n") { // Sol
    lcd.fillCircle(x + 15, y + 15, 8, ST77XX_YELLOW);
    // Raios do sol
    for (int i = 0; i < 8; i++) {
      float angle = i * PI / 4;
      int x1 = x + 15 + cos(angle) * 12;
      int y1 = y + 15 + sin(angle) * 12;
      int x2 = x + 15 + cos(angle) * 15;
      int y2 = y + 15 + sin(angle) * 15;
      lcd.drawLine(x1, y1, x2, y2, ST77XX_YELLOW);
    }
  } 
  else if (icone == "02d" || icone == "02n" || icone == "03d" || icone == "03n") { // Nuvens
    lcd.fillCircle(x + 8, y + 18, 6, ST77XX_WHITE);
    lcd.fillCircle(x + 15, y + 15, 8, ST77XX_WHITE);
    lcd.fillCircle(x + 22, y + 18, 6, ST77XX_WHITE);
    lcd.fillRect(x + 8, y + 18, 14, 8, ST77XX_WHITE);
  }
  else if (icone == "04d" || icone == "04n") { // Nuvens escuras
    lcd.fillCircle(x + 8, y + 18, 6, 0x39C7); // Cinza escuro
    lcd.fillCircle(x + 15, y + 15, 8, 0x39C7);
    lcd.fillCircle(x + 22, y + 18, 6, 0x39C7);
    lcd.fillRect(x + 8, y + 18, 14, 8, 0x39C7);
  }
  else if (icone.startsWith("09") || icone.startsWith("10")) { // Chuva
    // Nuvem
    lcd.fillCircle(x + 8, y + 15, 5, ST77XX_BLUE);
    lcd.fillCircle(x + 15, y + 12, 7, ST77XX_BLUE);
    lcd.fillCircle(x + 22, y + 15, 5, ST77XX_BLUE);
    lcd.fillRect(x + 8, y + 15, 14, 6, ST77XX_BLUE);
    // Gotas de chuva
    for (int i = 0; i < 5; i++) {
      lcd.drawLine(x + 10 + i * 3, y + 22, x + 8 + i * 3, y + 28, ST77XX_CYAN);
    }
  }
  else if (icone.startsWith("11")) { // Tempestade
    // Nuvem escura - usando cor roxa personalizada
    lcd.fillCircle(x + 8, y + 15, 5, ST77XX_PURPLE);
    lcd.fillCircle(x + 15, y + 12, 7, ST77XX_PURPLE);
    lcd.fillCircle(x + 22, y + 15, 5, ST77XX_PURPLE);
    lcd.fillRect(x + 8, y + 15, 14, 6, ST77XX_PURPLE);
    // Raio
    lcd.drawLine(x + 15, y + 20, x + 12, y + 25, ST77XX_YELLOW);
    lcd.drawLine(x + 12, y + 25, x + 18, y + 30, ST77XX_YELLOW);
  }
  else if (icone.startsWith("13")) { // Neve
    // Nuvem
    lcd.fillCircle(x + 8, y + 15, 5, ST77XX_WHITE);
    lcd.fillCircle(x + 15, y + 12, 7, ST77XX_WHITE);
    lcd.fillCircle(x + 22, y + 15, 5, ST77XX_WHITE);
    lcd.fillRect(x + 8, y + 15, 14, 6, ST77XX_WHITE);
    // Flocos de neve
    for (int i = 0; i < 4; i++) {
      int px = x + 10 + i * 4;
      int py = y + 25 + (i % 2) * 3;
      lcd.drawPixel(px, py, ST77XX_WHITE);
      lcd.drawPixel(px - 1, py, ST77XX_WHITE);
      lcd.drawPixel(px + 1, py, ST77XX_WHITE);
      lcd.drawPixel(px, py - 1, ST77XX_WHITE);
      lcd.drawPixel(px, py + 1, ST77XX_WHITE);
    }
  }
  else { // Padrão - nuvem
    lcd.fillCircle(x + 8, y + 18, 6, ST77XX_WHITE);
    lcd.fillCircle(x + 15, y + 15, 8, ST77XX_WHITE);
    lcd.fillCircle(x + 22, y + 18, 6, ST77XX_WHITE);
    lcd.fillRect(x + 8, y + 18, 14, 8, ST77XX_WHITE);
  }
}

// Função para obter dados do clima - corrigindo warnings de depreciação
bool obterDadosClima() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi não conectado");
    return false;
  }
  
  Serial.println("Iniciando requisição HTTP...");
  Serial.println("URL: " + String(apiURL));
  
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
    
    // Corrigindo warnings de depreciação - usando nova sintaxe
    if (doc["main"].is<JsonObject>() && doc["main"]["temp"].is<float>()) {
      temperatura = doc["main"]["temp"];
      umidade = doc["main"]["humidity"];
      
      if (doc["weather"].is<JsonArray>() && doc["weather"].size() > 0) {
        descricaoClima = doc["weather"][0]["description"].as<String>();
        iconeClima = doc["weather"][0]["icon"].as<String>();
      }
      
      Serial.printf("Dados extraídos com sucesso:\n");
      Serial.printf("Temperatura: %.1f°C\n", temperatura);
      Serial.printf("Umidade: %.0f%%\n", umidade);
      Serial.printf("Descrição: %s\n", descricaoClima.c_str());
      Serial.printf("Ícone: %s\n", iconeClima.c_str());
      
      ultimaAtualizacao = millis();
      http.end();
      return true;
    } else {
      Serial.println("Dados de temperatura não encontrados no JSON");
      
      // Verifica se há erro na resposta da API
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

// Função para decodificar base64
int base64_decode(const char* input, uint8_t* output, int outputLen) {
  const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int inputLen = strlen(input);
  int outputIndex = 0;
  
  for (int i = 0; i < inputLen; i += 4) {
    if (outputIndex >= outputLen - 3) break;
    
    uint32_t value = 0;
    for (int j = 0; j < 4; j++) {
      if (i + j < inputLen) {
        char c = input[i + j];
        if (c == '=') break;
        
        const char* pos = strchr(chars, c);
        if (pos) {
          value = (value << 6) | (pos - chars);
        }
      }
    }
    
    output[outputIndex++] = (value >> 16) & 0xFF;
    output[outputIndex++] = (value >> 8) & 0xFF;
    output[outputIndex++] = value & 0xFF;
  }
  
  return outputIndex;
}

// Função para exibir imagem BMP
void displayBMP(uint8_t* bmpData, int dataSize) {
  // Verifica se é um arquivo BMP válido
  if (dataSize < 54 || bmpData[0] != 'B' || bmpData[1] != 'M') {
    Serial.println("Arquivo BMP inválido");
    return;
  }
  
  // Lê header BMP
  uint32_t imageOffset = *(uint32_t*)(bmpData + 10);
  uint32_t width = *(uint32_t*)(bmpData + 18);
  uint32_t height = *(uint32_t*)(bmpData + 22);
  uint16_t bitsPerPixel = *(uint16_t*)(bmpData + 28);
  
  Serial.printf("BMP Info: %dx%d, %d bits\n", width, height, bitsPerPixel);
  
  // Suporte apenas para 24-bit BMP
  if (bitsPerPixel != 24) {
    Serial.println("Suporte apenas para BMP 24-bit");
    return;
  }
  
  // Redimensiona para caber no display (240x135)
  float scaleX = (float)240 / width;
  float scaleY = (float)135 / height;
  float scale = min(scaleX, scaleY);
  
  int newWidth = (int)(width * scale);
  int newHeight = (int)(height * scale);
  
  // Centraliza a imagem
  int offsetX = (240 - newWidth) / 2;
  int offsetY = (135 - newHeight) / 2;
  
  // Limpa o display
  lcd.fillScreen(ST77XX_BLACK);
  
  Serial.println("Desenhando imagem...");
  
  // Desenha a imagem pixel por pixel
  for (int y = 0; y < newHeight; y++) {
    for (int x = 0; x < newWidth; x++) {
      // Mapeia coordenadas redimensionadas para originais
      int origX = (int)(x / scale);
      int origY = (int)(y / scale);
      
      // BMP é armazenado de baixo para cima
      int bmpY = height - 1 - origY;
      
      // Calcula posição no buffer (3 bytes por pixel)
      int rowSize = ((width * 3 + 3) & ~3); // Alinhamento de 4 bytes
      int pixelOffset = imageOffset + (bmpY * rowSize) + (origX * 3);
      
      if (pixelOffset + 2 < dataSize) {
        // Lê pixels BGR (BMP format)
        uint8_t b = bmpData[pixelOffset];
        uint8_t g = bmpData[pixelOffset + 1];
        uint8_t r = bmpData[pixelOffset + 2];
        
        // Converte para RGB565
        uint16_t color = lcd.color565(r, g, b);
        
        // Desenha pixel no display
        lcd.drawPixel(offsetX + x, offsetY + y, color);
      }
    }
    
    // Atualiza progresso a cada 10 linhas
    if (y % 10 == 0) {
      Serial.printf("Progresso: %d%%\n", (y * 100) / newHeight);
    }
  }
  
  Serial.println("Imagem carregada com sucesso!");
}

// Função para ler o arquivo base64 e decodificar
bool loadWallpaperFromFile() {
  if (!SPIFFS.begin()) {
    Serial.println("Erro ao inicializar SPIFFS");
    return false;
  }
  
  File file = SPIFFS.open("/img.txt", "r");
  if (!file) {
    Serial.println("Erro ao abrir arquivo img.txt");
    return false;
  }
  
  String base64Data = file.readString();
  file.close();
  
  // Remove o prefixo "data:image/bmp;base64," se presente
  int commaIndex = base64Data.indexOf(',');
  if (commaIndex != -1) {
    base64Data = base64Data.substring(commaIndex + 1);
  }
  
  // Remove quebras de linha e espaços
  base64Data.replace("\n", "");
  base64Data.replace("\r", "");
  base64Data.replace(" ", "");
  
  Serial.println("Decodificando imagem...");
  
  // Buffer para dados decodificados
  const int bufferSize = 8192;
  uint8_t* imageBuffer = (uint8_t*)malloc(bufferSize);
  if (!imageBuffer) {
    Serial.println("Erro ao alocar memória");
    return false;
  }
  
  int decodedSize = base64_decode(base64Data.c_str(), imageBuffer, bufferSize);
  
  if (decodedSize > 0) {
    displayBMP(imageBuffer, decodedSize);
  }
  
  free(imageBuffer);
  return decodedSize > 0;
}

// Função para exibir papel de parede com transparência
void displayWallpaperWithOverlay() {
  // Primeiro desenha o papel de parede
  loadWallpaperFromFile();
  
  // Aguarda um pouco para visualizar
  delay(2000);
  
  // Agora desenha os elementos da interface
  drawInterfaceElements();
}

// Função para desenhar elementos da interface sobre o papel de parede
void drawInterfaceElements() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Falha ao obter o tempo");
    return;
  }
  
  // Prepara strings para exibição (sem segundos)
  char timeStr[8];  // Reduzido para HH:MM
  char dateStr[12];
  char tempStr[20];
  char umidadeStr[15];
  
  sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min); // Removido segundos
  sprintf(dateStr, "%02d/%02d/%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
  sprintf(tempStr, "%.1f°C", temperatura);
  sprintf(umidadeStr, "%.0f%%", umidade);
  
  // Desenha retângulos com fundo semi-transparente para melhor legibilidade
  
  // Área da hora
  int xTime = calcularPosicaoX(timeStr, 3);
  lcd.fillRect(xTime - 5, 0, 240 - xTime + 5, 30, ST77XX_BLACK);
  lcd.setTextColor(ST77XX_CYAN);
  lcd.setTextSize(3);
  lcd.setCursor(xTime, 5);
  lcd.print(timeStr);
  
  // Área da data
  int xDate = calcularPosicaoX(dateStr, 2);
  lcd.fillRect(xDate - 5, 30, 240 - xDate + 5, 25, ST77XX_BLACK);
  lcd.setTextColor(ST77XX_YELLOW);
  lcd.setTextSize(2);
  lcd.setCursor(xDate, 35);
  lcd.print(dateStr);
  
  // Área do clima
  lcd.fillRect(0, 60, 240, 35, ST77XX_BLACK);
  
  // Ícone do clima
  desenharIconeClima(10, 65, iconeClima);
  
  // Temperatura
  int xTemp = calcularPosicaoX(tempStr, 2);
  lcd.setTextColor(ST77XX_GREEN);
  lcd.setTextSize(2);
  lcd.setCursor(xTemp, 70);
  lcd.print(tempStr);
  
  // Área da umidade
  lcd.fillRect(0, 90, 240, 20, ST77XX_BLACK);
  
  // Ícone de umidade
  desenharIconeUmidade(10, 95);
  
  // Umidade
  int xUmid = calcularPosicaoX(umidadeStr, 1);
  lcd.setTextColor(ST77XX_BLUE);
  lcd.setTextSize(1);
  lcd.setCursor(xUmid, 95);
  lcd.print(umidadeStr);
  
  // Área da cidade
  lcd.fillRect(0, 105, 240, 15, ST77XX_BLACK);
  char cidadeStr[] = "Pouso Alegre/MG";
  lcd.setTextColor(ST77XX_WHITE);
  lcd.setTextSize(1);
  int xCidade = calcularPosicaoX(cidadeStr, 1);
  lcd.setCursor(xCidade, 110);
  lcd.print(cidadeStr);
}

// Função para limpar uma área específica do display
void clearArea(int16_t x, int16_t y, uint16_t w, uint16_t h) {
  lcd.fillRect(x, y, w, h, ST77XX_BLACK);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Iniciando display...");
  
  // Configura o pino do backlight
  pinMode(LCD_BLK, OUTPUT);
  digitalWrite(LCD_BLK, HIGH);
  
  // Inicializa SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("Erro ao inicializar SPIFFS");
  }
  
  // Inicializa o display
  lcd.init(135, 240);
  lcd.fillScreen(ST77XX_BLACK);
  Serial.println("Display inicializado");
  
  // Define rotação final
  lcd.setRotation(3);
  
  Serial.println("Conectando ao WiFi...");
  
  // Conecta ao WiFi
  WiFi.begin(ssid, password);
  lcd.setTextColor(ST77XX_YELLOW);
  lcd.setTextSize(1);
  lcd.setCursor(10, 10);
  lcd.println("Conectando WiFi...");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    lcd.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    
    lcd.setTextColor(ST77XX_GREEN);
    lcd.setCursor(10, 30);
    lcd.println("WiFi OK!");
    
    // Configura o tempo via NTP
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    
    // Aguarda sincronização do tempo
    Serial.println("Aguardando sincronização do tempo...");
    delay(3000);
    
    // Obtém dados iniciais do clima
    Serial.println("Obtendo dados do clima...");
    lcd.setCursor(10, 50);
    lcd.println("Carregando clima...");
    
    // Tenta obter dados do clima várias vezes
    bool climaOK = false;
    for (int i = 0; i < 3 && !climaOK; i++) {
      Serial.printf("Tentativa %d de obter dados do clima...\n", i + 1);
      climaOK = obterDadosClima();
      if (!climaOK) {
        delay(2000);
      }
    }
    
    if (climaOK) {
      lcd.setTextColor(ST77XX_GREEN);
      lcd.setCursor(10, 70);
      lcd.println("Clima OK!");
    } else {
      lcd.setTextColor(ST77XX_RED);
      lcd.setCursor(10, 70);
      lcd.println("Erro no clima!");
      
      // Define valores padrão
      temperatura = 25.0;
      umidade = 60.0;
      descricaoClima = "Sem dados";
      iconeClima = "01d";
    }
    
  } else {
    Serial.println("Falha na conexão WiFi!");
    lcd.fillScreen(ST77XX_BLACK);
    lcd.setTextColor(ST77XX_RED);
    lcd.setCursor(10, 10);
    lcd.println("WiFi ERRO!");
  }
  
  delay(2000);
  
  // Carrega papel de parede
  Serial.println("Carregando papel de parede...");
  displayWallpaperWithOverlay();
  
  forceFullUpdate = true;
}

unsigned long frameCount = 0;
unsigned long lastFPSCheck = 0;

void loop() {
  // Controla a frequência de atualização do display
  if (millis() - lastDisplayUpdate < displayUpdateInterval) {
    delay(10);
    return;
  }
  lastDisplayUpdate = millis();
  
  // Verifica se o WiFi ainda está conectado
  if (WiFi.status() != WL_CONNECTED) {
    if (forceFullUpdate) {
      lcd.fillScreen(ST77XX_BLACK); // Limpa a tela
      lcd.setTextColor(ST77XX_RED);
      
      char errorMsg[] = "WiFi desconectado!";
      int16_t x = calcularPosicaoX(errorMsg, 1);
      lcd.setCursor(x, 30);
      lcd.println(errorMsg);
      forceFullUpdate = false;
    }
    delay(1000);
    return;
  }
  
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Falha ao obter o tempo");
    if (forceFullUpdate) {
      lcd.fillScreen(ST77XX_BLACK); // Limpa a tela
      lcd.setTextColor(ST77XX_RED);
      
      char timeError[] = "Erro no tempo!";
      int16_t x = calcularPosicaoX(timeError, 1);
      lcd.setCursor(x, 50);
      lcd.println(timeError);
      forceFullUpdate = false;
    }
    delay(1000);
    return;
  }
  
  // Limpa a tela após verificações de conexão bem-sucedidas (uma vez)
  static bool screenCleared = false;
  if (!screenCleared && WiFi.status() == WL_CONNECTED) {
    // Carrega papel de parede novamente após conexão
    Serial.println("Reconectado - carregando papel de parede...");
    displayWallpaperWithOverlay();
    screenCleared = true;
    forceFullUpdate = true;
  }
  
  // Detecta mudança de minuto para atualizar apenas quando necessário
  bool minuteChanged = (timeinfo.tm_min != lastMinute);
  if (minuteChanged) {
    lastMinute = timeinfo.tm_min;
    forceFullUpdate = true;
    Serial.printf("Minuto mudou para: %02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min);
  }
  
  // Atualiza dados do clima a cada 10 minutos
  if (millis() - ultimaAtualizacao > intervaloAtualizacao) {
    Serial.println("Atualizando dados do clima...");
    bool climaAtualizado = obterDadosClima();
    if (climaAtualizado) {
      forceFullUpdate = true;
    }
  }
  
  // Só redesenha a interface se o minuto mudou ou foi forçada atualização
  if (forceFullUpdate || minuteChanged) {
    drawInterfaceElements();
    forceFullUpdate = false;
    
    // Debug quando atualiza
    Serial.printf("Display atualizado - Hora: %02d:%02d | Temp: %.1f°C | Umidade: %.0f%%\n", 
                  timeinfo.tm_hour, timeinfo.tm_min, temperatura, umidade);
  }
  
  // Debug menos frequente para não sobrecarregar
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 30000) { // Debug a cada 30 segundos
    Serial.printf("Status - Hora: %02d:%02d | Temp: %.1f°C | Umidade: %.0f%% | %s\n", 
                  timeinfo.tm_hour, timeinfo.tm_min,
                  temperatura, umidade, descricaoClima.c_str());
    lastDebug = millis();
  }
}