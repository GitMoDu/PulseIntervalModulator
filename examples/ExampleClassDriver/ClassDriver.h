// ClassDriver.h

#ifndef _CLASS_DRIVER_h
#define _CLASS_DRIVER_h

#include <PulseIntervalModulator.h>

template<const uint8_t MaxPacketSize, const uint8_t ReadPin, const uint8_t WritePin>
class ClassDriver : virtual public PacketReaderCallback, virtual public PacketWriterCallback
{
private:
	volatile bool PacketLostFlag = false;
	volatile bool PacketReceivedFlag = false;
	volatile bool PacketSentFlag = false;

	uint8_t IncomingPacketSize = 0;
	uint32_t IncomingStartTimestamp = 0;
	uint8_t IncomingPacket[Constants::MaxDataBytes];

	PacketReader<MaxPacketSize, ReadPin> Reader;
	PacketWriter<MaxPacketSize, WritePin> Writer;

public:
	uint8_t OutgoingPacket[MaxPacketSize];

public:
	ClassDriver()
		: PacketReaderCallback()
		, PacketWriterCallback()
		, Reader(IncomingPacket)
		, Writer()
	{
	}

	void OnWriterInterrupt()
	{
		Writer.OnWriterInterrupt();
	}

	void OnReaderInterrupt()
	{
		Reader.OnPulse();
	}

	// Must perform interrupt attach before starting.
	// attachInterrupt(digitalPinToInterrupt(ReadPin), OnPulse, RISING);
	void Start()
	{
		Reader.Start(this);
		Writer.Start(this);
	}

	const bool CanSend()
	{
		return Reader.CanSend();
	}

	// Must check with CanSend() right before this call.
	void SendPacket(const uint8_t packetSize)
	{
		// Blank reader to ignore cross-talk.
		Reader.BlankReceive();
		Writer.SendPacket(OutgoingPacket, packetSize);
	}

	void Check()
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
					Serial.print(IncomingPacket[i], HEX);
				}
				Serial.println('|');
				Serial.println();
#endif
			}

			// Restore Reader after consuming the packet.
			Reader.Restore();
		}
		else if (PacketSentFlag)
		{
			PacketSentFlag = false;
#ifdef DEBUG_LOG
			Serial.print(F("OnPacketSent @"));
			Serial.print(micros());
			Serial.println(F(" us"));
#endif
		}
	}

	virtual void OnPacketReceived(const uint32_t startTimestamp)
	{
		PacketReceivedFlag = true;
		IncomingStartTimestamp = startTimestamp;
	}

	virtual void OnPacketLost(const uint32_t startTimestamp)
	{
		PacketLostFlag = true;
		IncomingStartTimestamp = startTimestamp;
	}

	virtual void OnPacketSent()
	{
		// Restore Reader after blanking during sending.
		Reader.Restore();
		PacketSentFlag = true;
	}
};
#endif