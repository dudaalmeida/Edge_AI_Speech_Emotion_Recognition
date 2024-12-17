#include <driver/i2s.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define I2S_SD 25
#define I2S_WS 26
#define I2S_SCK 33
#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE (16000)
#define I2S_SAMPLE_BITS (16)
#define I2S_READ_LEN (16 * 1024)
#define RECORD_TIME (20) // Segundos
#define I2S_CHANNEL_NUM (1)
#define SEND_BUFFER_SIZE (I2S_READ_LEN)

const char* ssid = "NET_2GE9DC82";
const char* password = "DDE9DC82";

const char* serverUrl = "http://192.168.182.94:5000/upload";

HTTPClient http;

void setup() {
  Serial.begin(115200);
  
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
  Serial.println("Conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
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
  char* i2s_read_buff = (char*) malloc(I2S_READ_LEN);
  
  Serial.println("Iniciando gravação e envio...");

  for (int i = 0; i < RECORD_TIME; i++) { // Loop para RECORD_TIME segundos
    // Ler dados do microfone I2S
    i2s_read(I2S_PORT, (void*) i2s_read_buff, I2S_READ_LEN, &bytes_read, portMAX_DELAY);

    // Enviar dados via HTTP
    sendAudioToServer((uint8_t*)i2s_read_buff, bytes_read);
    Serial.printf("Enviado %u bytes para o servidor\n", bytes_read);
  }
  
  // Envio final com a flag JSON sinalizando o término
  sendEndFlag();

  free(i2s_read_buff);
  Serial.println("Gravação e envio concluídos.");
  vTaskDelete(NULL);
}

void sendAudioToServer(uint8_t* data, size_t length) {
  if (WiFi.status() == WL_CONNECTED) {

    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/octet-stream");

    int httpResponseCode = http.POST(data, length);

    if (httpResponseCode > 0) {
      Serial.printf("Resposta do servidor: %d\n", httpResponseCode);
    } else {
      Serial.printf("Erro ao enviar: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    http.end();
  } else {
    Serial.println("Erro: Wi-Fi desconectado!");
  }
}

void sendEndFlag() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    Serial.println("Enviando sinalização de término...");

    http.begin(serverUrl);  // URL do servidor
    http.addHeader("Content-Type", "application/json");

    String jsonPayload = "{\"end\": true}";  // JSON com a flag de término
    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      Serial.printf("Sinalização de término enviada. Resposta: %d\n", httpResponseCode);
    } else {
      Serial.printf("Erro ao enviar sinalização de término: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
  } else {
    Serial.println("Erro: Wi-Fi desconectado ao tentar enviar sinalização de término!");
  }
}