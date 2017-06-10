/*
 Name:		ArduinoNRFNode.ino
 Created:	2017/3/17 15:52:31
 Author:	Asuka
*/

#include <LowPower.h>

#include <nRF24L01.h>
#include <MirfSpiDriver.h>
#include <MirfHardwareSpiDriver.h>
#include <Mirf.h>

#define CENTER_ADDR "cent0"

#define PACKET_SIZE 32

struct NrfPacket
{
	uint8_t id;
};

// the setup function runs once when you press reset or power the board
void setup() {

	Mirf.spi = &MirfHardwareSpi;
	Mirf.cePin = A3;
	Mirf.csnPin = 10;

	Mirf.init();
	Mirf.setRADDR((byte *)"iotc1");
	Mirf.payload = PACKET_SIZE;
	Mirf.channel = 0;
	Mirf.config();
	
	Mirf.configRegister(EN_AA, 0);
}

// the loop function runs over and over again until power down or reset
void loop() {
	uint8_t data[32];

	Mirf.setTADDR((byte *)CENTER_ADDR);
	Mirf.send(data);
	while (Mirf.isSending());
	Mirf.powerDown();

	LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}
