/*
------------------------------------------------------------------------------------------------------------------------------
    VERSÃO 1.0 - I2C - M3 - Sensor Infravermelho                                                                                            
                                                                                                                                         
    Elaborador: Igor Tenório Teixeira                                                                                                    
                                                                                                                                         
    Conteúdo: Este programa realiza a medidas de nível e tempos de depósito de objetos na lixeira, passando ambos dados como
    parâmetros para o algorítmo de regressão do Médoto 3, enviando os resultados para o arduino slave. 
    Além disso os dados de nível são exibidos em  um display LCD Nokia 5110.
------------------------------------------------------------------------------------------------------------------------------
*/
#include <Wire.h>                                           // Declaração da biblioteca Wire
#include <SharpIR.h>                                        // Declaração da biblioteca SharpIR
#include <LCD5110_Basic.h>                                  // Declaração da biblioteca LCD5110
#include <LinearRegression.h>                               // Declaração da biblioteca LinearRegression

// Regressão Linear
LinearRegression LR = LinearRegression(0,3600);             // Inicia o objeto LR com tempo mínimo de 0 e máximo de 3600 (em segundos)
double(Parametros[3]);                                      // Declaração do vetor Parametros com três posições 
int i = 0;                                                  // Declaração da variável auxiliar i tipo inteiro
int j = 0;                                                  // Declaração da variável auxiliar j tipo inteiro  
double B = 0;                                               // Declaração da variável B (Parâmetro da equação da reta - Coeficiente angular)
double A = 0;                                               // Declaração da variável A (Parâmetro da equação da reta - Coeficiente linear, Y onde X = 0)   
double X = 0;                                               // Declaração da variável X (Parâmetro da equação da reta - Eixo do Tempo = Previsão)
String Previsao;                                            // Declaração da string previsão
char prev[9];                                               // Declaração do vetor de char prev com 9 posições
int Intervalo;                                              // Declaração da variável Intervalo    
int Horas;                                                  // Declaração da variável Horas
int Minutos;                                                // Declaração da variável Minutos
int Segundos;                                               // Declaração da variável Segundos
unsigned int tam_prev;                                      // Declaração da variável tam_prev

// Sensor Infravermelho GP2Y0A02Y
#define IR A0                                               // Pino analógico em que o sensor está conectado
#define Modelo 20150                                        // Determina o modelo do sensor (1080 para o 2Y0A21Y e 20150 para o 2Y0A02Y)
SharpIR SharpIR(IR,Modelo);                                 // Iniciação da biblioteca SharpIR
int Dist_I = 0;                                             // Variável de distância em centímetros
const float Altura = 82;                                    // Declaração da constante Altura (do suporte ao fundo da caixa)
float Nivel = 0;                                            // Declaração da variável Nível
double Niveis[500];                                         // Declaração do vetor Niveis com tamanho de 500 posições 
                                                            // (Assume-se que o máximo de objetos é 500 na caixa) - Uso do Arduino Mega
// Sensor KY-25
int analogPin = A1;                                         // Declaração do pino de entrada para interface analógica do sensor
int analogVal;                                              // Declaração da variável dos valores analógicos recebidos 
unsigned long Tempo;                                        // Declaração da variável Tempo
unsigned long TempMax;                                      // Declaração da variável TempMáx
double Tempos[500];                                         // Declaração do vetor Tempos com tamanho de 500 posições - Uso do Arduino Mega

//Configuração LCD Nokia 5110
LCD5110 tela(8,9,10,12,11);                                 // Declaração dos pinos conectados ao LCD5110
//Obtendo as fontes 
extern uint8_t SmallFont[];                                 // Declaração da fonte SmallFont  
extern uint8_t MediumNumbers[];                             // Declaração da fonte MediumNumbers
extern uint8_t BigNumbers[];                                // Delcaração da fonte BigNumbers

#define slaveAdress 8                                       // Definição do endereço do arduino slave para comunicação I2C

void setup() { 
  pinMode(analogPin,  INPUT);                               // Configura o pino do KY-25 como entrada
  tela.InitLCD();                                           //Inicializando o display
  Wire.begin();                                             // Ingressa ao barramento I2C
  LR.learn(0,0);                                            // Chama a função Learn (Aprender) com valores iniciais
 
  TempMax = 3600;                                           // Inicia TempMax com 3600s ou 1h
  Tempos[0] = 0;                                            // Inicia primeira posição do vetor Tempos com valor 0
  
  Serial.begin(9600);                                       // Inicia a serial com Baud Rate de 9600  
  delay(100);                                               // Aguarda 100 ms
}

// Função que exibe os valores na tela, recebe os parâmetros da regressão linear
void SerialMonitor () {
  if (i == 10) {                                            // Se i = 10, caso entrem 10 amostras
    for (j=0; j < i; j++) {                                 // Exibe na tela os valores de Tempo e Nivel coletados para analise
      Serial.print(Tempos[j]);                              // Tempos
      Serial.print("     ");                                //
      Serial.println(Niveis[j]);                            // Niveis
    }
  } else {                                                  // Mais que 10 amostras fica inviável printar o vetor, portanto é exibida
                                                            // somente a última amostra coletada
    Serial.print(Tempos[i-1]);                              // Tempos                              
    Serial.print("     ");                                  //
    Serial.println(Niveis[i-1]);                            // Niveis
  }
  Serial.println("");                                       // 
  Serial.print(B);                                          // Exibe o parâmetro B
  Serial.print("     ");                                    //
  Serial.print(A);                                          // Exibe o parâmetro A
  Serial.print("     ");                                    //
  Serial.print(X);                                          // Exibe o parâmetro X
  Serial.print("     ");                                    //
  Serial.print(Intervalo);                                  // Exibe o resultado do intervalo  
  Serial.print("     ");                                    //
  Serial.print(Previsao);                                   // Exibe o resultado da previsão       
  Serial.println("");                                       //
}

// Função que calcula os parâmetros de regressão linear e o valor estimado de previsão
void CalculoRegressao () {
  if (Tempo >= TempMax) {                                   // Se o tempo máximo ultrapassar o limite de 1h
    TempMax = TempMax + 3600;                               // Soma-se mais 1h para o tempo máximo
  }
  LinearRegression LR = LinearRegression(0,TempMax);        // Reinicia o objeto LR
  for (j=0; j < i; j++) {                                   // Laço d e repetição para aprendizado dos valores
    LR.learn(Tempos[j],Niveis[j]);                          // Aprende os valores contidos nos vetores
  }
  LR.getValues(Parametros);                                                          // Retorna os parâmetros da regressão linear
  B = Parametros[0];                                                                 // Parâmetro B
  A = Parametros[1];                                                                 // Parâmetro A
  X = abs((45-Parametros[1])/Parametros[0]);                                         // Com base nos últimos dois parâmetros calcula a previsão X levando em consideração quando a lixeira está cheia (Y=50)
  Intervalo =  int (X -Tempos[i-1]);                                                 // Calcula o intervalo (quanto tempo resta até a lixeira estar cheia)
  Horas =    (Intervalo/3600);                                                       // Transforma Intervalo em Horas
  Minutos =  ((Intervalo%3600)/60);                                                  // Transforma Intervalo em Minutos  
  Segundos = (Intervalo%60);                                                         // Transforma Intervalo em Segundos
  Previsao = String(Horas) + 'h' + String(Minutos) + 'm' + String(Segundos) + 's';   // Concatena Horas, Minutos e Segundos na string Previsão     
  tam_prev = Previsao.length();                                                      // Pega o tamanho da string previsão e atribui a variável tam_prev          
  Previsao.toCharArray(prev,9);                                                      // Transforma string Previsão em no array de char prev 
  Wire.beginTransmission(slaveAdress);                                               // Inicia a transmissão para o arduino slave   
  for (j = 0; j < tam_prev; j++){                                                    // Laço de 0 até o tamanho da variável Previsão (tam_prev) 
    Wire.write(prev[j]);                                                             // Escreve caracter por caracter no barramento I2C 
  }
  Wire.endTransmission();                                                            // Finaliza a transmissão para o arduino slave       
  SerialMonitor();                                                                   // Chama função de exibição no Serial Monitor
}

void loop() {
  
  // Medida Infravermelho 
  Dist_I = SharpIR.distance()+16;                           // Retorna a distância do objeto que está medindo
  Nivel = Altura - Dist_I;                                  // Calcula Nivel com base na distância coletada e a distância até o fundo da caixa = 82 cm
  Niveis[i] = Nivel;                                        // Coloca o valor de Nivel coletado no vetor Niveis
    
  // Medida KY-25
  analogVal = analogRead(analogPin);                        // Faz a leitura analógica do pino do KY-025
  if(analogVal < 900)                                       // Caso esse valor for menor que 900 significa que houve um sinal do sensor (fechamento dos resistor pelo campo magnetico)
  {
    Tempo = millis()/1000;                                  // A variável Tempo recebe a função millis e divide por 1000 para ter o valor em segundos
    Tempos[i] = Tempo;                                      // Coloca o valor Tempo coletado no vetor Tempos    
    delay(2000);                                            // Aguarda 2 segundos para próxima atualização
    i = i + 1;                                              // Soma 1 a variável auxiliar i para continuação dos vetores

    // Exibe no LCD5110
    tela.clrScr();                                          // Limpa a tela do LCD
    tela.setFont(SmallFont);                                // Configura fonte SmallFont
    tela.print("Nivel:", CENTER, 15);                       // Escreve Nivel no centro 
    tela.printNumF(Nivel,1,10, 24);                         // Escreve o valor da variável atual
    tela.print("/",36,24);                                  // Escreve barra (/)
    tela.printNumF(50.0,1,45,24);                           // Escrevev tamanho máximo da caixa coletora               
    tela.print("cm", 70, 24);                               // Escreve a unidade em cm
    if (i >= 10) {                                          // Se i for maior ou igual a 10
      CalculoRegressao();                                   // Chama a função de cálculo CalculoRegressao
    } 
  }
}
