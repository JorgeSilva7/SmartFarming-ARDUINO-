/***************************************************
 * SmartFarming Arduino CODE
 **************************************************/

/*---------------------------------
|  IMPORTACION DE LIBRERIAS       |
 ---------------------------------*/
//Librerias WiFi
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

//Libreria DHT11
#include <dht.h>

//Libreria coap_client
#include "coap_client.h" 
//--------------------------------
 
/*---------------------------------
|  DEFINICION DE PUERTOS          |
 ---------------------------------*/
#define DHT11_PIN D3
#define SOILHUMIDITY_PIN A0
#define RELAY1_PIN D2
#define RELAY2_PIN D1
#define RELAY3_PIN D5
//--------------------------------

/*----------------------------------
|  DEFINICION DE VARIABLES GLOBALES |
 -----------------------------------*/
coapClient coap;
dht dht;
IPAddress ip(170,239,86,38); //IP SERVIDOR COAP
int port =5683; //PUERTO DEL SERVIDOR COAP
int soilMoisterPorcentaje = 0;
char apikey[12] = "1a2b3c4d"; //APIKEY POR DEFAULT

// coap client response callback
void callback_response(coapPacket &packet, IPAddress ip, int port);

// coap client response callback
void callback_response(coapPacket &packet, IPAddress ip, int port) {
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;

 //response from coap server
  if(packet.type==3 && packet.code==0){
    Serial.println("ping ok");
  }
  Serial.println(p);
}

//Flag para guardar datos
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  shouldSaveConfig = true;
}

void setup() {
  
    Serial.begin(115200);
    //WiFiManager
    WiFiManager wifiManager;
    wifiManager.resetSettings(); //Se restean las settings al reiniciar arduino
    WiFiManagerParameter custom_apikey("HW Apikey", "1a2b3c45", apikey, 12); //Parametro para input custom al conectar wifi
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.addParameter(&custom_apikey);
    if(!wifiManager.autoConnect("SmartFarming", "123567890")){
      Serial.println("Error al conectar - Reiniciando");
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }

  Serial.println("WiFi Conectado");
  strcpy(apikey, custom_apikey.getValue());

  coap.start();
  coap.response(callback_response);
  // start coap client

  //Inicializando puertos de salida relays
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);

  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);
  digitalWrite(RELAY3_PIN, HIGH);

  //Iniciando lectura de DHT11
  int chk = dht.read11(DHT11_PIN);
  Serial.println("INICIALIZANDO...");
  delay(5000);
}

void loop() {
  char payload[30]="";
  
  char temp[8]="";
  dtostrf(getTemperatureData(), 6, 2, temp);
  if(getTemperatureData() > 20){
    digitalWrite(RELAY1_PIN, LOW);
    char payloadrelay[20]="";
    char relay[8]="";
    dtostrf(1, 2, 1, relay);
    strcpy(payloadrelay, apikey);
    strcat(payloadrelay, ";");
    strcat(payloadrelay, relay);
    coap.put(ip, port, "relay", payloadrelay, strlen(payloadrelay));
  }else{
    digitalWrite(RELAY1_PIN, HIGH);
  }
  
  char humidity[8]="";
  dtostrf(getHumidityData(), 6, 2, humidity);
  
  char soilHumidity[8]="";
  dtostrf(getSoilMoisterData(), 6, 2, soilHumidity);
  if(getSoilMoisterData() > 40){
    digitalWrite(RELAY3_PIN, LOW);
    char payloadrelay[20]="";
    char relay[8]="";
    dtostrf(3, 2, 1, relay);
    strcpy(payloadrelay, apikey);
    strcat(payloadrelay, ";");
    strcat(payloadrelay, relay);
    coap.put(ip, port, "relay", payloadrelay, strlen(payloadrelay));
    delay(2000);
    digitalWrite(RELAY3_PIN, HIGH);
  }

  strcpy(payload, apikey);
  strcat(payload, ";");
  strcat(payload, temp);
  strcat(payload, ";");
  strcat(payload, humidity);
  strcat(payload, ";");
  strcat(payload, soilHumidity);
    
  Serial.println(payload);
  
  int msg = coap.put(ip, port, "data", payload, strlen(payload));

  delay(20000);
  
}
/***************************************************
 * Get Temperature Sensor data
 **************************************************/
double getTemperatureData(void)
{
  int chk = dht.read11(DHT11_PIN);
  return dht.temperature;
}

/***************************************************
 * Get Humidity Sensor data
 **************************************************/
double getHumidityData(void)
{
  int chk = dht.read11(DHT11_PIN);
  return dht.humidity;
}


/***************************************************
 * Get Soil Humidity Sensor Data
 **************************************************/
double getSoilMoisterData(void)
{
  soilMoisterPorcentaje = 0;
  delay (500);
  int N = 3;
  //Leer sensor N veces para una mejor lectura (promedio)
  for(int i = 0; i < N; i++)
  {
    soilMoisterPorcentaje +=( 100.00 - ((analogRead(SOILHUMIDITY_PIN)/1023.00) * 100.00));   
    delay(150);
  }
  soilMoisterPorcentaje = soilMoisterPorcentaje/N; 
  return soilMoisterPorcentaje;
}
