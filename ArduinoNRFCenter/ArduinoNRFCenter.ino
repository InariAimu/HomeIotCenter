/*
 Name:		ArduinoNRFCenter.ino
 Created:	2017/3/17 15:52:15
 Author:	Asuka
*/

#include <Adafruit_NeoPixel.h>


#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfSpiDriver.h>
#include <MirfHardwareSpiDriver.h>

#define NRF_DEVICE_ADDR "iotc1"
const uint8_t PIN_NRF_CE = 4;
const uint8_t PIN_NRF_CSN = 5;
const uint8_t PayLoad_Length = 32;

typedef struct _NrfPacket
{
	uint8_t Id;
	uint8_t Type;
	uint8_t Options;
	uint8_t Payload[29];
}*NrfPacket;

typedef struct _WeatherStationData
{
	uint8_t		Device_Id;
	uint8_t		Packet_Id;
	uint16_t	BatteryVolt_Raw;
	uint16_t	RainVolt_Raw;
	double		DustVolt;
	uint16_t	Brightness;
	double		AirPressure;
	double		Temperature;
	uint8_t		Humidity;

}*WeatherStationData;


#include <Wire.h>

const uint8_t I2C_INT_PIN = 6;
volatile uint8_t I2C_STAT = 0;
String I2C_Buffer;


// the setup function runs once when you press reset or power the board
void setup() 
{
	Serial.begin(9600);

	pinMode(I2C_INT_PIN, OUTPUT);
	digitalWrite(I2C_INT_PIN, HIGH);

	Mirf.spi = &MirfHardwareSpi;
	Mirf.cePin = PIN_NRF_CE;
	Mirf.csnPin = PIN_NRF_CSN;
	Mirf.init();
	Mirf.setRADDR((byte *)NRF_DEVICE_ADDR);
	Mirf.payload = PayLoad_Length;
	Mirf.config();
	Mirf.configRegister(EN_AA, 0);
	delay(100);

	Wire.begin(0x08);
	Wire.onReceive(I2C_OnReceive);
	Wire.onRequest(I2C_OnRequest);
	delay(100);

	Serial.print("Init");
}

// the loop function runs over and over again until power down or reset
void loop()
{
	if (Serial.available())
	{
		I2C_Buffer = Serial.readStringUntil('\n');
		Serial.println(I2C_Buffer);
		Send_I2C();
	}

	if (!Mirf.isSending() && Mirf.dataReady())
	{
		byte data[PayLoad_Length];
		Mirf.getData(data);

		WeatherStationData wsd = (WeatherStationData)data;

		double batteryVoltage = (double)wsd->BatteryVolt_Raw * 5 / 1024;

		Serial.print("Device:");
		Serial.print(wsd->Device_Id);
		Serial.print(" PID:");
		Serial.print(wsd->Packet_Id);
		Serial.print(" ");
		Serial.print(batteryVoltage);
		Serial.print("V ");
		Serial.println(wsd->Temperature);

		Serial.print(wsd->AirPressure);
		Serial.print("hPa ");
		Serial.print(wsd->Humidity);
		Serial.print("%RH ");
		Serial.print(wsd->DustVolt);
		Serial.print("V ");
		Serial.print(wsd->Brightness);
		Serial.println("Lux");
		Serial.println();
	}
}

void Send_I2C()
{
	digitalWrite(I2C_INT_PIN, LOW);
	delay(1);
	digitalWrite(I2C_INT_PIN, HIGH);
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
