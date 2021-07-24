//
// Example of Receiver only with Reader.
//

#include <PulseIntervalEncoding.h>

#define DEBUG_LOG

const uint8_t ReadPin = 2;
const uint8_t BufferSize = 64;

PacketReader<BufferSize> Reader(ReadPin);

volatile bool PacketLostFlag = false;
volatile bool PacketReceivedFlag = false;

uint8_t Buffer[BufferSize];


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
	Serial.println(F("Tester Receiver Start."));
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
		if (Reader.GetIncoming(Buffer, incomingSize))
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
				Serial.print(Buffer[i], HEX);
			}
			Serial.println('|');
			Serial.println();
#endif
		}

		// Restore receiver after consuming the packet.
		Reader.Start();
	}
	else
	{
		//Reader.CheckForStuckPacket();
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

