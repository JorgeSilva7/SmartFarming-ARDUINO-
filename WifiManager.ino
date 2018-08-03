#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <dht.h>

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#include "coap_client.h" 
//instance for coapclient
coapClient coap;
//ip address and default port of coap server in which your interested in
IPAddress ip(170,239,86,38);
int port =5683;

char apikey[12] = "1a2b3c4d";

dht DHT;
#define DHT11_PIN D3

/* Soil Moister */
#define soilMoisterPin A0
#define relay3 D5
int soilMoister = 0;

#define relay1 D1
#define relay2 D2

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

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  shouldSaveConfig = true;
}

void setup() {
    Serial.begin(115200);
    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    WiFiManagerParameter custom_apikey("HW Apikey", "1a2b3c45", apikey, 12);
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.addParameter(&custom_apikey);
    if(!wifiManager.autoConnect("SmartFarming", "123567890")){
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }


    strcpy(apikey, custom_apikey.getValue());

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");

     // client response callback.
    // this endpoint is single callback.
    coap.response(callback_response);

    // start coap client
    coap.start();
    pinMode(relay1, OUTPUT);     //Set Pin12 as output 
    pinMode(relay2, OUTPUT);     //Set Pin12 as output 
    pinMode(relay3, OUTPUT);     //Set Pin12 as output 
    int chk = DHT.read11(DHT11_PIN);
  Serial.print(" Starting Humidity: " );
  Serial.print(DHT.humidity, 1);
  Serial.println('%');
  Serial.print(" Starting Temparature ");
  Serial.print(DHT.temperature, 1);
  Serial.println('C');
}

void loop() {
  bool state;
  Serial.println(apikey);
  char payload[20];
  int adc_MQ = analogRead(A0); //Lemos la salida analógica del MQ
  float voltaje = adc_MQ * (5.0 / 1023.0); //Convertimos la lectura en un valor de voltaje
  
  Serial.println(voltaje);
  char temp[8]; // Buffer big enough for 7-character float
  dtostrf(voltaje, 6, 2, temp); // Leave room for too large numbers!
  
  strcpy(payload, apikey);
  strcat(payload, ";");
  strcat(payload, temp);

  
  
  int msgid =coap.put(ip,port, "temp", payload,strlen(payload));

  state= coap.loop();

  int chk = DHT.read11(DHT11_PIN);
  Serial.print(" Starting Humidity: " );
  Serial.print(DHT.humidity, 1);
  Serial.println('%');
  Serial.print(" Starting Temparature ");
  Serial.print(DHT.temperature, 1);
  Serial.println('C');
  getSoilMoisterData();
  digitalWrite(relay1,LOW);
  delay(5000);
  digitalWrite(relay1,HIGH);
  
}

/***************************************************
 * Get Soil Moister Sensor data
 **************************************************/
void getSoilMoisterData(void)
{
  soilMoister = 0;
  delay (500);
  int N = 3;
  for(int i = 0; i < N; i++) // read sensor "N" times and get the average
  {
    soilMoister += analogRead(soilMoisterPin);   
    delay(150);
  }
  soilMoister = soilMoister/N; 
  Serial.println(soilMoister);
  soilMoister = map(soilMoister, 600, 0, 0, 100); 
}
