//
// Example of Sender only with Writer.
//



#define DEBUG_LOG
#include <PulseIntervalEncoding.h>

const uint8_t WritePin = 7;
const uint8_t BufferSize = 1;

uint8_t BufferOut[BufferSize];

volatile bool PacketSentFlag = false;

PacketWriter<BufferSize> Writer(WritePin);

const uint32_t SendPeriodMillis = 500;
uint32_t LastSent = 0;


void setup()
{
	pinMode(3, OUTPUT);
#ifdef DEBUG_LOG
	Serial.begin(115200);
#endif

	// Fill in test data.
	for (uint8_t i = 0; i < BufferSize; i++)
	{
		BufferOut[i] = i + 1;
	}

	Writer.Start(OnPacketSentInterrupt);

#ifdef DEBUG_LOG
	Serial.println(F("Tester Receiver Start."));
#endif
}

void loop()
{
	if (PacketSentFlag)
	{
		digitalWrite(3, LOW);
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

		digitalWrite(3, HIGH);
		Writer.SendPacket(BufferOut, BufferSize);
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