//
// Example of classed Driver, using the interface callbacks.
// Reads and Writes in Half-Duplex mode, avoiding collisions.
//

#define DEBUG_LOG

#include "ClassDriver.h"

const uint8_t MaxPacketSize = 8;
const uint8_t ReadPin = 2;
const uint8_t WritePin = 7;

ClassDriver<MaxPacketSize, ReadPin, WritePin> Driver;

const uint32_t SendPeriodMillis = 500;
uint32_t LastSent = 0;


void setup()
{
#ifdef DEBUG_LOG
	Serial.begin(115200);
#endif

	attachInterrupt(digitalPinToInterrupt(ReadPin), OnReaderPulse, RISING);
	Driver.Start();

#ifdef DEBUG_LOG
	Serial.println(F("ExampleDriver Start."));
#endif
}

void loop()
{
	if (millis() - LastSent > SendPeriodMillis)
	{
		if (Driver.CanSend())
		{
			LastSent = millis();
#ifdef DEBUG_LOG
			Serial.print(F("Sending Packet @"));
			Serial.print(micros());
			Serial.println(F(" us"));
#endif

			// Fill in test data.
			uint8_t MessageSize = 0;
			Driver.OutgoingPacket[MessageSize++] = 'H';
			Driver.OutgoingPacket[MessageSize++] = 'e';
			Driver.OutgoingPacket[MessageSize++] = 'l';
			Driver.OutgoingPacket[MessageSize++] = 'l';
			Driver.OutgoingPacket[MessageSize++] = 'o';

			Driver.SendPacket(MessageSize);
		}
		else
		{
#ifdef DEBUG_LOG
			Serial.print(F("Collision avoided @"));
			Serial.print(micros());
			Serial.println(F(" us"));
#endif
		}
	}
	else
	{
		Driver.Check();
	}
}

void OnReaderPulse()
{
	Driver.OnReaderInterrupt();
}

ISR(TIMER0_COMPA_vect)
{
	Driver.OnWriterInterrupt();
}
