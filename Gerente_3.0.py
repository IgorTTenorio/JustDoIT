'''
---------------------------------------------------------------------------------------------------
    VERSÃO 3.0 - MÓDULO GERENTE LIXEIRA INTELIGENTE
    
    Elaborador: Igor Tenório Teixeira                                        Data: 28/08/19
    
    Conteúdo: Nesta versão do módulo gerente será testado a coleta de informações do protótipo
    incrementado da lixeira. Os valores coletados da Base_3.0 através de comunicação serial serão
    exibidos conforme o número de medidas escolhido, e ao final com o comendo sair será gerado um
    arquivo XLS com log dos dados.  Serão coletados dados do sensor Infravermelho.
---------------------------------------------------------------------------------------------------
'''

# IMPORTA AS BIBLIOTECAS
import xlwt
import serial
import math
import time
import struct
from datetime import datetime

now = datetime.now()

#EXCEL
#Cria uma nova planilha Excel
wb = xlwt.Workbook()
#Cria uma nova aba dentro da planilha
ws = wb.add_sheet("Medidas")
ws.write(0,0,"Lixeira Inteligente")
ws.write(1,0,"Medida")
ws.write(1,1,"Dist_I")
ws.write(1,2,"Dist_I2")
ws.write(1,3,"Dist_U")

# Configura a serial
# para COM# o número que se coloca é n-1 no primeiro parâmetrso. Ex COM9  valor 8
n_serial = input("Digite o número da serial = ") #seta a serial
n_serial1 = int(n_serial) - 1
ser = serial.Serial(n_serial1, 9600, timeout=0.5,parity=serial.PARITY_NONE) # seta valores da serial


# Identificação da base
#ID_base = raw_input('ID_base = ')
ID_base = 0
# Identificação do sensor a ser acessado
#ID_sensor = raw_input('ID_sensor = ')
ID_sensor = 1

# Cria o vetor Pacote
PacoteTX = {}
PacoteRX = {}

# Cria Pacote de 52 bytes com valor zero em todas as posições
for i in range(52): # faz um array com 52 bytes
   PacoteTX[i] = 0
   PacoteRX[i] = 0

while True:
   try:

      # Imprime na tela o menu de opções
      print ('Escolha um comandos abaixos e depois enter')
      print ('1 - Realizar medidas:')
      print ('s - Para sair:')

      Opcao = input('Entre com a opção = ')

      # Limpa o buffer da serial
      ser.flushInput()

      # Coloca no pacote o ID_sensor e ID_base
      PacoteTX[8] = int(ID_sensor)
      PacoteTX[10] = int(ID_base)

      # Leitura de umidade do solo
      if Opcao == "1":
         Total_de_Medidas = input('Entre com o número de medidas = ')

         # FOR PARA REALIZAR AS MEDIDAS.
         for j in range(int(Total_de_Medidas)):
            # TRANSMISSÃO DO PACOTE
            for k in range(52): # transmite pacote
               TXbyte = chr(PacoteTX[k])
               ser.write(bytes(TXbyte, 'UTF-8'))
            # Aguarda a resposta do sensor
            time.sleep(0.1)

            PacoteRX = ser.read(52) # faz a leitura de 52 bytes do buffer que recebe da serial pela COM

            if len(PacoteRX) == 52:

               ad0h = PacoteRX[17] # inteiro
               ad0l = PacoteRX[18] # resto

               ad1h = PacoteRX[20] # inteiro
               ad1l = PacoteRX[21] # resto

               # Converte o número recebido para o valor multiplicado por 256 e divide por 100
               Dist_I = (ad0h * 256 + ad0l)
               Dist_U = (ad1h * 256 + ad1l)/100.0
               Dist_I2 = Dist_I + 20
           
            print ('Medida = ',j+1,'     Distância Infravermelho = '     ,Dist_I, ' cm','     Distância Corrigida = ',Dist_I2,'     Distância Ultrassônico = '     ,Dist_U)     
            ws.write(j+2,0,j+1)
            ws.write(j+2,1,Dist_I)
            ws.write(j+2,2,Dist_I2)
            ws.write(j+2,3,Dist_U)

            time.sleep(2)


      if Opcao == "s" or Opcao == "S":# caso o caracter digitado for s          
         ser.close() # fecha a porta COM
         strSave = now.strftime("Dados_Lixeira_%d_%m_%Y_%H-%M-%S.xls")
         wb.save(strSave)
         print ('Fim da Execução')  # escreve na tela
         print ('Arquivo Dados_Lixeira criado com sucesso!')  # escreve na tela
         break
            
   except KeyboardInterrupt:
       ser.close()
       break

