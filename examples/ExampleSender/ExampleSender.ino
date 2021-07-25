//
// Example of Sender only with Writer.
//


#define DEBUG_LOG
#define PIM_USE_STATIC_CALLBACK

#include <PulseIntervalModulator.h>

const uint8_t WritePin = 7;
const uint8_t BufferSize = 5;

PacketWriter<BufferSize, WritePin> Writer;

volatile bool PacketSentFlag = false;
uint8_t OutgoingPacket[BufferSize];
const uint32_t SendPeriodMillis = 500;
uint32_t LastSent = 0;


void setup()
{
#ifdef DEBUG_LOG
	Serial.begin(115200);
#endif

	Writer.Start(OnPacketSentInterrupt);

#ifdef DEBUG_LOG
	Serial.println(F("ExampleSender Start."));
#endif
}

void loop()
{
	if (PacketSentFlag)
	{
		PacketSentFlag = false;

#ifdef DEBUG_LOG
		Serial.print(F("OnPacketSent @"));
		Serial.print(micros());
		Serial.println(F(" us"));
#endif
	}
	else if (millis() - LastSent > SendPeriodMillis)
	{
		LastSent = millis();

#ifdef DEBUG_LOG
		Serial.print(F("Sending Packet @"));
		Serial.print(micros());
		Serial.println(F(" us"));
#endif

		// Fill in test data.
		uint8_t Message[5] = { 'H', 'e','l', 'l', 'o' };

		Writer.SendPacket(Message, 5);
	}
}

void OnPacketSentInterrupt()
{
	PacketSentFlag = true;
}

ISR(TIMER0_COMPA_vect)
{
	Writer.OnWriterInterrupt();
}