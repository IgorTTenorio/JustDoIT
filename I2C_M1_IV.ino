/*
------------------------------------------------------------------------------------------------------------------------------------------
    TESTE DO MODO 1 - PREVISÃO E REGRESSÃO LINEAR                                                                        Data: 15/08/19  
                                                                                                                                         
    Elaborador: Igor Tenório Teixeira                                                                                                    
                                                                                                                                         
    Conteúdo: 
------------------------------------------------------------------------------------------------------------------------------------------
*/ 

#include <Wire.h>                                           // Declaração da biblioteca Wire
#include <SharpIR.h>                                        // Declaração da biblioteca SharpIR
#include <LinearRegression.h>                               // Declaração da biblioteca LinearRegression.h

// Regressão Linear
LinearRegression LR = LinearRegression(0,3600);             // Inicia o objeto LR com tempo mínimo de 0 e máximo de 3600 (em segundos)
double(Parametros[3]);                                      // Declaração do vetor Parametros com três posições 
int i = 0;                                                  // Declaração da variável auxiliar i tipo inteiro
int j = 0;                                                  // Declaração da variável auxiliar j tipo inteiro  
double B = 0;                                               // Declaração da variável B (Parâmetro da equação da reta - Coeficiente angular)
double A = 0;                                               // Declaração da variável A (Parâmetro da equação da reta - Coeficiente linear, Y onde X = 0)   
double X = 0;                                               // Declaração da variável X (Parâmetro da equação da reta - Eixo do Tempo = Previsão)
String Previsao;
char prev[9];
int Intervalo;                                   
int Horas;
int Minutos;
int Segundos;
unsigned int tam_prev;

// Sensor Infravermelho GP2Y0A02Y
#define IR A0                                               // Pino analógico em que o sensor está conectado
#define Modelo 20150                                        // Determina o modelo do sensor (1080 para o 2Y0A21Y e 20150 para o 2Y0A02Y)
SharpIR SharpIR(IR,Modelo);                                 // Iniciação da biblioteca SharpIR
int Dist_I = 0;                                             // Variável de distância em centímetros
const float Altura = 82;                                    // Declaração da constante Altura (do suporte ao fundo da caixa)
float Nivel = 0;                                            // Declaração da variável Nível
double Niveis[10];                                          // Declaração do vetor Niveis com tamanho de 10 posições

// Sensor KY-25
int analogPin = A1;                                         // Declaração do pino de entrada para interface analógica do sensor
int analogVal;                                              // Declaração da variável dos valores analógicos recebidos 
unsigned long Tempo;                                        // Declaração da variável Tempo
unsigned long TempMax;                                      // Declaração da variável TempMáx
double Tempos[10];                                          // Declaração do vetor Tempos com tamanho de 10 posições

// endereco do modulo slave que pode ser um valor de 0 a 255
#define slaveAdress 8

void setup() { 
  Wire.begin();                                             // Ingressa ao barramento I2C
  LR.learn(0,0);                                            // Chama a função Learn (Aprender) com valores iniciais
  LR.fixN(10);                                              // Chama a função FixN (Fixa o número de amostras) é útil quando 
                                                            // é necessário atualizações continuas dos parâmetros de regressão linear
  TempMax = 3600;                                           // Inicia TempMax com 3600s ou 1h
  Tempos[0] = 0;                                            // Inicia primeira posição do vetor Tempos com valor 0
  
  Serial.begin(9600);                                       // Inicia a serial com Baud Rate de 9600  
}

// Função que exibe os valores na tela, recebe os parâmetros da regressão linear
void SerialMonitor () {
  for (j = 0; j < 10; j++) {                                // Exibe na tela os valores de Tempo e Nivel coletados para analise
    Serial.print(Tempos[j]);                                // Tempos
    Serial.print("     ");                                  //  
    Serial.println(Niveis[j]);                              // Niveis
  }
  Serial.println("");
  Serial.print(B);                                   
  Serial.print("     ");
  Serial.print(A);                                 
  Serial.print("     ");                              
  Serial.print(X);                                   
  Serial.print("     ");
  Serial.print(Intervalo);                                      
  Serial.print("     "); 
  Serial.print(Previsao);                                          
  Serial.println("");       
}

void loop() {
  // Medida Parâmetros e Previsão
  if (i == 10) {                                            // Se i for igual a 10 todas posições estão preenchidas
    if (Tempo >= TempMax) {                                 // Se o tempo máximo ultrapassar o limite de 1h
      TempMax = TempMax + 3600;                             // Soma-se mais 1h para o tempo máximo 
    }
    LinearRegression LR = LinearRegression(0,TempMax);      // Reinicia o objeto LR
    for (j = 0; j < 10; j++) {                              // Laço d e repetição para aprendizado dos valores
      LR.learn(Tempos[j],Niveis[j]);                        // Aprende os valores contidos nos vetores
    }
    LR.getValues(Parametros);                               // Retorna os parâmetros da regressão linear 
    B = Parametros[0];                                      // Parâmetro B
    A = Parametros[1];                                      // Parâmetro A
    X = abs((50-Parametros[1])/Parametros[0]);              // Com base nos últimos dois parâmetros calcula a previsão X levando em consideração quando a lixeira está cheia (Y=55)
    Intervalo =  int (X -Tempos[9]);
    Horas =    (Intervalo/3600);
    Minutos =  ((Intervalo%3600)/60);
    Segundos = (Intervalo%60);
    Previsao = String(Horas) + 'h' + String(Minutos) + 'm' + String(Segundos) + 's';
    tam_prev = Previsao.length();
    Previsao.toCharArray(prev,9);                   
    Wire.beginTransmission(slaveAdress);
    for (j = 0; j < tam_prev; j++){
      Wire.write(prev[j]); 
    }
    Wire.endTransmission();
    SerialMonitor();                               // Chama função de exibição no Serial Monitor
    i = 0;                                         // Zera a variável i para começar novos vetores
  }

  // Medida Infravermelho 
  Dist_I = SharpIR.distance();                              // Retorna a distância do objeto que está medindo
  Nivel = Altura - Dist_I;                                  // Calcula Nivel com base na distância coletada e a distância até o fundo da caixa = 82 cm
  Niveis[i] = Nivel;                                        // Coloca o valor de Nivel coletado no vetor Niveis

  
  // Medida KY-25
  analogVal = analogRead(analogPin);                        // Faz a leitura analógica do pino do KY-025
  if(analogVal < 900)                                       // Caso esse valor for menor que 900 significa que houve um sinal do sensor (fechamento dos resistor pelo campo magnetico) essa é uma margem para o caso de ruido
  {
  Tempo = millis()/1000;                                    // A variável Tempo recebe a função millis e divide por 1000 para ter o valor em segundos
  Tempos[i] = Tempo;                                        // Coloca o valor Tempo coletado no vetor Tempos    
  delay(2000);                                              // Aguarda 2 segundos para próxima atualização
  i = i + 1;                                                // Soma 1 a variável auxiliar i para continuação dos vetores
  }
}
