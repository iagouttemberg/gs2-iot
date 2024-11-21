#include <WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <ThingerESP32.h>

// Configurações Wi-Fi
const char* ssid = "Wokwi-GUEST";          // Nome da sua rede Wi-Fi
const char* password = "";                 // Senha da sua rede Wi-Fi

// Configurações do Adafruit IO
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883              // Porta MQTT (padrão: 1883)
#define AIO_USERNAME    "iagouttemberg"   // Seu nome de usuário no Adafruit IO
#define AIO_KEY         "aio_nFbn50KR0gX3jEKztiFgT61d5Mao"   // Sua chave API do Adafruit IO

WiFiClient wifiClient;
Adafruit_MQTT_Client mqttClient(&wifiClient, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Configurações dos feeds
Adafruit_MQTT_Publish consumoFeed = Adafruit_MQTT_Publish(&mqttClient, AIO_USERNAME "/feeds/consumo");
Adafruit_MQTT_Publish alertaFeed = Adafruit_MQTT_Publish(&mqttClient, AIO_USERNAME "/feeds/alerta");

// Configurações do Thinger.io
#define THINGER_USERNAME "iagouttemberg"
#define THINGER_DEVICE_ID "EcoTrack"
#define THINGER_DEVICE_CREDENTIAL "rYHqRCh&@&I6&jiS"
ThingerESP32 thing(THINGER_USERNAME, THINGER_DEVICE_ID, THINGER_DEVICE_CREDENTIAL);

// Pinos de hardware
const int sensorPin = A0; // Pino analógico do sensor de corrente
const int ledPin = 2;     // LED para indicação de alertas

// Limites de consumo
const int limiteNormal = 1638;   // Consumo normal (0 a 1638)
const int limiteCritico = 3276;  // Consumo crítico (1639 a 3276)
const int limiteElevado = 4095;  // Consumo elevado (> 3276)

// Variáveis do Thinger.io
int valorConsumo;
String alerta;

void conectarWiFi() {
  Serial.println("Conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Conectado!");
}

void conectarMQTT() {
  int8_t ret;
  while ((ret = mqttClient.connect()) != 0) {
    Serial.println("Falha na conexão MQTT. Código: ");
    Serial.println(mqttClient.connectErrorString(ret));
    mqttClient.disconnect();
    delay(5000); // Tenta novamente após 5 segundos
  }
  Serial.println("Conectado ao MQTT!");
}

void conectarThinger() {
  Serial.println("Conectando ao Thinger.io...");
  thing.handle();
  Serial.println("Conexão com o Thinger.io estabelecida!");
}

void verificarConsumo(int consumo) {
  char mensagem[50];
  snprintf(mensagem, sizeof(mensagem), "Consumo atual: %d", consumo);

  // Publica no Adafruit IO
  if (consumoFeed.publish((int32_t)consumo)) {
    Serial.printf("Publicado no feed de consumo: %d\n", consumo);
  } else {
    Serial.printf("Falha ao publicar no feed de consumo");
  }

  // Atualiza o valor do consumo no Thinger.io
  valorConsumo = consumo;

  if (consumo <= limiteNormal) {
    digitalWrite(ledPin, LOW);
    alerta = "Consumo normal.";
    if (alertaFeed.publish(alerta.c_str())) {
      Serial.println("Publicado no feed de alerta: Consumo normal");
    }
  } else if (consumo <= limiteCritico) {
    digitalWrite(ledPin, LOW);
    alerta = "Consumo crítico!";
    if (alertaFeed.publish(alerta.c_str())) {
      Serial.println("Publicado no feed de alerta: Consumo crítico");
    }
  } else {
    digitalWrite(ledPin, HIGH);
    alerta = "Consumo elevado!";
    if (alertaFeed.publish(alerta.c_str())) {
      Serial.println("Publicado no feed de alerta: Consumo elevado");
    }
  }

}

void setup() {
  Serial.begin(115200);

  // Configuração dos pinos
  pinMode(ledPin, OUTPUT);

  conectarWiFi();   // Conexão com o Wi-Fi
  conectarMQTT();   // Conexão com o broker MQTT (Adafruit IO)
  conectarThinger(); // Conexão com o Thinger.io

  // Configuração do Thinger.io
  thing.add_wifi(ssid, password);
  thing["Consumo"] >> outputValue(valorConsumo); 
  thing["Alerta"] >> outputValue(alerta);
}

void loop() {
  if (!mqttClient.connected()) {
    conectarMQTT(); // Reconecta ao MQTT caso a conexão seja perdida
  }

  mqttClient.processPackets(10000); // Mantém a conexão MQTT ativa

  int consumo = analogRead(sensorPin); // Leitura do sensor
  verificarConsumo(consumo);           // Processa e publica os dados de consumo

  thing.handle();  // Comunicação com a plataforma Thinger.io

  delay(2000); // Aguarda 2 segundos antes da próxima leitura
}
