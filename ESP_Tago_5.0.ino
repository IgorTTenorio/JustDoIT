/*
------------------------------------------------------------------------------------------------------------------------------
    VERSÃO 5.0 - ESP8266 COM TAGO                                                                                             
                                                                                                                                         
    Elaborador: Igor Tenório Teixeira                                                                                                    
                                                                                                                                         
    Conteúdo: Este programa realiza a medidas de nível e recebe medidas de previsão e os envia para plataforma Tago por meio de 
    comunicação MQTT
------------------------------------------------------------------------------------------------------------------------------
*/
#include "Wire.h"                                             // Declaração da biblioteca Wire
#include <WiFiEspClient.h>                                    // Declaração da biblioteca WiFiEspClient - WiFiESP
#include <WiFiEsp.h>                                          // Declaração da biblioteca WiFiESP
#include <WiFiEspUdp.h>                                       // Declaração da biblioteca WiFiEspUdp - WiFiESP
#include <PubSubClient.h>                                     // Declaração da biblioteca PubSubClient
#include <SoftwareSerial.h>                                   // Declaração da biblioteca SoftwareSerial

// Sensor Ultrassônico HC-SR04
int pinTrig = 4;                                              // Pino usado para disparar os pulsos do sensor
int pinEcho = 5;                                              // Pino usado para ler a saída do sensor
float TimeEcho = 0;                                           // Variável de tempo entre emissão e recepção
const float VelSound_mperus = 0.000340;                       // Em metros por microsegundo
float Dist_C = 0;                                             // Declaração da variável de distância calculada (cm)
float Nivel = 0;                                              // Declaração da variável Nível

#define myAdress 8                                            // Definição do endereço do arduino slave para comunicação I2C

// Defininção do SID e Password da Rede  
#define WIFI_AP ""                                            // Descomentar linha para uso DOMESTICO / Comentar linhas abaixo
#define WIFI_PASSWORD ""                                      // Descomentar linha para uso DOMESTICO / Comentar linhas abaixo 
//#define WIFI_AP "PUC-ACD"                                   // Descomentar linha para uso na PUC-CAMPINAS /  Comentar linhas acima
//#define WIFI_PASSWORD ""                                    // Descomentar linha para uso na PUC-CAMPINAS / Comentar linhas acima

#define TOKEN ""                                              // Definição do Token da Aplicação Tago

int Debug = 1;                                                // Chave de depuração do código, se debug for igual a 1 exibe valores no Serial Monitor

char apiServer[] = "mqtt.tago.io";                            // Definição do endereço do servidor da aplicação
char attributes[160];                                         // Declara variável attributes como um vetor de caracter em tamanho

// Strings de envio JSON
String NivelLixo;                                             // String de envio do Nivel
String Previsao;                                              // String de envio da Previsão

// Iniciação do objeto EspClient
WiFiEspClient espClient;                                      // A classe WiFiEspClient cria clientes que podem se conectar a servidores e enviar e receber dados
PubSubClient client(espClient);                               // Cria uma instância do cliente parcialmente inicializada 
                                                              // Antes de poder ser usado, os detalhes do servidor devem ser configurados
SoftwareSerial soft(2,3);                                     // Definição dos da comunicação serial nos pinos digitais (RX, TX)

int status = WL_IDLE_STATUS;                                  // Variável Status recebe parâmetro temporário até que o número de tentativas expire ou que uma conexão seja estabelecida
unsigned long ultEnvio;                                       // Declaração da variável ultEnvio (último envio ao servidor)
int intervaloEnvio = 30000;                                   // Declaração da variável intervaloEnvio

void setup() {
  Wire.begin(myAdress);                                       // Inicia a comunicação I2C
  Wire.onReceive(PegaPrevisao);                               // Registra o evento para coleta da previs
  pinMode(pinTrig,OUTPUT);                                    // Configura o pino Trig como saída
  digitalWrite(pinTrig,LOW);                                  // Inicia o pino em nível baixo
  pinMode(pinEcho,INPUT);                                     // Configura o pino Echo como entrada

  IniciaWiFi();                                               // Chama a função InitWiFi  
  client.setServer( apiServer, 1883 );                        // Define os detalhes do servidor (servidor,porta) ((IPAddress, uint8_t [] ou const char []),Int)
                                                              // Porta TCP/IP - 1883 || Porta TCP/IP sobre SSL 8883
                                                              // Permite a encadeação da função que instancia o cliente
  Previsao = '0';
  ultEnvio = 0;                                               // Variável ultEnvio recebe valor 0

  Serial.begin(9600);                                         // Inicia a serial com Baud Rate de 9600
  delay(100);                                                 // Aguarda 100 ms
}

void loop() {
  status = WiFi.status();                                     // Verifica o status da conexão
  if ( status != WL_CONNECTED) {                              // Se não estiver com status "Conectado"
    while ( status != WL_CONNECTED) {                         // Enquanto o status não for "Conectado"
    Serial.print("Tentando se conectar ao WPA SSID: ");       // Exibe mensagem de tentativa de conexão
    Serial.println(WIFI_AP);                                  // Nome do AP (SID)
                                                                    
                                                              // Conexão com a rede WPA/WPA2
    status = WiFi.begin(WIFI_AP, WIFI_PASSWORD);              // Inicia as configurações de rede da biblioteca WiFi e fornece o status atual
    delay(500);                                               // Aguarda meio segundo
  }
  Serial.println("Conectado ao AP");                          // Quando o status for "Conectado" exibe mensagem de sucesso
  }

  if ( !client.connected() ) {                                // Se o cliente não estiver conectado
    Reconexao();                                              // Chama função Reconnect
  }

  if ( millis() - ultEnvio > intervaloEnvio ) {               // Atualiza e envia apenas após o intervalo determinado
    PegaNivelLixo();                                          // Chama a função de medição do HC-SR04 (Nivel)
    EnviaDados();                                             // Chama função de envio de dados para o Tago
    if (Debug == 1){                                          // Se a chave Debug estiver com valor 1
      Depuracao();                                            // Chama a função de depuração
    }
    ultEnvio = millis();                                      // Variável ultEnvio recebe tempo em que foi enviado
  }
  client.loop();                                              // Deve ser chamado regularmente para permitir que o cliente processe mensagens recebidas e mantenha conexão com o servidor
}                                                             

void PegaNivelLixo(){
  digitalWrite(pinTrig, HIGH);                                // Nível alto no pino Trig
  delayMicroseconds(10);                                      // Aguardar pelo menos 10 us para SR04
  digitalWrite(pinTrig, LOW);                                 // Nível baixo no pino Trig
  TimeEcho = pulseIn(pinEcho,HIGH);                           // Mede o tempo de duração do sinal no pino de leitura (us)
  Dist_C = ((TimeEcho*100*VelSound_mperus)/2)+5;              // Calcula em centimetros
  Nivel = 82 - Dist_C;                                        // Calcula o nivel baseado na altura
  NivelLixo = String(Nivel);                                  // Armazena o valor de nivel em uma string para envio
}

void PegaPrevisao (){                                     
  Previsao = "";                                              // Limpa a variável previsão
  while (Wire.available()) {                                  // Enquanto a comunicação estiver disponível
    char prev = Wire.read();                                  // Realiza a leitura e guarda na variável prev
    Previsao += prev;                                         // Variável previsão recebe caracteres da variável prev 
  }
}

void EnviaDados(){
  // Prepara a string de dados JSON  
  String payload = "[{\"variable\":";                         // String dados Nivel                                                           
         payload += "\"NivelLixo\", \"value\":";              // String dados Nivel      
         payload += NivelLixo;                                // String dados Nivel     
         payload += ", \"unit\":\"cm\"}";                     // String dados Nivel

         payload += ",{\"variable\":";                        // String dados Previsao                                                           
         payload += "\"Previsao\", \"value\":\"";             // String dados Previsao      
         payload += Previsao;                                 // String dados Previsao 
         payload += "\"}]";                                   // String dados Previsao
         
  // Envio do payload        
  payload.toCharArray( attributes, 160 );                     // Converte a string de dados em um vetor com os parâmetros acima criados
  client.publish( "tago/data/post", attributes );             // Publica uma mensagem de cadeia (string) no tópico especificado (tópico,payload)                               
}

void Depuracao(){
  Serial.println("Dados Coletados...");                       // Exibe mensagem de coleta de dados
  Serial.println(Dist_C);
  Serial.print("Nivel Lixo: ");                               // Exibe mensagem Nivel 
  Serial.print(Nivel);                                        // Exibe mensagem Nivel 
  Serial.print(" cm\t");                                      // Exibe mensagem Nivel
  Serial.print("Previsao: ");                                 // Exibe mensagem Previsao
  Serial.print(Previsao);                                     // Exibe mensagem Previsao
  Serial.print(" ");
  
  Serial.print( "Dados Enviados : [" );                       // Exibe mensagem de envio
  Serial.print( NivelLixo );   Serial.print( "," );           // Exime mensagem de envio
  Serial.print( Previsao );                                   // Exibe mensagem de envio      
  Serial.print( "]   -> " );                                  // Exibe mensagem de envio
  Serial.println( attributes );                               // Exibe ao final a variável attributes  
}

void IniciaWiFi(){
  soft.begin(9600);                                                 // Inicia a serial para o módulo ESP
  WiFi.init(&soft);                                                 // Inicia o módulo ESP com parâmetro "soft" ou espSerial para interface Serial
  // Verifica a presença do módulo ESP
  if (WiFi.status() == WL_NO_SHIELD) {                              // Retorna o status da conexão quando não nenhum shield WiFi estiver presente
    Serial.println("ERRO: O módulo ESP não está conectado!");       // Exibe mensagem de erro 
    while (true);                                                   // Não continua até que erro seja resolvido
  }
  Serial.println("Conectando ao AP ...");                           // Se o módulo estiver conectado, exibe mensagem de conexão
  // Tentativa de conectar a Rede WiFi
  while ( status != WL_CONNECTED) {                                 // Enquanto o status não for "Conectado"
    Serial.print("Tentando se conectar ao WPA SSID: ");             // Exibe mensagem de tentativa de conexão
    Serial.println(WIFI_AP);                                        // Nome do AP (SID)
    // Conexão com a rede WPA/WPA2
    status = WiFi.begin(WIFI_AP, WIFI_PASSWORD);                    // Inicia as configurações de rede da biblioteca WiFi e fornece o status atual
    delay(500);                                                     // Aguarda meio segundo
  }
  Serial.println("Conectado ao AP");                                // Quando o status for "Conectado" exibe mensagem de sucesso
}

void Reconexao() {
  while (!client.connected()) {                                     // Enquanto o cliente não estiver conectado
    Serial.println("Conectando ao Tago ...");                       // Exibe mensagem de conectando
    if ( client.connect("Arduino Uno Device", TOKEN, NULL) ) {      // Tenta conectar ao serviço, passando os parâmetros (ClientID, Username, Password)
      Serial.println( "[DONE]" );                                   // Caso sucesso, exibe mensagem de exito
    } else {                                                        // Senão,
      Serial.print( "[FAILED] [ rc = " );                           // Exibe mensagem de falha
      Serial.print( client.state() );                               // Retorna o estado atual do cliente, consultar constantes PubSubClient para mais info
      Serial.println( " : próxima tentativa em 5 segundos]" );      // Exibe mensagem de nova tentativa
      delay( 5000 );                                                // Aguarda 5 segundos antes da próxima tentativa  
    }
  }
}
