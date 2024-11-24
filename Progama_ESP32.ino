/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#include    <WiFi.h>
#include    <HTTPClient.h>
#include    <Adafruit_INA219.h> 

Adafruit_INA219 ina219_0 (0x40); /*Adafruit_INA219 ina219_1 (0x41) ; para utilização de outro sensor */

const char* ssid     = "raspiAP";
const char* password = "raspberry";
const String rpiStaticIp = "10.0.0.220/";
const String folderName = "mpptSolar/";
const String fileName = "post_mppt_data.php";
int httpResponseCode = 0;  
String httpRequestData = ""; 
bool success = 0;
String      apiKeyValue = "tPmAT5Ab3j7F9NicolasMatheus";
uint32_t    SERIAL_BAUDRATE = 115200;
uint32_t    lastUpdate = 0;
uint32_t    chipID = 0;
uint16_t    interval = 60000; // Intervalo de medição definido para 1 minuto
uint8_t     counterEsp = 0; 
// MPPT GLOBAL VARIABLES
float tensao = .0, corrente = .0, potencia = .0;
uint8_t passo = 0;	// 0 ~ 255
float PotenciaAnt = 0;
int x = 0;
int x2 = 0;
int i_fimcurso = 0;
int i_tensao = 0;
int i4 = 0;
int i5 = 0;
int i6 = 0;
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
String createServerName(void){  // older - apiPhp 
String serverNamePath = "http://" + rpiStaticIp + folderName + fileName;
Serial.print("|>>> serverNamePath:\t"); Serial.println(serverNamePath);
return (serverNamePath);}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void showChipID(void){
for(int i1 = 0; i1 < 17; i1 = i1 + 8) {
	  chipID |= ((ESP.getEfuseMac() >> (40 - i1)) & 0xff) << i1;}
Serial.printf("|> ESP32 Chip model:\t%s - Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
Serial.printf("|> This chip has:\t%d cores\n", ESP.getChipCores());
 Serial.print("|> Chip ID:\t\t"); Serial.println(chipID, HEX); }
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
// FUNÇÃO QUE LÊ OS VALORES DO INA219
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void LeituraINA219(void){
  
  tensao = ina219_0.getShuntVoltage_mV()*0.1; /*comando para chamar a tensão no shunt */
  corrente = ina219_0.getCurrent_mA(); /* comando para chamar a corrente */
  potencia = tensao*corrente/1000; /*comando para chamar a potência */
  i_tensao = 0;
  if (tensao > 0){
  i_tensao = 1;
  Serial.print("Tensão do Painel: "); 
  Serial.print(tensao); 
  Serial.println(" V"); /*printa a tensão de entrada */
  Serial.print("Corrente: "); 
  Serial.print(corrente); 
  Serial.println(" mA"); /*printa a corrente */
  Serial.print("Potência: "); 
  Serial.print(potencia); 
  Serial.println(" W"); /* printa a potência */
  Serial.print("Potência Referência: "); 
  Serial.print(PotenciaAnt); 
  Serial.println(" W"); /* printa a potência */ 
  Serial.print("Passos: "); 
  Serial.println(passo);
  Serial.println("");
  }
  if (tensao < 0){
  i_tensao = 0;
  Serial.print("Tensão do Painel Negativa "); 
  Serial.println("");
  }   

}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
// FUNÇÃO QUE TESTA OS FIM DE CURSOS
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void validacaoFimCurso (void){
  if (digitalRead(35) == LOW){
    i_fimcurso = 1;
  }
  }
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
// FUNÇÃO QUE RETORNA O PAINEL PARA O INÍCIO
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void RetornaPosicaoInicial (void){
     while(i_fimcurso == 1){
     Serial.println("Retornando a posição Inicial");
     Serial.println("");
     digitalWrite(19, HIGH);
     digitalWrite(18, HIGH);
     vTaskDelay(250);
     digitalWrite(18, LOW);
     vTaskDelay(250);
     if (digitalRead(36) == LOW){
     vTaskDelay(1000);
     if (digitalRead(36) == LOW){
     i_fimcurso = 0;
     passo = 0;
     PotenciaAnt = 0;
     }
     }
     }
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
// GENERATE DATA TO SEND TO SERVER - RPi
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void getSensorDataToSend(void){
// Prepare your HTTP POST request data
      httpRequestData =  "api_key=" 		  + apiKeyValue + 
                          "&tensao=" 		  + String(tensao) +
                          "&corrente=" 	  + String(corrente) +
                          "&potencia=" 	  + String(potencia) +
                          "&passo=" 		  + String(passo) +
                          "&counterEsp="  + String(counterEsp) +
                          "&chipID=" 		  + String(chipID, HEX) + "";
Serial.print("|> httpRequestData: "); Serial.println(httpRequestData);}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
// curl -X POST -d "api_key=tPmAT5Ab3j7F9NicolasMatheus&tensao=20.00&corrente=807.00&potencia=16.14&passo=5&counterEsp=94&chipID=1fbec4" "localhost/mpptSolar/post_mppt_data.php"
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
// SEND DATA TO SERVER - RPi
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool sendDataPHP(void){
success = 0;
// REALLY SEND DATA TO SERVER  
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
// Check WiFi connection status
  if(WiFi.status() == WL_CONNECTED){
    WiFiClient client;
    HTTPClient http;
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
// Your Domain name with URL path or IP address with path
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
    http.begin(client, createServerName());
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
// Specify content-type header
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
// Send HTTP POST request
    httpResponseCode = http.POST(httpRequestData);
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
    if (httpResponseCode > 0) {
      Serial.print("|> HTTP Response code:\t");
      Serial.println(httpResponseCode);
      String response = http.getString();
//    Serial.println("|> PHP Response string:"); Serial.println(response);
      if(response == "0K"){counterEsp++;
      Serial.print("|> counterEsp:\t"); Serial.println(counterEsp);
      success = 1;
        } else {success = 0; return success;}} 
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
    else {  Serial.print("|> Error code:\t");
            Serial.println(httpResponseCode);}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
    http.end();}
  else { Serial.println("WiFi Disconnected");}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
return success;}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
// FUNÇÃO - GIRO SENTIDO HORÁRIO
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void Avanca(void){

   digitalWrite(19, LOW);
   Serial.println("Girando Sentido Horário"); 
   Serial.print("Valor de i_fimcurso: "); 
   Serial.print(i_fimcurso);
   Serial.println("");
   i4 = 0;
   x = 0;
   while(i4 == 0){
   digitalWrite(18, HIGH);
   vTaskDelay(250);
   digitalWrite(18, LOW);
   vTaskDelay(250);
   x = x + 1;
   validacaoFimCurso();
   if (i_fimcurso == 1){
   i4 = 1;}
   if (x == 30){
   i4 = 1;}
   }
   passo = passo + 1;
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
// FUNÇÃO - GIRO SENTIDO ANTIHORÁRIO
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void Retorna(void){
    digitalWrite(19, HIGH);
    Serial.println("Girando Sentido Antihorário"); 
    Serial.print("Valor de i_fimcurso: "); 
    Serial.print(i_fimcurso);
    Serial.println("");
    while(i4 == 0){
    digitalWrite(18, HIGH);
    vTaskDelay(250);
    digitalWrite(18, LOW);
    vTaskDelay(250);
    x = x + 1;
    validacaoFimCurso();
    if (x == 30){
    i4 = 1;}
    if (i_fimcurso == 1){
    i4 = 1;}
    }
    i4 = 0;
    x = 0;
    passo = passo - 1; 

}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
// FUNÇÃO - AVISO DE INÍCIO DO PROGRAMA E SETUP
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void setup(){
  Serial.begin(SERIAL_BAUDRATE);
  vTaskDelay(2500);
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) { 
    vTaskDelay(500);
    Serial.print(".");}
  Serial.println("");
Serial.println("|> ------------------------------------------<|");
Serial.println("|> Connected to WiFi network");
  Serial.print("|> ESP32 MAC:\t\t");
      Serial.println(WiFi.macAddress());
  Serial.print("|> IP Address:\t\t");
      Serial.println(WiFi.localIP());
  Serial.print("|> Gateway:\t\t");      
      Serial.println(WiFi.gatewayIP());
  Serial.print("|> Signal strength:\t");
      Serial.print(WiFi.RSSI()); Serial.println(" dBm");
  showChipID();   // SHOW CHIP ID - EPS32
Serial.println("|> NICOLAS & MATHEUS STARTING SYSTEM");
Serial.println("|> ------------------------------------------<|");

  pinMode (18,OUTPUT); 
  pinMode (19,OUTPUT); 
  pinMode (35,INPUT_PULLUP); 
  pinMode (36,INPUT_PULLUP);

  if (! ina219_0.begin()) 
  { 
    Serial.println("Falha ao encontrar o INA219"); 
    while (1) { vTaskDelay(10); }
  }
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
// FUNÇÃO - boolean runEvery(unsigned uint32_t interval)
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
boolean runEveryA(uint32_t interval){
    volatile static uint32_t previousMillis = 0;
    uint32_t currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;   return true;}     return false;}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
// FUNÇÃO - TAREFAS PARA CADA INTERVALO
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
boolean runEveryB(uint32_t interval){
    volatile static uint32_t previousMillis = 0;
    uint32_t currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;   return true;}     return false;}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
// FUNÇÃO - LOOP PRINCIPAL 
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void loop() {
  
  while(i_fimcurso == 1){
    if(potencia < 10){
    Serial.print("Potência Medida Inferior a 10 W");
    Serial.println("");
    RetornaPosicaoInicial();
    }
    else{
    if(runEveryA(interval)){
    LeituraINA219();
    Serial.print("Valor de i_fimcurso: ");
    Serial.print(i_fimcurso);
    Serial.println("");
    getSensorDataToSend();
    }
    }
  }

  if(runEveryA(interval)){
  int i = 0;
  validacaoFimCurso();
  LeituraINA219();
  Serial.print("Valor de i_fimcurso: ");
  Serial.print(i_fimcurso);
  Serial.println("");

    /* PARTE DO CÓDIGO MPPT */

  if (i_tensao == 1){

  if (potencia >= PotenciaAnt){
    PotenciaAnt = potencia;    
   }

  if ((potencia <= PotenciaAnt*0.9)&&(potencia >= PotenciaAnt*0.7)){ 
    i = 0; 
    i6 = 0;
    while (i == 0){
    if(i6 <= 3){
    Avanca();
    i6 = i6 + 1;
    }

    while(i_fimcurso == 1){
    i = 1;
    if(potencia < 10){
    Serial.print("Potência Medida Inferior a 10 W");
    Serial.println("");
    RetornaPosicaoInicial();
    }
    else{
    if(runEveryA(interval)){
    LeituraINA219();
    Serial.print("Valor de i_fimcurso: ");
    Serial.print(i_fimcurso);
    Serial.println("");
    getSensorDataToSend();
    }
    }

    }
    if (i == 0){
      PotenciaAnt = potencia;
      LeituraINA219();
    if(potencia < PotenciaAnt){
      i = 1;
      vTaskDelay(5000);
      Retorna();
    }
    }

    }
  PotenciaAnt = potencia;
  }

getSensorDataToSend();
success = sendDataPHP();
vTaskDelay(5000);
if(!success){
if(runEveryB(5000)){  
LeituraINA219();
getSensorDataToSend();
sendDataPHP();
}
}
}

}  
}