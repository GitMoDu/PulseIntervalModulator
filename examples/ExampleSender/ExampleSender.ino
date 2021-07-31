//
// Example of Sender only with Writer.
//

#define DEBUG_LOG

//#define PIM_USE_STATIC_CALLBACK must be enabled in Constants.
#include <PulseIntervalModulator.h>

const uint8_t WritePin = 7;
const uint8_t BufferSize = 32;
uint8_t OutgoingPacket[BufferSize];

PacketWriter Writer(BufferSize, WritePin);

volatile bool PacketSentFlag = false;
volatile uint32_t SentTimestamp = 0;

uint32_t LastSent = 0;

const uint32_t SendPeriodMillis = 500;

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
		Serial.print(F(" us | Took "));
		Serial.print(SentTimestamp);
		Serial.println(F(" us to transmit."));
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

		SentTimestamp = micros();
		Writer.SendPacket(Message, 5);
	}
}

void OnPacketSentInterrupt()
{
	PacketSentFlag = true;
	SentTimestamp = micros() - SentTimestamp;
}