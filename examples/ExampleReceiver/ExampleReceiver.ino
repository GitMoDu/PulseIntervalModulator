//
// Example of Receiver only with Reader.
//


#define DEBUG_LOG
#define PIM_USE_STATIC_CALLBACK

#include <PulseIntervalModulator.h>

const uint8_t ReadPin = 2;
const uint8_t BufferSize = 64;
uint8_t IncomingBuffer[BufferSize];

PacketReader<BufferSize, ReadPin> Reader(IncomingBuffer);

volatile bool PacketLostFlag = false;
volatile bool PacketReceivedFlag = false;
volatile uint32_t IncomingStartTimestamp = 0;


void setup()
{
#ifdef DEBUG_LOG
	Serial.begin(115200);
#endif

	// Attach interrupt before starting.
	attachInterrupt(digitalPinToInterrupt(ReadPin), OnReaderPulse, RISING);
	Reader.Start(OnPacketReceived, OnPacketLost);

#ifdef DEBUG_LOG
	Serial.println(F("ExampleReceiver Start."));
#endif
}

void loop()
{
	if (PacketLostFlag)
	{
		PacketLostFlag = false;
#ifdef DEBUG_LOG
		Serial.print(F("OnPacketLost @"));
		Serial.print(IncomingStartTimestamp);
		Serial.println(F(" us"));
#endif
	}
	else if (PacketReceivedFlag)
	{
		PacketReceivedFlag = false;

		uint8_t incomingSize = 0;

		if (Reader.HasIncoming(incomingSize))
		{
			// When packet is available.
#ifdef DEBUG_LOG
			Serial.print(F("OnPacketReceived @"));
			Serial.print(IncomingStartTimestamp);
			Serial.print(F(" us ("));
			Serial.print(incomingSize);
			Serial.println(F(") bytes"));

			Serial.print(F("Data: "));
			for (uint8_t i = 0; i < incomingSize; i++)
			{
				Serial.print("|");
				Serial.print(IncomingBuffer[i], HEX);
			}
			Serial.println('|');
			Serial.println();
#endif
		}

		// Restore Reader after consuming the packet.
		Reader.Restore();
	}
}

void OnPacketReceived(const uint32_t packetStartTimestamp)
{
	PacketReceivedFlag = true;
	IncomingStartTimestamp = packetStartTimestamp;
}

void OnPacketLost(const uint32_t packetStartTimestamp)
{
	PacketLostFlag = true;
	IncomingStartTimestamp = packetStartTimestamp;
}

void OnReaderPulse()
{
	Reader.OnPulse();
}

