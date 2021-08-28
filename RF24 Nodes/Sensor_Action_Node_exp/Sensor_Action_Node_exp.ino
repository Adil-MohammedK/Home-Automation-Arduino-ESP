#include <Arduino.h>

#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <DHT.h>

// The DHT data line is connected to pin 2 on the Arduino
#define DHTPIN 2

// Leave as is if you're using the DHT11. Change if not.
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302)
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);

// PIR variables
byte pirPin = 9;
int pirCalibrationTime = 20;
// OUTPUT pins
byte ledPin = 3;
byte relayPin = 5;
byte button = 6;
// Status of Message
byte status_msg = 1;

// Photocell variable
byte photocellPin = A3;

// Magnetic Door Sensor variable
byte switchPin = 4;

// Radio with CE & CSN connected to pins 7 & 8
RF24 radio(7, 8);
RF24Network network(radio);

// Constants that identify this node and the node to send data to
const uint16_t this_node = 1;
const uint16_t parent_node = 0;

// Time between packets (in ms)
const unsigned long interval = 1000;  // every sec

// Structure of our message
struct sensor {
  float temperature;
  float humidity;
  int light;
  bool motion;
  bool dooropen;
};
sensor message_sen;
// Structure for Action
struct action {
  bool state;
  uint16_t obj;
};
action message_act;

// The network header initialized for this node
RF24NetworkHeader header(parent_node);

void setup(void)
{
  // Set up the Serial Monitor
  Serial.begin(9600);

  // Initialize all radio related modules
  SPI.begin();
  radio.begin();
  delay(5);
  network.begin(90, this_node);

  // Initialize the DHT library
  dht.begin();

  // Activate the internal Pull-Up resistor for the door sensor
  pinMode(switchPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(button, OUTPUT);

  // Calibrate PIR
  pinMode(pirPin, INPUT);
  digitalWrite(pirPin, LOW);
  digitalWrite(relayPin,LOW);
  Serial.print("Calibrating PIR ");
  for (int i = 0; i < pirCalibrationTime; i++)
  {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(" done");
  Serial.println("PIR ACTIVE");
  delay(50);
}

void loop() {

  // Update network data
  network.update();
  // Part for Action
  while (network.available()) {
    RF24NetworkHeader header;
    network.peek(header);
    if (header.type == '2') {
      network.read(header, &message_act, sizeof(message_act));
      Serial.print("Data received from node ");
      Serial.println(header.from_node);
      // Check value and turn the LED on or off
      Serial.print("Value: ");
      Serial.print(message_act.state);
	  Serial.print(" Address: ");
	  Serial.println(message_act.obj);
	  switchState(message_act.obj, message_act.state);
    } else {
      // This is not a type we recognize
      network.read(header, &message_act, sizeof(message_act));
      Serial.print("Unknown message received from node ");
      Serial.println(header.from_node);
    }
    status_msg=0;
  }

    // Read humidity (percent)
    float h = dht.readHumidity();
    // Read temperature as Celsius
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit
    float f = dht.readTemperature(true);

    // Read photocell
    int p = analogRead(photocellPin);
    // Testing revealed this value never goes below 50 or above 1000,
    //  so we're constraining it to that range and then mapping that range
    //  to 0-100 so it's like a percentage
    p = constrain(p, 50, 1000);
    p = map(p, 50, 1000, 0, 100);

    // Read door sensor: HIGH means door is open (the magnet is far enough from the switch)
    bool d = (digitalRead(switchPin) == HIGH);

    // Read motion: HIGH means motion is detected
    bool m = (digitalRead(pirPin) == HIGH);

    // Headers will always be type 1 for this node
    // We set it again each loop iteration because fragmentation of the messages might change this between loops
    header.type = '1';

    // Only send values if any of them are different enough from the last time we sent:
    //  0.5 degree temp difference, 1% humdity or light difference, or different motion state
    if (abs(f - message_sen.temperature) > 0.5 ||
        abs(h - message_sen.humidity) > 1.0 ||
        abs(p - message_sen.light) > 1.0 ||
        m != message_sen.motion ||
        d != message_sen.dooropen) {
      // Construct the message we'll send
      message_sen = (sensor) { t, h, p, m, d
};

      // Writing the message to the network means sending it
      if (network.write(header, &message_sen, sizeof(message_sen))) {
        Serial.print("Message sent\n");
      } else {
        Serial.print("Could not send message\n");
      }
    }

  // Wait a bit before we start over again
  status_msg=1;
  delay(interval);
}

void switchState(int addr, bool state)
{
  switch (addr) {
    case 768:
    digitalWrite(ledPin,state);
    break;
    case 1280:
    digitalWrite(relayPin,state);
    break;
  }

}
