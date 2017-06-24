/*
 Name:		ArduinoNRFCenter.ino
 Created:	2017/3/17 15:52:15
 Author:	Asuka
*/

#include <Adafruit_NeoPixel.h>
const uint8_t LED_PIN = 9;
Adafruit_NeoPixel LED_Strip = Adafruit_NeoPixel(8, LED_PIN, NEO_GRB + NEO_KHZ800);

const uint8_t PIN_BatteryVoltage = A0;

#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfSpiDriver.h>
#include <MirfHardwareSpiDriver.h>

#define NRF_DEVICE_ADDR "iotc1"
const uint8_t PIN_NRF_CE = 4;
const uint8_t PIN_NRF_CSN = 5;
const uint8_t PayLoad_Length = 32;
uint8_t data[32];

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

typedef struct _Pt_SetClock
{
	uint8_t Year;
	uint8_t Month;
	uint8_t Day;
	uint8_t Weekday;
	uint8_t Hour;
	uint8_t Minute;
	uint8_t Second;
	uint8_t TimeMode;
}*Pt_SetClock;

// the setup function runs once when you press reset or power the board
void setup() 
{
	Serial.begin(115200);
	I2C_Buffer.reserve(128);

	LED_Strip.begin();
	LED_Strip.show();
	delay(100);

	Serial1.begin(115200);
	delay(100);

	Wire.begin(0x08);
	Wire.onReceive(I2C_OnReceive);
	Wire.onRequest(I2C_OnRequest);
	delay(100);

	pinMode(I2C_INT_PIN, OUTPUT);
	digitalWrite(I2C_INT_PIN, HIGH);
	delay(1000);

	Mirf.spi = &MirfHardwareSpi;
	Mirf.cePin = 4;
	Mirf.csnPin = 5;
	Mirf.init();
	Mirf.setRADDR((byte *)NRF_DEVICE_ADDR);
	Mirf.payload = 32;
	Mirf.configRegister(EN_AA, 0);
	Mirf.config();
	delay(100);

	InitLcd();

	Serial.print("Init");
	delay(500);
	LogLcd("init");
}

// the loop function runs over and over again until power down or reset
void loop()
{
	if (Serial.available())
	{
		char c = Serial.read();
		if (c == 'I')
		{
			I2C_Buffer = Serial.readStringUntil('\n');
			Serial.println(I2C_Buffer);
			Send_I2C();
		}
		else if (c == 'D')
		{
			uint8_t datat[32];
			Serial.println("send nrf lamp info");
			datat[0] = 2;
			datat[1] = 10;
			Mirf.setTADDR((uint8_t*)"iotc1");
			Mirf.send(datat);
			while (Mirf.isSending());
			Serial.readStringUntil('\n');
		}
		else if (c == 'T')
		{
			Serial.println("send nrf lamp info");
			((NrfPacket)data)->Id = (uint8_t)0;
			((NrfPacket)data)->Type = 'C';
			((NrfPacket)data)->Options = (uint8_t)0;
			Pt_SetClock pc = (Pt_SetClock)(((NrfPacket)data)->Payload);
			pc->Year = (uint8_t)17;
			pc->Month = (uint8_t)6;
			pc->Day = (uint8_t)12;
			pc->Hour = (uint8_t)19;
			pc->Minute = (uint8_t)37;
			pc->Second = (uint8_t)0;
			pc->Weekday = (uint8_t)1;
			pc->TimeMode = '2';
			for (int i = 0; i < 16; i++)
			{
				Serial.print(data[i]);
				Serial.print(' ');
			}
			Serial.println();
			Mirf.setTADDR((uint8_t*)"Lamp0");
			Mirf.send(data);
			while (Mirf.isSending())
			{

			}
			Serial.println("finished sending");
			Serial.readStringUntil('\n');
		}
	}

	if (Mirf.dataReady())
	{
		Mirf.getData(data);

		WeatherStationData wsd = (WeatherStationData)data;

		double batteryVoltage = (double)wsd->BatteryVolt_Raw * 5 / 1023;
		double rainVoltage = (double)(1023 - wsd->RainVolt_Raw) * 5 / 1023;

		Serial1.print("DS16(0,0,'Arduino Weather Station',15);");

		int selfBattRaw = analogRead(A0);
		double selfBattVolt= (double)selfBattRaw * 5 / 1023;
		if (selfBattVolt < 2.0f)
		{
			Serial1.println("DS16(0,16,'DC Line ',2);");
		}
		else
		{
			Serial1.print("DS16(0,16,'");
			Serial1.print(selfBattVolt);
			Serial1.print("V   ',15);");
		}

		Serial1.print("DS16(0,32,'Packet: ");
		Serial1.print(wsd->Packet_Id);
		Serial1.print("   ',15);");

		Serial1.print("DS16(0,48,'Remote Battery: ");
		Serial1.print(batteryVoltage);
		Serial1.print("V ',15);");

		Serial1.print("DS16(0,64,'");
		Serial1.print(wsd->Temperature);
		Serial1.print("C  ");
		Serial1.print(wsd->Humidity);
		Serial1.print("%RH  ',15);"); 
		
		Serial1.print("DS16(0,80,'");
		Serial1.print(wsd->AirPressure);
		Serial1.print("hPa  ");
		Serial1.print(wsd->Brightness);
		Serial1.print("Lx  ',15);");

		Serial1.print("DS16(0,96,'Dust: ");
		Serial1.print(wsd->DustVolt);
		Serial1.print("V  Rain: ");
		Serial1.print(rainVoltage);
		Serial1.print("V  ',15);");
		Serial1.println();


		Serial.print(selfBattVolt);
		Serial.print("V Device:");
		Serial.print(wsd->Device_Id);
		Serial.print(" PID:");
		Serial.print(wsd->Packet_Id);
		Serial.print(" ");
		Serial.print(batteryVoltage);
		Serial.print("V ");
		Serial.print(wsd->Temperature);
		Serial.print("C ");
		Serial.print(rainVoltage);
		Serial.println("V ");

		Serial.print(wsd->AirPressure);
		Serial.print("hPa ");
		Serial.print(wsd->Humidity);
		Serial.print("%RH ");
		Serial.print(wsd->DustVolt);
		Serial.print("V ");
		Serial.print(wsd->Brightness);
		Serial.println("Lux");
		Serial.println();

		Mirf.setRADDR((byte *)NRF_DEVICE_ADDR);
		Mirf.config();
	}
}

void InitLcd()
{

	Serial1.println("CLS(0);");
}

int lcd_line = 0;
void LogLcd(String s)
{
	if (120 + lcd_line * 12 > 240)
	{
		Serial1.println("CBOF(0,120,319,239,0,0);");
		delay(10);
		lcd_line = 0;
	}
	Serial1.print("DS12(0,");
	Serial1.print(120+ lcd_line * 12);
	Serial1.print(",'");
	Serial1.print(s);
	Serial1.println("',15);");
	delay(10);

	Serial.print("DS12(0,");
	Serial.print(120 + lcd_line * 12);
	Serial.print(",'");
	Serial.print(s);
	Serial.println("',15);");
	lcd_line++;
}

void Send_I2C()
{
	digitalWrite(I2C_INT_PIN, LOW);
	delay(2);
	digitalWrite(I2C_INT_PIN, HIGH);
}

void I2C_OnReceive(int n)
{
	//if (Wire.available())
	{
		char c = Wire.read();
		switch (c)
		{
		case 'R':
			I2C_STAT = 1;
			break;

		case 'S':
			I2C_STAT = 2;
			break;

		case 'P':
			/*if (Wire.available())
			{
				//I2C_Buffer = Wire.readStringUntil('\n');
				I2C_Buffer = Wire.readString();
			}*/
			I2C_Buffer = "";
			while (Wire.available())
			{
				I2C_Buffer += (char)Wire.read();
				
			}
			I2C_Buffer.trim();
			LogLcd(I2C_Buffer);
			break;

		case 'W':
			I2C_Buffer = "";
			while (Wire.available())
			{
				I2C_Buffer += (char)Wire.read();
			}
			I2C_Buffer.trim();
			Serial.println(I2C_Buffer);
			break;

		default:
			break;
		}
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
	else if (I2C_STAT == 2)
	{
		I2C_STAT = 0;
		for (int i = 0; i<32; i++)
		{
			Wire.write(data[i]);
		}
	}
}
