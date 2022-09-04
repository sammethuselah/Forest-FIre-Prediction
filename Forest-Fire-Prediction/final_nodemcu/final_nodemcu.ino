#include <ThingSpeak.h>
#include "DHT.h"
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

unsigned long myChannelNumber = 1701474;
const char * myWriteAPIKey = "U6A9SEZOZX78K9YN";

const char* resource = "https://maker.ifttt.com/trigger/forest_fire/json/with/key/o3UmGdPco8QoD871FmUf95Fq0EpjyP40ZtJWYXmrt5x";
const char* server = "maker.ifttt.com";

#define DHTPIN 2     // what digital pin the DHT22 is conected to
#define DHTTYPE DHT11   // there are multiple kinds of DHT sensors
#define         MQ_PIN                       (0)     //define which analog input channel you are going to use

DHT dht(DHTPIN, DHTTYPE);

float Prob=0.0;
float t=0.0;
float h=0.0;
int sensorValue;
float sensorValue1 = 0.0;
int flame = 0;
int flame_sensor = 12; //Connected to D6 pin of NodeMCU
int buzzer = 4; //Connected to D2 pin of NodeMCU
int temp = 0;

float t_severe=37.5;
float t_moderate=33.5;
float t_mild=31.0;
float t_curr=0.0;
/******************************************************************************/
const char* ssid     = "Sam1"; // Your WiFi ssid
const char* password = "12345677"; // Your Wifi password;
/********************************************************************************/


// Initialise the WiFi and MQTT Client objects
WiFiClient wifiClient;

/*************************************************************/

const char* Status1="No Forest fire";

void setup()
{
  pinMode(flame_sensor, INPUT);
  pinMode(buzzer, OUTPUT);
  
  digitalWrite(buzzer, LOW);
  
  Serial.begin(9600);
  Serial.setTimeout(2000);
  Serial.print("Attempting to connect to SSID: ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  ThingSpeak.begin(wifiClient);  // Initialize ThingSpeak
  // Attempt to connect to WiFi network:
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    // Connect to WPA/WPA2 network. Change this line if using open or WEP  network:
    // Wait 5 seconds for connection:
    delay(1000);
  }
  Serial.print("\n");
  dht.begin();
}

void loop()
{
  delay(2000);
  ReadDHT11();
  gasSensor();
  flameSensor();
  prediction();
}

/*TEMPERATIRE AND HUMIDITY CALCULATION*/
void ReadDHT11()
{
  h = dht.readHumidity();
  
  t = dht.readTemperature();
  
  float f = dht.readTemperature(true);
  
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);
  
  
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.print(hif);
  Serial.println(F("°F"));
  ThingSpeak.writeField(myChannelNumber, 2, t, myWriteAPIKey);
  ThingSpeak.writeField(myChannelNumber, 3, h, myWriteAPIKey);

  if(temp==0)
  {
    t_curr= dht.readTemperature();
    t_mild=t_curr+ 2.0 ;
    t_moderate=t_mild+3.0;
    t_severe=t_moderate+2.0;
    temp=1;
  }
}
/*****************************************************************************/
/*MQ2 SENSOR*/
void gasSensor()
{
  sensorValue1 = analogRead(MQ_PIN); // read analog input pin 0
  ThingSpeak.writeField(myChannelNumber, 4, sensorValue1, myWriteAPIKey);
  
  sensorValue = digitalRead(D0); // read analog input pin 0
  Serial.print("Sensor Value: ");
  Serial.print(sensorValue1);

  if (sensorValue1 > 400)
  {
    Serial.print(" | Smoke detected!");
  }
}

/*****************************************************************************/
/*FLAME SENSOR*/

void flameSensor() {

  flame = digitalRead(flame_sensor); //reading the sensor data on A0 pin
  ThingSpeak.writeField(myChannelNumber, 5, flame, myWriteAPIKey);
  if (flame == 0)
  {
    Serial.print("\nALERT FIRE DETECTED\n");
  }
  else
  {
    Serial.print("\nNO FIRE DETECTED\n");
  }
}

void Buzzerfn()
{
  pinMode(D2,OUTPUT);
  digitalWrite(D2, HIGH);
  delay(200);
  digitalWrite(D2, LOW);
  delay(200);
}

void prediction()
{
  if(flame==0 || t>t_severe || (t>t_moderate && sensorValue==1))
  {
    Status1="Severe";
    Serial.print("Severe");
    digitalWrite(buzzer, HIGH);
  }
  else if(t>=t_moderate || (t>t_mild && sensorValue==1))
  {
    Status1="Moderate";
    Serial.print("Moderate");
    Buzzerfn();
  }
  else if(t>=t_mild || sensorValue==1)
  {
    Status1="Mild";
    Serial.print("Mild");
    Buzzerfn();
  }
  else
  {
    Status1="No Forest fire";
    Serial.print("No Forest fire");
    pinMode(D2,INPUT);
  }
  
 float conf=classify(t,sensorValue,flame);
 Serial.print(" chance of forest fire in %=");
 Prob=conf*100;
 ThingSpeak.writeField(myChannelNumber, 1, Prob, myWriteAPIKey);
 Serial.println(Prob);
 Serial.print("\n");
 if(Prob>50)
 {
    WiFiClient client;

      client.print(String("GET ") + resource + 
                      " HTTP/1.1\r\n" +
                      "Host: " + server + "\r\n" + 
                      "Connection: close\r\n\r\n");
                  
      int timeout = 5 * 10; // 5 seconds             
      while(!!!client.available() && (timeout-- > 0))
      {
        delay(100);
      }
      while(client.available())
      {
        Serial.write(client.read());
      }
      client.stop();
 }
 else 
 {
      return;
 }
}

 
float trimf(float x,float a, float b, float c){
  float f;
  if(x<=a)
     f=0;
  else if((a<=x)&&(x<=b)) 
     f=(x-a)/(b-a);
  else if((b<=x)&&(x<=c)) 
     f=(c-x)/(c-b);
  else if(c<=x) 
     f=0;
  return f; 
}
float trimf1(float x,float a, float b, float c){
  float f;
  if(x>=b || x>=c) return 0;
  else if(x>a) return (x-a)/(b-a);

  if(x==a) return 0.001;
  
}
// Function for predicting atmospheric comfort from temperature and relative humidity
float classify(float T,float smoke,float flame ){
  float C1 = 1; //severe 
  float C2 = 0.66;//moderate 
  float C3 = 0.33; //mild
  
  if(T>t_severe) T=t_severe; // if temperature is above 35°C, set temperature to maximum in our range
  if(flame==0) flame=1;
  else flame=0;
  
  // Fuzzy rules
  float w1 = trimf(T,t_moderate,t_severe,t_severe);//severe
  float w2 = trimf(T,t_mild,t_moderate,t_severe);//moderate
  float w3 = trimf(T,t_mild,t_mild,t_moderate);//mild
  float w4 = trimf1(T,t_curr,t_mild,t_moderate);//mild
 
    // Defuzzyfication
  float z = (w1*C1 + w2*C2 + w3*C3+ w4*C3*w4)/(w1+w2+w3+w4);
  if(Status1== "Severe" && z<=0.80)
  {
    z=0.99;
    return z;
  }
  else if(Status1== "Moderate" && z<=0.66)
  {
    z=0.6789;
    return z;
  }
  
  else if(Status1== "Mild" && z<0.33)
  {
    z=0.3769;
    return z;
  }
  return z;
}
