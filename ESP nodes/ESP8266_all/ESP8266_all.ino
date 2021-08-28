

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include "DHT.h"

#define DHTPIN 12
#define DHTTYPE DHT11  // change to DHT11 if that's what you have

float rssi;                  // To measure signal strenght
int rssiinterval = 10000;     // ms interval between checkups (reality is longer as rest of code takes more time).
long lastrssi = 0;           // counter for non-interupt measurement


int checkinterval = 15000;   // ms interval between checkups (reality is longer as rest of code takes more time), smaller then 6 seconds makes it unstable.
long lastMsg = 0;            // counter for non-interupt measurement

      //4 momentary buttons
      #define button1 0   //D3
      int button1State = 0;
      #define button2 2   //D4
      int button2State = 0;
      #define button3 14   //D5
      int button3State = 0;
 //     #define button4 12   //D6
  //    int button4State = 0;

DHT dht(DHTPIN, DHTTYPE, 15);

float temp_c;


// Update these with values suitable for your network.
const char* ssid  = "Brahmaputra";
const char* password = "Brahmaputra123";
const char* mqtt_server = "10.22.9.71";
int mqttport = 1883;
#define CLIENT_ID   "ESP8266"                    //has to be unique in your system
#define MQTT_TOPIC  "home/btn"         //has to be identical with home assistant configuration.yaml

WiFiClient espClient;
PubSubClient client(espClient);
      
//RGB LED
#define relay1 5  //D1
#define relay2 4  //D2
#define relay3 15  //D8
#define relay4 13  //D7
const char* topic_relay1 = MQTT_TOPIC"/relay1/set";
const char* topic_relay2 = MQTT_TOPIC"/relay2/set";
const char* topic_relay3 = MQTT_TOPIC"/relay3/set";
const char* topic_relay4 = MQTT_TOPIC"/relay4/set";
int light = 0;
 
void setup(){
  Serial.begin(9600);
  Serial.println("setup begin");
  pinMode(LED_BUILTIN, OUTPUT);
  
       
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay4, OUTPUT);
  delay(10);
  digitalWrite(relay1, LOW);           //start with off (I used SSR, if you use regular then you should use LOW)
  digitalWrite(relay2, LOW);           //start with off (I used SSR, if you use regular then you should use LOW)
  digitalWrite(relay3, LOW);           //start with off (I used SSR, if you use regular then you should use LOW)
  digitalWrite(relay4, LOW);           //start with off (I used SSR, if you use regular then you should use LOW)
  digitalWrite(LED_BUILTIN, HIGH);

       // test relays for 1 second
       digitalWrite(LED_BUILTIN, LOW);
       delay(1000);
       digitalWrite(LED_BUILTIN, HIGH);
       digitalWrite(relay1, HIGH);
       delay(1000);
       digitalWrite(relay1, LOW);
       digitalWrite(relay2, HIGH);
       delay(1000);
       digitalWrite(relay2, LOW);
       digitalWrite(relay3, HIGH);
       delay(1000);
       digitalWrite(relay3, LOW);
       digitalWrite(relay4, HIGH);
       delay(1000);
       digitalWrite(relay4, LOW);

       pinMode(button1, INPUT_PULLUP);
       pinMode(button2, INPUT_PULLUP);
       pinMode(button3, INPUT_PULLUP);
 //      pinMode(button4, INPUT_PULLUP);

  //start wifi and mqqt
  setup_wifi(); 
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // OTA setup
   ArduinoOTA.setHostname(CLIENT_ID);
   ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  dht.begin();

  Serial.println("setup end");
}

void loop(){
  ArduinoOTA.handle();

  // Connect to Wifi if not connected
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
    reconnect();
    client.setServer(mqtt_server, mqttport);
    client.setCallback(callback);
  } else {
    //if connected get RSSI (Wifi signal strengt) from ESP8266 and publishes it to MQTT
    checkrssi();
  }

  // reconnect when MQTT is not connected
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  //listen to buttons
        int debugserial = 1;    // make 1 if you want to see the button status in serial monitor
        button1State = digitalRead(button1); 
        if (button1State == LOW) {
          digitalWrite(LED_BUILTIN, LOW); 
          client.publish(MQTT_TOPIC"/btn1", "1", true);
          delay(200); // delay to prevent double click
          digitalWrite(LED_BUILTIN, HIGH);
        }
        if (debugserial == 1){Serial.print("1 = ");Serial.print(button1State);}

         // read button press
        button2State = digitalRead(button2); 
        if (button2State == LOW) {
          digitalWrite(LED_BUILTIN, LOW); 
          client.publish(MQTT_TOPIC"/btn2", "1", true);
          delay(200); // delay to prevent double click
          digitalWrite(LED_BUILTIN, HIGH); 
        } 
        if (debugserial == 1){Serial.print(" - 2 = ");Serial.print(button2State);}

         // read button press
        button3State = digitalRead(button3); 
        if (button3State == LOW) {
          digitalWrite(LED_BUILTIN, LOW); 
          client.publish(MQTT_TOPIC"/btn3", "1", true);
          delay(200); // delay to prevent double click
          digitalWrite(LED_BUILTIN, HIGH); 
        }
        if (debugserial == 1){Serial.print(" - 3 = ");Serial.print(button3State);}

        // Read temperature as Celsius
         temp_c = dht.readTemperature();
         char temp=char(temp_c);

         // Check if any reads failed and exit early (to try again).
          if ( isnan(temp)) {
             Serial.println("Failed to read from DHT sensor!");
             return;
           }

         // Computes temperature values in Celsius
       //  float hic = dht.computeHeatIndex(temp_c, h, false);
         static char temperatureTemp[7],lastTemp[7]="0";
         dtostrf(temp_c, 6, 2, temperatureTemp);
         
         if(strcmp(lastTemp,temperatureTemp)!=0)
         {
         digitalWrite(LED_BUILTIN, LOW); 
          client.publish(MQTT_TOPIC"/btn4", temperatureTemp, true);
          delay(200); // delay to prevent double click
          digitalWrite(LED_BUILTIN, HIGH); 
          strcpy(lastTemp,temperatureTemp);
         }
          if (debugserial == 1){Serial.print(" - temp = ");Serial.println(temperatureTemp);}

    /*    // read button press
        button4State = digitalRead(button4); 
        if (button4State == LOW) {
          digitalWrite(LED_BUILTIN, LOW); 
          client.publish(MQTT_TOPIC"/btn4", "1", true);
          delay(200); // delay to prevent double click
          digitalWrite(LED_BUILTIN, HIGH); 
        }   
        if (debugserial == 1){Serial.print(" - 4 = ");Serial.println(button4State);}
      */
  
}


void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid); 
  WiFi.mode(WIFI_STA); //ESP in connect to network mode
  WiFi.begin(ssid, password); 

  while (WiFi.status() != WL_CONNECTED) {
    // wait 50 * 10 ms = 500 ms to give ESP some time to connect (need to give delay else you get soft WDT reset)
      for (int i=0; i <= 50; i++){
        delay(10);
        if (i == 50) { 
          Serial.println(".");
        } else {
          Serial.print(".");
        }     
      }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  digitalWrite(LED_BUILTIN, HIGH);           //LED off
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  //switch on relay1 LED if topic is correct and 1 is received
  if (strcmp(topic, topic_relay1)==0){      
    if ((char)payload[0] == '1') {
      digitalWrite(relay1, HIGH);                                  //on  (you might need to change this for regular relays, I used SSR)
      client.publish(MQTT_TOPIC"/relay1", "1", true);
    } else {
      digitalWrite(relay1, LOW);                                 //off  (you might need to change this for regular relays, I used SSR)
      client.publish(MQTT_TOPIC"/relay1", "0", true);
    }
  }

  //switch on relay2 LED if topic is correct and 1 is received
  if (strcmp(topic, topic_relay2)==0){
    if ((char)payload[0] == '1') {
      digitalWrite(relay2, HIGH);                                  //on  (you might need to change this for regular relays, I used SSR)
      client.publish(MQTT_TOPIC"/relay2", "1", true);
    } else {
      digitalWrite(relay2, LOW);                                 //off  (you might need to change this for regular relays, I used SSR)
      client.publish(MQTT_TOPIC"/relay2", "0", true);
    }
  }

  //switch on relay3 LED if topic is correct and 1 is received
  if (strcmp(topic, topic_relay3)==0){
    if ((char)payload[0] == '1') {
      client.publish(MQTT_TOPIC"/relay3", "1", true);
      digitalWrite(relay3, HIGH);                                  //on  (you might need to change this for regular relays, I used SSR)
    } else {
      client.publish(MQTT_TOPIC"/relay3", "0", true);
      digitalWrite(relay3, LOW);                                 //off  (you might need to change this for regular relays, I used SSR)
    }
  }

   //switch on relay4 LED if topic is correct and 1 is received
  if (strcmp(topic, topic_relay3)==0){
    if ((char)payload[0] == '1') {
      client.publish(MQTT_TOPIC"/relay4", "1", true);
      digitalWrite(relay3, HIGH);                                  //on  (you might need to change this for regular relays, I used SSR)
    } else {
      client.publish(MQTT_TOPIC"/relay4", "0", true);
      digitalWrite(relay3, LOW);                                 //off  (you might need to change this for regular relays, I used SSR)
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  digitalWrite(LED_BUILTIN, LOW);     //LED on
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(CLIENT_ID)) {
      Serial.println("connected");
      client.subscribe(MQTT_TOPIC"/relay1/set");    //subscribe relay to MQTT server, you can set your name here
      client.subscribe(MQTT_TOPIC"/relay2/set");    //subscribe relay to MQTT server, you can set your name here
      client.subscribe(MQTT_TOPIC"/relay3/set");    //subscribe relay to MQTT server, you can set your name here
      client.subscribe(MQTT_TOPIC"/relay4/set");    //subscribe relay to MQTT server, you can set your name here
      digitalWrite(LED_BUILTIN, HIGH);           //LED off
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      // Wait (500 * 10ms = 5000ms) 5 seconds before retrying, but read button press meanwhile
      for (int i=0; i <= 500; i++){
        delay(10);
        if (i == 50) { 
          Serial.println(".");
        } else {
          Serial.print(".");
        }     
      }     
    }
  }
}

void checkrssi(){
  long nowrssi = millis();                  // only check every x ms  checkinterval
  if (nowrssi - lastrssi > rssiinterval) {
    lastrssi = nowrssi;
    // measure RSSI
    float rssi = WiFi.RSSI();

    // make RSSI -100 if long rssi is not between -100 and 0
    if (rssi < -100 && rssi > 0){
      rssi = -110;
    }
    
    Serial.print("RSSI = ");
    Serial.println(rssi);

    if (client.connected()) {
      client.publish(MQTT_TOPIC"/rssi", String(rssi).c_str(), false);   
    }
  }
}
