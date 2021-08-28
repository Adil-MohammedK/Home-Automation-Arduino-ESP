#include <DHT.h>
#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>

// Long leg of LED is connected to pin 3 on Arduino
// Short leg of LED is connected to ground via 220 ohm resistor
byte ledPin = 3;

// Radio with CE & CSN connected to pins 7 & 8
RF24 radio(7, 8);
RF24Network network(radio);

// Constant that identify this node
const uint16_t this_node = 2;

// Time between packets (in ms)
const unsigned long interval = 1000;  // every sec

// Structure of our message
struct message_action {
  bool state;
};
message_action message;


void setup(void)
{
  // Set up the Serial Monitor
  Serial.begin(9600);
  Serial.print("");

  // Initialize all radio related modules
  SPI.begin();
  radio.begin();
  delay(5);
  network.begin(90, this_node);

  // Configure LED pin
  pinMode(ledPin, OUTPUT);
}

void loop() {

  // Update network data
  network.update();

  while (network.available()) {
    RF24NetworkHeader header;
    message_action message;
    network.peek(header);
    if (header.type == '2') {
      network.read(header, &message, sizeof(message));
      Serial.print("Data received from node ");
      Serial.println(header.from_node);
      // Check value and turn the LED on or off
      Serial.print("Value: ");
      Serial.println(message.state);
      if (message.state) {
        digitalWrite(ledPin, HIGH);
      } else {
        digitalWrite(ledPin, LOW);
      }
    } else {
      // This is not a type we recognize
      network.read(header, &message, sizeof(message));
      Serial.print("Unknown message received from node ");
      Serial.println(header.from_node);
    }
  }

  // Wait a bit before we start over again
  delay(interval);
}
