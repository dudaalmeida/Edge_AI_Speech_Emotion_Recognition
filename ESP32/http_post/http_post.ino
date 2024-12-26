#include <driver/i2s.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

// Definições do I2S
#define I2S_SD 25
#define I2S_WS 26
#define I2S_SCK 33
#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE (16000)
#define I2S_SAMPLE_BITS (16)
#define I2S_READ_LEN (16 * 1024)
#define RECORD_TIME (8) // Segundos
#define I2S_CHANNEL_NUM (1)
#define SEND_BUFFER_SIZE (I2S_READ_LEN)

// Definições do Display SSD1306
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET 4
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#define NUMFLAKES     10 // Number of snowflakes in the animation example
#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16
static const unsigned char PROGMEM logo_bmp[] =
{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

// Definições de Wi-Fi
const char* ssid = "NET_2GE9DC82";
const char* password = "DDE9DC82";
const char* serverUrl = "http://192.168.0.83:5000/upload";

HTTPClient http;

void setup() {
  Serial.begin(115200);


  // Inicializar o display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Erro ao inicializar o display SSD1306.");
    while (true);
  }

  display.display();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Iniciando...");
  display.display();

  // Conectar ao Wi-Fi
  connectWiFi();

  // Inicializar I2S
  i2sInit();

  // Criar tarefa de captura e envio de áudio
  xTaskCreate(i2s_adc_send, "i2s_adc_send", 4096, NULL, 1, NULL);
}

void loop() {
  // Não faz nada no loop principal
}

void connectWiFi() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Conectando ao Wi-Fi...");
  display.display();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Wi-Fi conectado!");
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.display();

  Serial.println("\nWi-Fi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void i2sInit() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = i2s_bits_per_sample_t(I2S_BITS_PER_SAMPLE_16BIT),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 1024,
    .use_apll = false
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_set_clk(I2S_PORT, I2S_SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
}

void getInferenceFromServer() {
  if (WiFi.status() == WL_CONNECTED) {
    String getUrl = "http://192.168.0.83:5000/infer";

    http.begin(getUrl);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.printf("Resposta do servidor (GET): %s\n", response.c_str());
      
      ParseJSON(httpResponseCode, response);
    } else {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Erro no GET!");
      display.display();
      Serial.printf("Erro ao realizar GET: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
  } else {
    display.clearDisplay();
    display.println("Wi-Fi desconectado!");
    display.display();
    Serial.println("Erro: Wi-Fi desconectado!");
  }
}

void i2s_adc_send(void *arg) {
  size_t bytes_read;
  char* i2s_read_buff = (char*)malloc(I2S_READ_LEN);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Gravando audio...");
  display.display();

  for (int i = 0; i < RECORD_TIME; i++) {
    i2s_read(I2S_PORT, (void*)i2s_read_buff, I2S_READ_LEN, &bytes_read, portMAX_DELAY);

    sendAudioToServer((uint8_t*)i2s_read_buff, bytes_read);
    //display.clearDisplay();
    //display.setCursor(0, 0);
    //display.printf("Enviado: %u bytes", bytes_read);
    //Serial.println("Enviado: %u bytes", bytes_read);
    //display.display();
  }

  sendEndFlag();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Envio concluido!");
  Serial.println("Envio concluido!");
  display.display();

  getInferenceFromServer();

  free(i2s_read_buff);
  vTaskDelete(NULL);
}

void ParseJSON(int httpResponseCode, String response){
  Serial.printf("Código HTTP: %d\n", httpResponseCode);
  Serial.println("Resposta do servidor:");
  Serial.println(response);

  // Parse do JSON
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    Serial.print("Erro ao analisar JSON: ");
    Serial.println(error.f_str());
    display.println("Erro JSON!");
  } else {
    // Exibir campos relevantes do JSON
    const char* status = doc["status"]; // Exemplo de chave "status"
    const char* message = doc["message"]; // Exemplo de chave "message"
        
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Servidor:");
    //display.println(status);
    Serial.printf("Status: %s, Message: %s\n", status, message);
    display.println(message);
    display.display();
  }
}

void sendAudioToServer(uint8_t* data, size_t length) {
  if (WiFi.status() == WL_CONNECTED) {
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/octet-stream");

    int httpResponseCode = http.POST(data, length);
    String response = http.getString();

    if (httpResponseCode > 0) {
      Serial.printf("Código HTTP: %d\n", httpResponseCode);
      Serial.println("Resposta do servidor:");
      Serial.println(response);
      
      ParseJSON(httpResponseCode, response);

    } else {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Erro ao enviar!");
      display.display();
      Serial.printf("Erro ao enviar: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
  } else {
    display.clearDisplay();
    display.println("Wi-Fi desconectado!");
    display.display();
    Serial.println("Erro: Wi-Fi desconectado!");
  }
}

void sendEndFlag() {
  if (WiFi.status() == WL_CONNECTED) {
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    String jsonPayload = "{\"end\": true}";
    int httpResponseCode = http.POST(jsonPayload);

    //display.clearDisplay();
    //display.setCursor(0, 0);

    if (httpResponseCode > 0) {
      //display.println("Sinal enviado!");
      //display.display();
      Serial.printf("Sinal enviado: %d\n", httpResponseCode);
    } else {
      display.println("Erro sinal!");
      display.display();
      Serial.printf("Erro ao enviar sinal: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
  } else {
    display.clearDisplay();
    display.println("Wi-Fi desconectado!");
    display.display();
    Serial.println("Erro: Wi-Fi desconectado!");
  }
}
