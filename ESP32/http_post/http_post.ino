#include <WiFi.h>
#include <HTTPClient.h>
#include <driver/i2s.h>

const char* ssid = "NET_2GE9DC82";
const char* password = "DDE9DC82";
const char* serverURL = "http://192.168.0.83:5000/";  // URL do seu servidor Flask

WiFiClient client;

#define I2S_SD 25
#define I2S_WS 26
#define I2S_SCK 33
#define I2S_PORT I2S_NUM_0


#define BUFFER_CNT 10
#define BUFFER_LEN 1024
#define MAX_BUFFER_SIZE 38000
//int16_t sBuffer[BUFFER_LEN];

#define BUTTON_PIN 2  // Pino do botão (alterar conforme necessidade)
#define DEBOUNCE_DELAY 50  // Tempo de debounce em milissegundos
#define MAX_RECORD_TIME 2000  // Tempo máximo de gravação (2 segundos)

volatile bool recording = false;  // Estado de gravação (iniciado ou não)
unsigned long recordStartTime = 0;  // Tempo de início da gravação

int16_t accumulatedBuffer[MAX_BUFFER_SIZE];  // Buffer global para acumular os dados de áudio
int bufferIndex = 0;  // Índice para armazenar os dados no buffer

void i2s_install() {
  // Set up I2S Processor configuration
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 44100,
    //.sample_rate = 16000,
    .bits_per_sample = i2s_bits_per_sample_t(16),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0,
    .dma_buf_count = BUFFER_CNT,
    .dma_buf_len = BUFFER_LEN,
    .use_apll = false
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}

void i2s_setpin() {
  // Set I2S pin configuration
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };

  i2s_set_pin(I2S_PORT, &pin_config);
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  // Configuração do I2S para capturar os dados de áudio
  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if (xTaskCreatePinnedToCore(micTask, "micTask", 10000, NULL, 5, NULL, 1) == pdPASS) {
    Serial.println("micTask criada com sucesso");
  }
    if (xTaskCreatePinnedToCore(buttonTask, "buttonTask", 2048, NULL, 5, NULL, 1) == pdPASS) {
    Serial.println("buttonTask criada com sucesso");
  }
}

void loop(){}

void micTask(void* parameter) {
  size_t bytesIn = 0;
  int16_t buffer[BUFFER_LEN];
  while (1) {
    if (recording) {
      esp_err_t result = i2s_read(I2S_PORT, &buffer, BUFFER_LEN, &bytesIn, portMAX_DELAY);
      if (result == ESP_OK) {
        accumulateAudioData(buffer, bytesIn);
      }

      // Verifica se o tempo máximo de gravação foi atingido
      if (millis() - recordStartTime >= MAX_RECORD_TIME) {
        stopRecording();
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void buttonTask(void* parameter) {
  static unsigned long lastButtonPress = 0;
  static bool lastButtonState = HIGH;  // Estado anterior do botão

  while (1) {
    bool currentButtonState = digitalRead(BUTTON_PIN);
    if (lastButtonState == HIGH && currentButtonState == LOW && (millis() - lastButtonPress) > DEBOUNCE_DELAY) {
      lastButtonPress = millis();
      if (recording) {
        stopRecording();
      } else {
        startRecording();
      }
    }
    lastButtonState = currentButtonState;
    vTaskDelay(10 / portTICK_PERIOD_MS);  // Verificar o botão a cada 10 ms
  }
}

void startRecording() {
  recording = true;
  recordStartTime = millis();
  Serial.println("Iniciando gravação...");
}

void stopRecording() {
  recording = false;
  Serial.println("Gravação interrompida.");
  sendAudioData();
  clearBuffer();
}

// Função para acumular dados de áudio no buffer
void accumulateAudioData(int16_t* data, size_t size) {
  // Verifica se há espaço suficiente no buffer para os novos dados
  if (bufferIndex + size <= MAX_BUFFER_SIZE) {
    // Copia os dados lidos para o buffer
    memcpy(&accumulatedBuffer[bufferIndex], data, size * sizeof(int16_t));
    bufferIndex += size;  // Atualiza o índice para a próxima posição
  } else {
    // Se o buffer estiver cheio, envia os dados e reinicia o buffer
    sendAudioData();
    clearBuffer();
    // Agora, começa a acumular os novos dados
    memcpy(&accumulatedBuffer[0], data, size * sizeof(int16_t));
    bufferIndex = size;  // Atualiza o índice
  }
}

void sendAudioData() {
  HTTPClient http;
  http.begin(serverURL);  // URL do servidor Flask
  http.addHeader("Content-Type", "application/octet-stream");

  // Enviar os dados de áudio acumulados
  int httpResponseCode = http.POST((uint8_t*)accumulatedBuffer, sizeof(accumulatedBuffer));
  //Serial.println("Audio: " + accumulatedBuffer[0]);

  if (httpResponseCode > 0) {
    String payload = http.getString();
    Serial.println("Server Response: " + payload);
  } else {
    Serial.println("Erro ao enviar dados");
  }

  http.end();  // Fecha a requisição
}

// Função para limpar o buffer após o envio
void clearBuffer() {
  memset(accumulatedBuffer, 0, sizeof(accumulatedBuffer));  // Limpa o buffer após o envio
  bufferIndex = 0;  // Reinicia o índice
}
