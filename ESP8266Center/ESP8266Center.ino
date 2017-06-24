/*
 Name:		ESP8266Center.ino
 Created:	2017/4/23 1:43:09
 Author:	Asuka
*/

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
#include <WiFiServer.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>

#include <Wire.h>

volatile uint8_t I2C_INT_STAT = 0;
String I2C_Buffer;

uint8_t data[32];
int value = 0;

WiFiClient client;
String SSID = "Kaname_Aimu";
String Password = "iAsuka1204";

const char* device_id = "964";
const char* api_id = "29f1a5679";
const char* host = "www.bigiot.net";

typedef struct _WeatherStationData
{
	uint8_t		Device_Id;
	uint8_t		Packet_Id;
	uint16_t	BatteryVolt_Raw;
	uint16_t	RainVolt_Raw;
	uint32_t	DustVolt;
	uint16_t	Brightness;
	uint32_t	AirPressure;
	uint32_t	Temperature;
	uint8_t		Humidity;

}*WeatherStationData;


// the setup function runs once when you press reset or power the board
void setup() 
{
	Serial.begin(115200);

	pinMode(D5, INPUT);
	attachInterrupt(D5, D5_Int, FALLING);

	Wire.pins(D1, D2);
	Wire.begin();

	delay(5000);

	InitWiFi();

	int retry = 0;
	while (!InitBigIoT())
	{
		delay(10000);
		Serial.print("retry times:");
		Serial.println(++retry);

		I2C_Println(String("retry times:") + retry);
	}

	interrupts();
}

// the loop function runs over and over again until power down or reset
void loop()
{
	bool validData = GetWeatherDataFromI2C();

	WeatherStationData wsd = (WeatherStationData)data;

	double batteryVoltage = (double)wsd->BatteryVolt_Raw * 5 / 1023;
	double rainVoltage = (double)(1023 - wsd->RainVolt_Raw) * 5 / 1023;

	delay(100);

	if (!client.connected())
	{
		Serial.println("lost connection, reconnecting...");
		I2C_Println("lost connection, reconnecting...");
		int retry = 0;
		while (!InitBigIoT())
		{
			delay(10000);
			Serial.print("retry times:");
			Serial.println(++retry);

			I2C_Println(String("retry times:") + retry);
		}
	}
	Serial.println(String("{\"M\":\"update\",\"ID\":\"") + device_id + "\"," +
		"\"V\":{" +
		"\"1017\":\"" + String(*(float*)(&wsd->AirPressure), 2) + "\"," +
		"\"904\":\"" + String(*(float*)(&wsd->Temperature), 2) + "\"," +
		"\"1018\":\"" + wsd->Brightness + "\"," +
		"\"1019\":\"" + wsd->Humidity + "\"," +
		"\"1022\":\"" + rainVoltage + "\"," +
		"\"1024\":\"" + *(float*)(&wsd->DustVolt) + "\"," +
		"\"1025\":\"" + batteryVoltage + "\"" +
		"}}");
	client.println(String("{\"M\":\"update\",\"ID\":\"") + device_id + "\"," +
		"\"V\":{" +
		"\"1017\":\"" + String(*(float*)(&wsd->AirPressure), 2) + "\"," +
		"\"904\":\"" + String(*(float*)(&wsd->Temperature), 2) + "\"," +
		"\"1018\":\"" + wsd->Brightness + "\"," +
		"\"1019\":\"" + wsd->Humidity + "\"," +
		"\"1022\":\"" + rainVoltage + "\"," +
		"\"1024\":\"" + *(float*)(&wsd->DustVolt) + "\"," +
		"\"1025\":\"" + batteryVoltage + "\"" +
		"}}");
	client.flush();
	delay(10);
	++value;
	Serial.print(value);
	Serial.println(" data sent");
	I2C_Println(String("") + value + " data sent");

	unsigned long startTime = millis();
	while (millis() - startTime < 40000) {
		String line;
		if (client.available())
		{
			while (client.available()) {
				line = client.readStringUntil('\n');
				Serial.print(line);
				I2C_Println(line);
			}
			Serial.println();
		}


		if (I2C_INT_STAT == 1)
		{
			Deal_I2C_Command();
			I2C_INT_STAT = 0;
		}

		/*I2C_Buffer = String("main:") + (millis() / 1000) + '\n';
		Serial.print(I2C_Buffer);
		I2C_Send();*/
	}

	//delay(1000);
}

bool GetWeatherDataFromI2C()
{
	I2C_SendCommand('S');
	delay(1);
	//get raw data via I2C
	Wire.requestFrom(0x08, 32);
	for (int i = 0; i < 32; i++)
	{
		data[i] = Wire.read();
		Serial.print(data[i]);
		Serial.print(' ');
	}
	Serial.println();
	return true;
}

void Deal_I2C_Command()
{
	I2C_SendCommand('R');

	Wire.requestFrom(0x08, 2);
	while (!Wire.available())
	{

	}
	int n = Wire.read() + Wire.read() * 256;

	Serial.print(n);
	Serial.print(">");

	Wire.requestFrom(0x08, n);
	while (!Wire.available())
	{

	}
	char c = Wire.read();
	I2C_Buffer = Wire.readStringUntil('\n');
	I2C_Buffer.trim();
	Serial.println(I2C_Buffer);

}

bool InitBigIoT()
{
	Serial.print("Connecting to BigIot: ");
	Serial.println(host);

	I2C_Println(String("Connecting to BigIot: ") + host);

	//Init TCP connection to bigiot
	const int httpPort = 8181;
	if (!client.connect(host, httpPort)) {
		Serial.println("Connection failed");
		I2C_Println("Connection failed");
		return false;
	}

	//Wait until connected
	unsigned long timeout = millis();
	while (client.available() == 0) {
		if (millis() - timeout > 5000) {
			Serial.println(">>> Client Timeout");
			I2C_Println(">>> Client Timeout");
			client.stop();
			return false;
		}
	}

	//Receive a {"M":"WELCOME TO BIGIOT"}\n message
	//just receive and print via serial
	while (client.available()) {
		String line = client.readStringUntil('\n');
		Serial.print(line);
		I2C_Println(line);
	}
	Serial.println("connected");
	I2C_Println("connected");

	delay(1000 * 2);

	//Login using DEVICE_ID and API_ID
	Serial.println("Process Login"); 
	I2C_Println("Process Login");
	client.println(String("{\"M\":\"checkin\",\"ID\":\"") + device_id + "\"," +
		"\"K\":\"" + api_id + "\"}");
	client.flush();
	timeout = millis();
	while (client.available() == 0) {
		if (millis() - timeout > 5000) {
			Serial.println(">>> Client Timeout or already logged!!");
			I2C_Println(">>> Client Timeout or already logged!!");
			client.stop();
			delay(1000 * 30);
			return false;
		}
	}

	//Receive a {"M":"checkinok" message
	while (client.available()) {
		String line = client.readStringUntil('\n');
		Serial.print(line);
		I2C_Println(line);
	}

	//If things goes ok we have sucessfully logged into bigiot
	Serial.println();
	Serial.println("check in OK");
	I2C_Println("check in OK");
	delay(5000);
	return true;
}

void InitWiFi()
{
	WiFi.disconnect(true);
	delay(1000);
	WiFi.onEvent(WiFiEvent);

	Serial.println();
	Serial.print("Connecting to ");
	Serial.print(SSID);
	Serial.print(",");
	Serial.println(Password);

	I2C_Println(String("Connecting to ") + SSID + "," + Password);

	WiFi.begin(SSID.c_str(), Password.c_str());

	while (WiFi.status() != WL_CONNECTED) 
	{
		delay(500);
		Serial.print(".");
	}

	/*Serial.println("");
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());*/
}

void WiFiEvent(WiFiEvent_t event) {
	//Serial.printf("[WiFi-event] event: %d\n", event);

	switch (event) {
	case WIFI_EVENT_STAMODE_GOT_IP:
		Serial.println("WiFi connected");
		Serial.println("IP address: ");
		Serial.println(WiFi.localIP().toString());

		I2C_Println("WiFi connected");
		I2C_Println(String("IP address: ") + WiFi.localIP().toString());

		//I2C_Buffer = String("OK\n");
		//I2C_SendBuff();
		break;
	case WIFI_EVENT_STAMODE_DISCONNECTED:
		Serial.println("WiFi lost connection");
		I2C_Println("WiFi lost connection");
		delay(10000);
		InitWiFi();
		break;
	}
}

void I2C_SendCommand(uint8_t c)
{
	Wire.beginTransmission(0x08);
	Wire.write(c);
	Wire.endTransmission();
}

void I2C_SendBuff()
{
	Wire.beginTransmission(0x08);
	Wire.write('W');
	Wire.print(I2C_Buffer);
	Wire.endTransmission();
}

void I2C_Println(String s)
{
	Wire.beginTransmission(0x08);
	Wire.write('P');
	s.trim();
	if (s.length() > 128)
	{
		s = s.substring(0, 128);
	}
	Wire.println(s);
	Wire.endTransmission();
}

void D5_Int()
{
	I2C_INT_STAT = 1;
}
