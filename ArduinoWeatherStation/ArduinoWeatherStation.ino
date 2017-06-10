/*
 Name:		ArduinoWeatherStation.ino
 Created:	2017/6/10 15:02:45
 Author:	Itsuka Kotori
*/


// Low Power library
#include "LowPower.h"

// Battery volt adc pin
#define PIN_BatteryVoltage A0

// Bmp180 for air BMP180_PressureSensor & temp
#include <Wire.h>
#include <SFE_BMP180.h>
SFE_BMP180 BMP180_PressureSensor;

// DS1032 backup temp
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 6
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20_Sensors(&oneWire);

// DHT11 for humidity
#include <dht11.h>
dht11 DHT11;
#define DHT11PIN 9

// NRF24L01 for wireless comm
#include <SPI.h>
#include <Mirf.h>
#include <MirfSpiDriver.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>
#define PIN_NRF_CE 8
#define PIN_NRF_CSN 7
const uint8_t PayLoad_Length = 32;

// GY-30 module  BH1750FVI for light intensity 
// in ADDR 'L' mode 7bit addr
#define GY_30_ADDR 0b0100011
// addr 'H' mode
// #define ADDR 0b1011100

// Adc for rain sensor
#define PIN_RAIN_SENSOR A3

// sharp dust sensor properties
#define PIN_DATA_OUT A1 
#define PIN_LED_VCC 5 
const int DELAY_BEFORE_SAMPLING = 280;
const int DELAY_AFTER_SAMPLING = 40;
const int DELAY_LED_OFF = 9680;


//-------------------------------------------------------------------------------------------------------------------
const uint8_t device_id = 1;
uint8_t packet_id = 0;

void setup() {

	pinMode(PIN_DATA_OUT, INPUT);
	pinMode(PIN_LED_VCC, OUTPUT);

	Mirf.spi = &MirfHardwareSpi;
	Mirf.cePin = PIN_NRF_CE;
	Mirf.csnPin = PIN_NRF_CSN;
	Mirf.init();
	Mirf.setRADDR((byte *)"ws_01");
	Mirf.payload = PayLoad_Length;
	Mirf.config();
	Mirf.configRegister(EN_AA, 0);
	delay(100);

	//
	BMP180_PressureSensor.begin();
	delay(100);

	//
	DS18B20_Sensors.begin();
	delay(100);

	//
	Wire.beginTransmission(GY_30_ADDR);
	Wire.write(0b00000001);
	Wire.endTransmission();
	delay(100);
}

void loop() {
	// put your main code here, to run repeatedly:
	uint8_t data[PayLoad_Length];

	data[0] = device_id;
	data[1] = packet_id;
	packet_id++;

	int battery_value_1 = analogRead(PIN_BatteryVoltage);

	int rain_value = analogRead(PIN_RAIN_SENSOR);
	data[4] = *(((byte *)&rain_value) + 0);
	data[5] = *(((byte *)&rain_value) + 1);

	float dust_volt = getSharpDustSensorVoltage();
	data[6] = *(((byte *)&dust_volt) + 0);
	data[7] = *(((byte *)&dust_volt) + 1);
	data[8] = *(((byte *)&dust_volt) + 2);
	data[9] = *(((byte *)&dust_volt) + 3);

	unsigned int brightness_val = (unsigned int)getGY30Reading();
	data[10] = *(((byte *)&brightness_val) + 0);
	data[11] = *(((byte *)&brightness_val) + 1);

	float pres, temp;
	if (!getBMP180Readings(&pres, &temp))
	{
		pres = 0;
		temp = getDS18B20Temp();
	}
	data[12] = *(((byte *)&pres) + 0);
	data[13] = *(((byte *)&pres) + 1);
	data[14] = *(((byte *)&pres) + 2);
	data[15] = *(((byte *)&pres) + 3);
	data[16] = *(((byte *)&temp) + 0);
	data[17] = *(((byte *)&temp) + 1);
	data[18] = *(((byte *)&temp) + 2);
	data[19] = *(((byte *)&temp) + 3);

	data[20] = getDHT11Humidity();

	int battery_value_2 = analogRead(PIN_BatteryVoltage);
	int battery_value = (battery_value_1 + battery_value_2) / 2;
	data[2] = *(((byte *)&battery_value) + 0);
	data[3] = *(((byte *)&battery_value) + 1);

	for (int i = 21; i < 32; i++)
	{
		data[i] = 0;
	}

	Mirf.setTADDR((byte *)"cen00");
	Mirf.send(data);
	while (Mirf.isSending())
	{
		//wait for data sending
	}
	Mirf.powerDown();

	LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
	LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
	LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
	LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
	LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}


//-------------------------------------------------------------------------------------------------------------------
float getSharpDustSensorVoltage()
{
	digitalWrite(PIN_LED_VCC, LOW);
	delayMicroseconds(DELAY_BEFORE_SAMPLING);
	float analogOutput = analogRead(PIN_DATA_OUT);
	delayMicroseconds(DELAY_AFTER_SAMPLING);
	digitalWrite(PIN_LED_VCC, HIGH);
	//delayMicroseconds(DELAY_LED_OFF);

	float outputV = analogOutput * 5 / 1024;
	return outputV;
}

float getDS18B20Temp()
{
	DS18B20_Sensors.requestTemperatures();
	return DS18B20_Sensors.getTempCByIndex(0);
}

unsigned long getGY30Reading()
{
	unsigned long val = 0;
	Wire.beginTransmission(GY_30_ADDR);
	Wire.write(0b00000111);
	Wire.endTransmission();
	delay(100);

	Wire.beginTransmission(GY_30_ADDR);
	Wire.write(0b00100000);
	Wire.endTransmission();
	// typical read delay 120ms
	delay(120);

	Wire.requestFrom(GY_30_ADDR, 2); // 2byte every time

	for (val = 0; Wire.available() >= 1; )
	{
		char c = Wire.read();
		//Serial3.println(c, HEX);
		val = (val << 8) + (c & 0xFF);
	}
	val = val / 1.2;
	return val;
}

byte getDHT11Humidity()
{
	//int chk = DHT11.read(DHT11PIN);
	DHT11.read(DHT11PIN);
	return (byte)DHT11.humidity;
}

bool getBMP180Readings(float* p, float* temp)
{
	char stat;
	double T, P;
	stat = BMP180_PressureSensor.startTemperature();
	if (stat != 0)
	{
		delay(stat);
		stat = BMP180_PressureSensor.getTemperature(T);
		if (stat != 0)
		{
			//Serial.print("temperature: ");
			//Serial.print(T,2);
			//Serial.print(" deg C, ");
			stat = BMP180_PressureSensor.startPressure(3);
			if (stat != 0)
			{
				delay(stat);
				stat = BMP180_PressureSensor.getPressure(P, T);
				/*if (stat != 0)
				{
				p0=1000;
				a = BMP180_PressureSensor.altitude(P,p0);
				}*/
				*p = (float)P;
				*temp = (float)T;
				
				return true;
			}
		}
		else return false;
	}
	else return false;
	return false;
}