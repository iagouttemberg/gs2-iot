# Integrantes
RM550806 - Iago Uttemberg - 2TDSPJ

RM98641 - Guilherme Jeronimo - 2TDSPJ

RM99347 - Thomas Wang - 2TDSPJ

RM550878 - Matheus Gonçalves - 2TDSPJ

RM99959 - Gustavo Furtado - 2TDSPJ

# Monitoramento de Consumo de Energia Elétrica

O projeto utiliza um microcontrolador ESP32 conectado a um sensor de corrente (simulado por um potenciômetro no ambiente Wokwi) para monitorar o consumo de energia elétrica. O ESP32 se conecta à internet através de uma rede Wi-Fi e envia dados de consumo para as plataformas **Adafruit IO** e **Thinger.io**.

## Objetivo Principal
Monitorar e reportar o consumo de energia elétrica em tempo real, identificando diferentes níveis de consumo (normal, crítico e elevado) e gerando alertas de acordo com esses níveis.

## Plataformas Utilizadas
1. **Adafruit IO**: Uma plataforma que permite enviar e receber dados utilizando o protocolo MQTT. Ela é usada para publicar os dados de consumo e alertas.
2. **Thinger.io**: Uma plataforma de Internet das Coisas (IoT) que permite monitorar e controlar dispositivos remotamente. Ela é usada para visualizar e atualizar os dados de consumo e alertas em um painel de controle.

## Componentes de Hardware
- **ESP32**: Microcontrolador que gerencia a leitura dos dados do sensor de corrente e a comunicação com as plataformas Adafruit IO e Thinger.io.
- **Sensor de Corrente (simulado por um potenciômetro)**: Mede a corrente elétrica (simulada) consumida pelo sistema.
- **LED**: Indica visualmente os alertas de consumo elevado.

## Descrição do Funcionamento
- O ESP32 lê continuamente os dados do sensor de corrente.
- Os dados são publicados em intervalos regulares nas plataformas Adafruit IO e Thinger.io.
- Baseado nos níveis de consumo (normal, crítico ou elevado), o sistema gera alertas e aciona o LED de alerta.

## Níveis de Consumo
- **Consumo Normal**: Indicativo de operação dentro dos parâmetros esperados.
- **Consumo Crítico**: Indicativo de um aumento significativo, mas ainda controlável.
- **Consumo Elevado**: Indicativo de um possível problema que necessita de atenção imediata.

## Benefícios do Projeto
- **Monitoramento Remoto**: Permite aos usuários monitorar o consumo de energia de forma remota, ajudando na gestão e economia de energia.
- **Alertas Proativos**: Gera alertas que ajudam a identificar e corrigir problemas de consumo de energia antes que se tornem críticos.
- **Integração com Plataformas IoT**: Utiliza plataformas populares de IoT, facilitando a integração com outros sistemas e dispositivos.

Em resumo, este projeto visa oferecer uma solução eficiente e acessível para monitoramento de consumo de energia elétrica, utilizando a capacidade de comunicação e processamento do ESP32 juntamente com as funcionalidades das plataformas Adafruit IO e Thinger.io.

## Códigos fonte

### sketch.ino
```cpp
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
```

### diagram.json
```cpp
{
  "version": 1,
  "author": "Iago Uttemberg",
  "editor": "wokwi",
  "parts": [
    { "type": "wokwi-breadboard-half", "id": "bb1", "top": 25.8, "left": -438.8, "attrs": {} },
    { "type": "board-esp32-devkit-c-v4", "id": "esp", "top": 0, "left": -4.76, "attrs": {} },
    {
      "type": "wokwi-led",
      "id": "led1",
      "top": -80.4,
      "left": -91.8,
      "attrs": { "color": "red", "flip": "1" }
    },
    { "type": "wokwi-potentiometer", "id": "pot1", "top": -97.3, "left": 9.4, "attrs": {} },
    {
      "type": "wokwi-resistor",
      "id": "r1",
      "top": 14.4,
      "left": -96.55,
      "rotate": 90,
      "attrs": { "value": "330" }
    }
  ],
  "connections": [
    [ "esp:TX", "$serialMonitor:RX", "", [] ],
    [ "esp:RX", "$serialMonitor:TX", "", [] ],
    [ "pot1:GND", "esp:GND.2", "black", [ "v28.8", "h76.8", "v38.4" ] ],
    [ "led1:A", "esp:2", "red", [ "v268.8", "h240", "v-67.2" ] ],
    [ "pot1:SIG", "esp:VP", "gold", [ "v19.2", "h-86.8", "v67.2" ] ],
    [ "esp:3V3", "bb1:tp.25", "red", [ "h-95.85", "v9.6" ] ],
    [ "pot1:VCC", "bb1:tp.24", "red", [ "v9.6", "h-202.4" ] ],
    [ "esp:GND.1", "bb1:tn.25", "black", [ "h-95.85", "v-105.6" ] ],
    [ "led1:C", "r1:1", "black", [ "v0" ] ],
    [ "r1:2", "bb1:tn.23", "black", [ "h0", "v18", "h-86.4" ] ]
  ],
  "dependencies": {}
}
```

### libraries.txt
```cpp
# Wokwi Library List
# See https://docs.wokwi.com/guides/libraries

WiFi
thinger.io
Adafruit IO Arduino
```

## Como simular o projeto
1. Baixe o arquivo `.zip` do projeto.
2. Descompacte o arquivo `.zip`.
3. Crie um projeto em branco no **Wokwi**.
4. Exclua todos os arquivos presentes no projeto.
5. Envie os arquivos `diagram.json`, `libraries.txt` e `sketch.ino` para o projeto.
6. Crie uma conta no **Adafruit IO** e em seguida crie os feeds necessários.
7. Crie uma conta no **Thinger.IO** e em seguida adicione um dispositivo.
8. Altere as informações do **Adafruit IO** e **Thinger.IO** necessárias do arquivo `sketch.ino` para o seu uso.
9. Execute a simulação do projeto no **Wokwi**.
10. Crie uma dashboard no **Thinger.IO** e adicione os widgets a sua escolha.

## Vídeo
[Demonstração da simulação do Projeto](https://www.youtube.com/watch?v=opm4qBfaCTY)

## Wokwi - Diagrama
![image](https://github.com/user-attachments/assets/9ff398b6-fcc8-4d64-8a8f-f23816f598c9)

## Adafruit IO - Feed de Alerta
![image](https://github.com/user-attachments/assets/67e63d64-1b0e-4d06-a400-f2c3da9b056a)

## Adafruit IO - Feed de Consumo
![image](https://github.com/user-attachments/assets/d78c7703-93d3-4c58-8977-e53a10695cbe)

## Thinger.IO - Dashboard
![image](https://github.com/user-attachments/assets/908b4d12-1c10-4953-8425-bb4917947954)

