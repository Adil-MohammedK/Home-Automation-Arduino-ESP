#include <ESP8266WiFi.h>

const char* ssid = "your-ssid";
const char* password = "your-password";

//Structure for action
struct message_action
{
	int addr;
	bool state;
};

struct message_sensor
{
	float temperature;
	float humidity;
	byte light;
	bool motion;
	bool dooropen;
};

void setup()
{
	Serial.begin(115200);
	delay(10);

	// We start by connecting to a WiFi network

	Serial.println();
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(ssid);

	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}

	Serial.println("");
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());
  

}

void loop()
{



}
