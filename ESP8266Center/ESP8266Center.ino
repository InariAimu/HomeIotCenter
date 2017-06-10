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

#define I2C_COM_Millis '0'
#define I2C_COM_SSID 'A'
#define I2C_COM_Password 'B'
#define I2C_COM_DeviceID 'C'
#define I2C_COM_APIKey 'D'
#define I2C_COM_Host 'E'

#define I2C_COM_InitWiFi 'a'
#define I2C_COM_RequestIP 'b'

WiFiClient client;
String SSID;
String Password;

// the setup function runs once when you press reset or power the board
void setup() 
{
	Serial.begin(9600);

	pinMode(D5, INPUT);
	attachInterrupt(D5, D5_Int, FALLING);

	Wire.pins(D1, D2);
	Wire.begin();

	interrupts();
}

// the loop function runs over and over again until power down or reset
void loop()
{
	if (I2C_INT_STAT == 1)
	{
		Deal_I2C_Command();
		I2C_INT_STAT = 0;
	}

	/*I2C_Buffer = String("main:") + (millis() / 1000) + '\n';
	Serial.print(I2C_Buffer);
	I2C_Send();

	delay(1000);*/
}

void Deal_I2C_Command()
{
	Wire.beginTransmission(0x08);
	Wire.write('R');
	Wire.endTransmission();

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

	switch (c)
	{
	case I2C_COM_Millis:
		I2C_Buffer = String("") + millis() + '\n';
		I2C_Send();
		break;

	case I2C_COM_SSID:
		SSID=I2C_Buffer;
		I2C_Buffer = String("OK\n");
		I2C_Send();
		Serial.print("Set SSID=");
		Serial.println(SSID);
		break;

	case I2C_COM_Password:
		Password = I2C_Buffer;
		Serial.println(I2C_Buffer.length());
		I2C_Buffer = String("OK\n");
		I2C_Send();
		Serial.print("Set Pwd=");
		Serial.println(Password);
		break;

	case I2C_COM_InitWiFi:
		InitWiFi();
		break;

	default:
		I2C_Buffer = String("error\n");
		I2C_Send();
		break;
	}
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
	Serial.printf("[WiFi-event] event: %d\n", event);

	switch (event) {
	case WIFI_EVENT_STAMODE_GOT_IP:
		Serial.println("WiFi connected");
		Serial.println("IP address: ");
		Serial.println(WiFi.localIP().toString());
		I2C_Buffer = String("OK\n");
		I2C_Send();
		break;
	case WIFI_EVENT_STAMODE_DISCONNECTED:
		Serial.println("WiFi lost connection");
		delay(10000);
		InitWiFi();
		break;
	}
}

void I2C_Send()
{
	Wire.beginTransmission(0x08);
	Wire.write('W');
	Wire.print(I2C_Buffer);
	Wire.endTransmission();
}

void D5_Int()
{
	I2C_INT_STAT = 1;
}
