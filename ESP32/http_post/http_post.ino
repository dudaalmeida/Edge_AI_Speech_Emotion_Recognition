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
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 4
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Definições do botão
#define BUTTON_PIN 2
#define DEBOUNCE_DELAY 50 // 50 ms para debouncing

// Variáveis globais
bool isRecording = false;
unsigned long lastDebounceTime = 0;
bool buttonState = LOW;
bool lastButtonState = LOW;

// Wi-Fi
const char* ssid = "NET_2GE9DC82";
const char* password = "DDE9DC82";
const char* serverUrl = "http://192.168.0.83:5000/upload";
HTTPClient http;

// Prototipação
void connectWiFi();
void i2sInit();
void i2s_adc_send(void *arg);
void checkButton(void *arg);
void sendAudioToServer(uint8_t* data, size_t length);
void sendEndFlag();

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT);

  // Inicializar o display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Erro ao inicializar o display SSD1306.");
    while (true);
  }

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

  // Criar tarefa para monitorar o botão
  xTaskCreate(checkButton, "Check Button", 2048, NULL, 1, NULL);
}

void loop() {
  // O loop principal fica vazio, as tasks lidam com o comportamento.
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

void i2s_adc_send(void *arg) {
  size_t bytes_read;
  char* i2s_read_buff = (char*)malloc(I2S_READ_LEN);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Gravando audio...");
  display.display();

  while (isRecording) {
    i2s_read(I2S_PORT, (void*)i2s_read_buff, I2S_READ_LEN, &bytes_read, portMAX_DELAY);
    sendAudioToServer((uint8_t*)i2s_read_buff, bytes_read);
  }

  sendEndFlag();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Envio concluído!");
  display.display();

  free(i2s_read_buff);
  vTaskDelete(NULL);
}

void checkButton(void *arg) {
  while (true) {
    int reading = digitalRead(BUTTON_PIN);
    if (reading != lastButtonState) {
      lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
      if (reading != buttonState) {
        buttonState = reading;

        if (buttonState == HIGH) {
          isRecording = !isRecording;
          if (isRecording) {
            xTaskCreate(i2s_adc_send, "i2s_adc_send", 4096, NULL, 1, NULL);
          }
        }
      }
    }

    lastButtonState = reading;
    vTaskDelay(pdMS_TO_TICKS(10)); // Pequeno delay para evitar saturação da CPU
  }
}

void sendAudioToServer(uint8_t* data, size_t length) {
  if (WiFi.status() == WL_CONNECTED) {
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/octet-stream");
    int httpResponseCode = http.POST(data, length);
    http.end();

    if (httpResponseCode <= 0) {
      Serial.printf("Erro ao enviar: %s\n", http.errorToString(httpResponseCode).c_str());
    }
  }
}

void sendEndFlag() {
  if (WiFi.status() == WL_CONNECTED) {
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");
    String jsonPayload = "{\"end\": true}";
    http.POST(jsonPayload);
    http.end();
  }
}
