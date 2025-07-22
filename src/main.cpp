#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Definição de cor personalizada (roxo) para o display ST77XX
#define ST77XX_PURPLE 0x780F // RGB565: R=120, G=0, B=240 (ajuste conforme desejado)

// Configurações do Display ST7789 240x135 - baseado na imagem
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

// API OpenWeatherMap - Substitua pela sua chave API gratuita
const char* apiKey = "da1bf1f33a51a2bfa3ce1745bf65fd7f"; // Obtenha em: https://openweathermap.org/api
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

// Inicializa o display com as configurações corretas
Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);

// Função para calcular posição X para alinhamento à direita
int16_t calcularPosicaoX(const char* texto, uint8_t tamanho, int16_t margemDireita = 10) {
  int16_t x1, y1;
  uint16_t w, h;
  lcd.setTextSize(tamanho);
  lcd.getTextBounds(texto, 0, 0, &x1, &y1, &w, &h);
  return 240 - w - margemDireita; // 240 é a largura do display na rotação 3
}

// Função para desenhar ícones do tempo (simplificados)
void desenharIconeClima(int16_t x, int16_t y, String icone) {
  lcd.setTextColor(ST77XX_WHITE);
  
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
    lcd.fillCircle(x + 8, y + 18, 6, ST77XX_BLUE);
    lcd.fillCircle(x + 15, y + 15, 8, ST77XX_BLUE);
    lcd.fillCircle(x + 22, y + 18, 6, ST77XX_BLUE);
    lcd.fillRect(x + 8, y + 18, 14, 8, ST77XX_BLUE);
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
    // Nuvem escura
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

// Função para obter dados do clima - versão corrigida
bool obterDadosClima() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi não conectado");
    return false;
  }
  
  Serial.println("Iniciando requisição HTTP...");
  Serial.println("URL: " + String(apiURL));
  
  HTTPClient http;
  http.begin(apiURL);
  http.setTimeout(10000); // Timeout de 10 segundos
  
  int httpResponseCode = http.GET();
  Serial.printf("Código de resposta HTTP: %d\n", httpResponseCode);
  
  if (httpResponseCode > 0) {
    String payload = http.getString();
    Serial.println("Resposta da API:");
    Serial.println(payload);
    
    // Verifica se recebeu dados válidos
    if (payload.length() < 10) {
      Serial.println("Resposta muito pequena");
      http.end();
      return false;
    }
    
    // Usando JsonDocument em vez de DynamicJsonDocument (corrigindo deprecação)
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
      Serial.print("Erro ao analisar JSON: ");
      Serial.println(error.c_str());
      http.end();
      return false;
    }
    
    // Verifica se os dados existem antes de extrair
    if (doc.containsKey("main") && doc["main"].containsKey("temp")) {
      temperatura = doc["main"]["temp"];
      umidade = doc["main"]["humidity"];
      
      if (doc.containsKey("weather") && doc["weather"].size() > 0) {
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
      if (doc.containsKey("message")) {
        Serial.println("Mensagem de erro da API: " + doc["message"].as<String>());
      }
      
      http.end();
      return false;
    }
  } else {
    Serial.printf("Erro na requisição HTTP: %d\n", httpResponseCode);
    
    // Diagnóstico adicional para erros HTTP
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

void setup() {
  Serial.begin(115200); // Mudando para 115200 para melhor performance
  delay(1000);
  
  Serial.println("Iniciando display...");
  
  // Configura o pino do backlight
  pinMode(LCD_BLK, OUTPUT);
  digitalWrite(LCD_BLK, HIGH); // Liga o backlight
  
  // Inicializa o display
  lcd.init(135, 240);
  lcd.fillScreen(ST77XX_BLACK);
  Serial.println("Display inicializado");
  
  // Define rotação final
  lcd.setRotation(3);
  lcd.fillScreen(ST77XX_BLACK);
  
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
        delay(2000); // Aguarda antes de tentar novamente
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
      
      // Define valores padrão para evitar exibição de zeros
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
  lcd.fillScreen(ST77XX_BLACK);
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

void loop() {
  // Verifica se o WiFi ainda está conectado
  if (WiFi.status() != WL_CONNECTED) {
    lcd.fillScreen(ST77XX_BLACK);
    lcd.setTextColor(ST77XX_RED);
    
    char errorMsg[] = "WiFi desconectado!";
    int16_t x = calcularPosicaoX(errorMsg, 1);
    lcd.setCursor(x, 30);
    lcd.println(errorMsg);
    
    delay(1000);
    return;
  }
  
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Falha ao obter o tempo");
    lcd.fillScreen(ST77XX_BLACK);
    lcd.setTextColor(ST77XX_RED);
    
    char timeError[] = "Erro no tempo!";
    int16_t x = calcularPosicaoX(timeError, 1);
    lcd.setCursor(x, 50);
    lcd.println(timeError);
    
    delay(1000);
    return;
  }
  
  // Atualiza dados do clima a cada 10 minutos
  if (millis() - ultimaAtualizacao > intervaloAtualizacao) {
    Serial.println("Atualizando dados do clima...");
    obterDadosClima();
  }
  
  // Limpa a tela
  lcd.fillScreen(ST77XX_BLACK);
  
  // Prepara strings para exibição - usando string literal para grau (Opção 1)
  char timeStr[10];
  char dateStr[12];
  char tempStr[20];
  char umidadeStr[15];
  
  sprintf(timeStr, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  sprintf(dateStr, "%02d/%02d/%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
  // Opção 1: String literal com símbolo de grau
  sprintf(tempStr, "%.1f%cC", temperatura, 248); // ASCII 248 para °
  sprintf(umidadeStr, "%.0f%%", umidade);
  
  // Exibe a hora (alinhada à direita)
  lcd.setTextColor(ST77XX_CYAN);
  lcd.setTextSize(3);
  int16_t xTime = calcularPosicaoX(timeStr, 3);
  lcd.setCursor(xTime, 5);
  lcd.print(timeStr);
  
  // Exibe a data (alinhada à direita)
  lcd.setTextColor(ST77XX_YELLOW);
  lcd.setTextSize(2);
  int16_t xDate = calcularPosicaoX(dateStr, 2);
  lcd.setCursor(xDate, 35);
  lcd.print(dateStr);
  
  // Exibe ícone do clima (lado esquerdo)
  desenharIconeClima(10, 65, iconeClima);
  
  // Exibe temperatura (alinhada à direita)
  lcd.setTextColor(ST77XX_GREEN);
  lcd.setTextSize(2);
  int16_t xTemp = calcularPosicaoX(tempStr, 2);
  lcd.setCursor(xTemp, 70);
  lcd.print(tempStr);
  
  // Exibe ícone de umidade (lado esquerdo, abaixo do ícone do clima)
  desenharIconeUmidade(10, 95);
  
  // Exibe umidade (alinhada à direita)
  lcd.setTextColor(ST77XX_BLUE);
  lcd.setTextSize(1);
  int16_t xUmid = calcularPosicaoX(umidadeStr, 1);
  lcd.setCursor(xUmid, 95);
  lcd.print(umidadeStr);
  
  // Exibe cidade (alinhado à direita)
  char cidadeStr[] = "Pouso Alegre/MG";
  lcd.setTextColor(ST77XX_WHITE);
  lcd.setTextSize(1);
  int16_t xCidade = calcularPosicaoX(cidadeStr, 1);
  lcd.setCursor(xCidade, 110);
  lcd.print(cidadeStr);
  
  // Debug no Serial
  Serial.printf("Hora: %s | Temp: %.1f°C | Umidade: %.0f%% | %s\n", 
                timeStr, temperatura, umidade, descricaoClima.c_str());
  
  delay(1000);
}