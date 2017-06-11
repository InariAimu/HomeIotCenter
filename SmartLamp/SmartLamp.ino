/*
 Name:		SmartLamp.ino
 Created:	2017/3/19 14:52:53
 Author:	Asuka
*/

#include <Wire.h>
#include <RealTimeClockDS1307.h>

#include <nRF24L01.h>
#include <MirfSpiDriver.h>
#include <MirfHardwareSpiDriver.h>
#include <Mirf.h>

#include <math.h>

const uint8_t NRF_CE_PIN = 4;
const uint8_t NRF_CSN_PIN = 5;

#define NRF_DEVICE_ADDR "Lamp0"

const uint8_t ManSensorPin = 3;

const uint8_t TrigPin = 7;
const uint8_t EchoPin = 6;

const uint8_t Yellow_Addr = 0x62;
const uint8_t White_Addr = 0x63;

const uint8_t AT24C32_Addr = 0x50;
const uint8_t DS1307_Addr = 0x68;

//Generic NrfPacket Properties
//ID:		packet id 0-255
//Type:		packet type
//			'A'
//Option:	packet options
//Payload:	payload
typedef struct _NrfPacket
{
	uint8_t Id;
	uint8_t Type;
	uint8_t Options;
	uint8_t Payload[29];
}*NrfPacket;

//Brightness of each channel
//From 0 to 1023
typedef struct _Pt_SetLamp
{
	int16_t Yellow;
	int16_t White;
}*Pt_SetLamp;

//Set clock
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

#define Mode_Auto 0
#define Mode_Manual 1

uint8_t Mode_Lamp = Mode_Auto;
uint8_t Mode_OnOff = Mode_Manual;

uint8_t Stat_Man = 0;
uint8_t Stat_Man_Prev = 0;
int16_t ManDistance = 95;


#define CHANNEL_YELLOW 0
#define CHANNEL_WHITE 1

uint16_t Yellow_Val = 4095;
uint16_t White_Val = 4095;

byte data[32];

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
	pinMode(ManSensorPin, INPUT_PULLUP);

	pinMode(TrigPin, OUTPUT);
	pinMode(EchoPin, INPUT);

	Wire.begin();

	delay(100);

	Yellow_Val = 2048;
	White_Val = 2048 + 1024;

	SetMCP4725(Yellow_Addr, Yellow_Val);
	SetMCP4725(White_Addr, White_Val);
	Serial.println("i2c init");
	delay(1000);

	Mirf.spi = &MirfHardwareSpi;
	Mirf.cePin = NRF_CE_PIN;
	Mirf.csnPin = NRF_CSN_PIN;
	Mirf.init();
	Mirf.setRADDR((byte *)NRF_DEVICE_ADDR);
	Mirf.payload = 32;
	Mirf.configRegister(EN_AA, 0);
	Mirf.config();

	Serial.println("nrf init");
}

// the loop function runs over and over again until power down or reset
void loop() {
	if (Mode_OnOff == Mode_Auto)
	{
		if (digitalRead(ManSensorPin) == HIGH && Stat_Man_Prev == 0)
		{
			Serial.println("Man detect");
			Stat_Man = 1;
			Serial.print(Yellow_Val);
			Serial.print(",");
			Serial.println(White_Val);
			Stat_Man_Prev = 1;
		}
		else if (digitalRead(ManSensorPin) == 0)
		{
			Stat_Man_Prev = 0;
		}

		if (Stat_Man == 1)
		{
			int dist = GetDistance();

			if (dist <= 0 || dist > ManDistance)
			{
				Stat_Man = 0;
				Serial.println("Man out of range");

				//Mode_Lamp = Mode_Manual;
				SetMCP4725(Yellow_Addr, 0);
				SetMCP4725(White_Addr, 0);
			}
			else
			{
				//Mode_Lamp = Mode_Auto;
				//SetMCP4725(Yellow_Addr, Yellow_Val);
				//SetMCP4725(White_Addr, White_Val);
			}
		}
	}
	else
	{
		Stat_Man = 1;
	}

	if (Stat_Man == 1 && Mode_Lamp == Mode_Auto)
	{
		RTC.readClock();
		uint16_t yb = 0;
		uint16_t wb = 0;
		GetTimeBrightness(RTC.getHours(), RTC.getMinutes(), &yb, &wb);
		/*Serial.print(RTC.getHours());
		Serial.print(':'); 
		Serial.print(RTC.getMinutes());
		Serial.print(':');
		Serial.println(RTC.getSeconds());*/
		Yellow_Val = BrightnessToVoltage(yb, CHANNEL_YELLOW);
		White_Val = BrightnessToVoltage(wb, CHANNEL_WHITE);
		SetMCP4725(Yellow_Addr, Yellow_Val);
		SetMCP4725(White_Addr, White_Val);
	}

	long t = millis();
	while (millis() - t < 1000)
	{
		if (Mirf.dataReady())
		{
			Serial.println("get nrf packet");
			Mirf.getData(data);
			NrfPacket np = (NrfPacket)data;
			switch (np->Type)
			{
			case 'S':
			{
				Pt_SetLamp pl = (Pt_SetLamp)(np->Payload);
				Yellow_Val = BrightnessToVoltage(pl->Yellow, CHANNEL_YELLOW);
				White_Val = BrightnessToVoltage(pl->White, CHANNEL_WHITE);
				SetMCP4725(Yellow_Addr, Yellow_Val);
				SetMCP4725(White_Addr, White_Val);
				Mode_Lamp = Mode_Manual;
				break;
			}
			case 'C':
			{
				Pt_SetClock pc = (Pt_SetClock)(np->Payload);
				RTC.stop();
				RTC.setYear(pc->Year);
				RTC.setMonth(pc->Month);
				RTC.setDate(pc->Day);
				RTC.setDayOfWeek(pc->Weekday);
				RTC.setHours(pc->Hour);
				RTC.setMinutes(pc->Minute);
				RTC.setSeconds(pc->Second);
				if (pc->TimeMode == 'A')
				{
					RTC.setAM();
				}
				else
				{
					RTC.set24h();
				}
				RTC.setClock();
				if (RTC.isStopped())RTC.start();
				break;
			}
			case 'Q':
			{
				break;
			}
			}

			break;
		}
	}


}

uint16_t BrightnessToVoltage(int16_t brightness,int16_t channel)
{
	if (brightness == 0)
	{
		return 0;
	}
	if (channel == CHANNEL_YELLOW)
	{
		if (brightness >= 0 && brightness < 512)
		{
			return 1024 + (brightness - 0) * 2;
		}
		else if (brightness >= 512)
		{
			return 2200 + ((int32_t)brightness - 512) * (4095 - 2200) / 512;
		}
		else
		{
			return brightness * 4 - 1;
		}
	}
	else if (channel == CHANNEL_WHITE)
	{
		if (brightness >= 0 && brightness < 512)
		{
			return 1024 + (brightness - 0) * 2;
		}
		else if (brightness >= 512)
		{
			return 2200 + ((int32_t)brightness - 512) * (4095 - 2200) / 512;
		}
		else
		{
			return brightness * 4 - 1;
		}
	}
}

void GetTimeBrightness(int16_t hour, int16_t minute, uint16_t* yellow, uint16_t* white)
{
	if (hour >= 0 && hour <= 6)
	{
		*yellow = 512;
		*white = 0;
	}
	else if (hour > 6 && hour <= 8)
	{
		int t = (hour - 6) * 60 + minute;
		*yellow = 512 + (1024 - 512)*t / 180;
		*white = 1024 * t / 180;
	}
	else if (hour > 8 && hour < 20)
	{
		*yellow = 1024;
		*white = 1024;
	}
	else if (hour >= 20 && hour <= 23)
	{
		int32_t t = (hour - 20) * 60 + minute;
		int32_t ty = 512 * t / 240;
		int32_t tw = 1024 * t / 240;
		*yellow = 1024 - ty;
		*white = 1024 - tw;
	}
}

int GetDistance()
{
	digitalWrite(TrigPin, LOW);
	delayMicroseconds(2);
	digitalWrite(TrigPin, HIGH);
	delayMicroseconds(10);
	digitalWrite(TrigPin, LOW);

	return pulseIn(EchoPin, HIGH) / 58.0;
}

void SetMCP4725(uint8_t Addr, uint16_t Value)
{
	Wire.beginTransmission(Addr);
	Wire.write((uint8_t)64);                     // cmd to update the DAC
	Wire.write((uint8_t)(Value >> 4));        // the 8 most significant bits...
	Wire.write((uint8_t)((Value & 15) << 4)); // the 4 least significant bits...
	Wire.endTransmission();
}
