/*
 Name:		ArduinoNRFCenter.ino
 Created:	2017/3/17 15:52:15
 Author:	Asuka
*/

#include <Adafruit_BMP280.h>
#include <nRF24L01.h>
#include <MirfSpiDriver.h>
#include <MirfHardwareSpiDriver.h>
#include <Mirf.h>

#include <Wire.h>

const uint8_t NRF_CE_PIN = 4;
const uint8_t NRF_CSN_PIN = 5;

#define NRF_DEVICE_ADDR "cen00"

typedef struct _NrfPacket
{
	uint8_t Id;
	uint8_t Type;
	uint8_t Options;
	uint8_t Payload[29];
}*NrfPacket;

const uint8_t I2C_INT_PIN = 6;
volatile uint8_t I2C_STAT = 0;
String I2C_Buffer;

// the setup function runs once when you press reset or power the board
void setup() 
{
	Serial.begin(9600);

	pinMode(4, OUTPUT);
	digitalWrite(4, HIGH);

	Wire.begin(0x08);
	Wire.onReceive(I2C_OnReceive);
	Wire.onRequest(I2C_OnRequest);

	Serial.print("Init");
}

// the loop function runs over and over again until power down or reset
void loop()
{
	/*I2C_Buffer = String("arduino:") + (millis()/1000) + '\n';
	Serial.print(I2C_Buffer);
	Send_I2C();

	delay(1000);*/
	if (Serial.available())
	{
		I2C_Buffer = Serial.readStringUntil('\n');
		Serial.println(I2C_Buffer);
		Send_I2C();
	}
}

void Send_I2C()
{
	digitalWrite(4, LOW);
	delay(1);
	digitalWrite(4, HIGH);
}

void I2C_OnReceive(int n)
{
	while (!Wire.available())
	{
	}
	char c = Wire.read();
	switch (c)
	{
	case 'R':
		I2C_STAT = 1;
		break;

	case 'W':
		if (Wire.available())
		{
			I2C_Buffer = Wire.readStringUntil('\n');
		}
		Serial.println(I2C_Buffer);
		break;

	default:
		break;
	}

}

void I2C_OnRequest()
{
	if (I2C_STAT == 0)
	{
		Wire.print(I2C_Buffer);
	}
	else if (I2C_STAT == 1)
	{
		uint8_t lh = I2C_Buffer.length() / 256;
		uint8_t ll = I2C_Buffer.length() % 256;
		Wire.write(ll);
		Wire.write(lh);
		I2C_STAT = 0;
	}
}
