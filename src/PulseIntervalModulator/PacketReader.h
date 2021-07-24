// PacketReader.h
// Catches the pulse stream and builds up a packet buffer,
// as long as the incoming bits are valid, otherwise it resets.
// All work is done during interrupts.

#ifndef _PIM_PACKET_READER_h
#define _PIM_PACKET_READER_h

#include "Constants.h"
#include <Fast.h>

#ifndef PIM_USE_STATIC_CALLBACK
class PacketReaderCallback
{
public:
	// After the callback, the buffer is considered free for the next incoming packet.
	virtual void OnPacketReceived(const uint32_t packetStartTimestamp) {}

	virtual void OnPacketLost(const uint32_t packetStartTimestamp) {}
};
#endif 

template<const uint8_t MaxDataBytes, const uint8_t ReadPin>
class PacketReader
{
private:
	uint8_t* IncomingBuffer = nullptr;
	uint8_t IncomingIndex = 0;
	volatile uint8_t IncomingSize = 0;

	uint8_t BitBuffer = 0;
	uint8_t BitIndex = 0;
	volatile uint32_t PacketStartTimestamp = 0;
	uint32_t BitTimestamp = 0;

	volatile uint32_t LastTimeStamp = 0;

	enum StateEnum
	{
		Blanking,
		BlankingWithPendingPacket,
		WaitingForPreAmbleStart,
		WaitingForPreAmbleEnd,
		WaitingForHeaderEnd,
		WaitingForDataBits,
		WaitingForPacketClear,
	};

	volatile StateEnum State = StateEnum::WaitingForPreAmbleStart;

#if defined(PIM_USE_STATIC_CALLBACK)
	void (*ReceiveCallback)(const uint32_t packetStartTimestamp) = nullptr;
	void (*LostCallback)(const uint32_t packetStartTimestamp) = nullptr;
#else
	PacketReaderCallback* Callback = nullptr;
#endif

public:
	PacketReader(uint8_t* incomingBuffer)
		: IncomingBuffer(incomingBuffer)
	{
		pinMode(ReadPin, INPUT);
	}

#if defined(PIM_USE_STATIC_CALLBACK)
	void Start(
		void (*receiveCallback)(const uint32_t packetStartTimestamp),
		void (*lostCallback)(const uint32_t packetStartTimestamp))
	{
		ReceiveCallback = receiveCallback;
		LostCallback = lostCallback;
		Start();
	}
#else
	void Start(PacketReaderCallback* callback)
	{
		Callback = callback;
		Start();
	}
#endif

	// Must perform interrupt attach before starting.
	// attachInterrupt(digitalPinToInterrupt(ReadPin), OnPulse, RISING);
	void Start()
	{
		State = StateEnum::WaitingForPreAmbleStart;

		// Make sure we don't return a previous size after it's been cleared.
		IncomingSize = 0;
	}

	// Called after blanking or after consuming packet after OnPacketReceived.
	void Restore()
	{
		switch (State)
		{
		case StateEnum::Blanking:
			Start();
			break;
		case StateEnum::BlankingWithPendingPacket:
			if (IncomingSize > 0)
			{
				State = StateEnum::WaitingForPacketClear;
			}
			else
			{
				State = StateEnum::WaitingForPreAmbleStart;
			}
		case StateEnum::WaitingForPacketClear:
			Start();
			break;
		default:
			break;
		}
	}

	void Stop()
	{
		detachInterrupt(digitalPinToInterrupt(ReadPin));
		BlankReceive();
	}

	// Blank receiving during 
	void BlankReceive()
	{
		if (State == StateEnum::WaitingForPacketClear)
		{
			State = StateEnum::BlankingWithPendingPacket;
		}
		else
		{
			State = StateEnum::Blanking;
		}
	}

	const bool HasIncoming(uint8_t& incomingSize)
	{
		switch (State)
		{
		case StateEnum::BlankingWithPendingPacket:
		case StateEnum::WaitingForPacketClear:
			incomingSize = IncomingSize;
			return incomingSize > 0;
			break;
		default:
			return false;
		}
	}

	// Returns false if in the middle of receiving a packet.
	// Returns true the minimum silenceInterval has been observed.
	const bool CanSend(const uint32_t silenceInterval)
	{
		switch (State)
		{
		case StateEnum::Blanking:
		case StateEnum::BlankingWithPendingPacket:
			return false;
		default:
			return micros() - LastTimeStamp > silenceInterval;
		}
	}

	void OnPulse()
	{
		LastTimeStamp = micros();
		bool bit = false;
		switch (State)
		{
		case StateEnum::Blanking:
		case StateEnum::BlankingWithPendingPacket:
		case StateEnum::WaitingForPacketClear:
			// Ignore.
			break;
		case StateEnum::WaitingForPreAmbleStart:
			PacketStartTimestamp = LastTimeStamp;
			State = StateEnum::WaitingForPreAmbleEnd;
			break;
		case StateEnum::WaitingForPreAmbleEnd:
			if (Constants::ValidatePreamble(LastTimeStamp - PacketStartTimestamp))
			{
				// Preamble header detected.
				BitTimestamp = LastTimeStamp;

				// Take this time to reset the incoming buffer.
				IncomingIndex = 0;
				IncomingSize = 0;
				BitIndex = 0;

				State = StateEnum::WaitingForHeaderEnd;
			}
			else
			{
				// Restart assuming the last pulse was a start pulse.
				PacketStartTimestamp = LastTimeStamp;
				State = StateEnum::WaitingForPreAmbleEnd;
			}
			break;
		case StateEnum::WaitingForHeaderEnd:
			if (Constants::DecodeBit(LastTimeStamp - BitTimestamp, bit))
			{
				BitTimestamp = LastTimeStamp;

				// Header bits come in MSB first.
				IncomingSize += (bit << (Constants::HeaderBits - 1 - BitIndex));
				BitIndex++;

				if (BitIndex > (Constants::HeaderBits - 1))
				{
					// Add one, according to specification.
					IncomingSize += Constants::MinDataBytes;

					if (IncomingSize > MaxDataBytes) {
						// Invalid packet size.
						// Restart assuming the last pulse was a start pulse.
						PacketStartTimestamp = LastTimeStamp;
						State = StateEnum::WaitingForPreAmbleEnd;
					}
					else
					{
						// Packet size has been read, wait for data bits.
						BitBuffer = 0;
						BitIndex = 0;
						State = StateEnum::WaitingForDataBits;
					}
				}
			}
			else
			{
				// Restart assuming the last pulse was a start pulse.
				PacketStartTimestamp = LastTimeStamp;
				State = StateEnum::WaitingForPreAmbleEnd;
			}
			break;
		case StateEnum::WaitingForDataBits:
			if (Constants::DecodeBit(LastTimeStamp - BitTimestamp, bit))
			{
				BitTimestamp = LastTimeStamp;

				// Data bits come in MSB.
				BitBuffer += (bit << (7 - BitIndex));
				BitIndex++;

				if (BitIndex > 7)
				{
					IncomingBuffer[IncomingIndex++] = BitBuffer;

					if (IncomingIndex > (IncomingSize - 1))
					{
						State = StateEnum::WaitingForPacketClear;
#if defined(PIM_USE_STATIC_CALLBACK)
						if (ReceiveCallback != nullptr)
						{
							ReceiveCallback(PacketStartTimestamp);
						}
#else
						if (Callback != nullptr)
						{
							Callback->OnPacketReceived(PacketStartTimestamp);
						}
#endif
					}
					else
					{
						//Serial.println(BitBuffer);
						BitBuffer = 0;
						BitIndex = 0;
					}
				}
			}
			else
			{
				// Using BitTimestamp as copy, just for the event.
				BitTimestamp = PacketStartTimestamp;

				// Restart assuming the last pulse was a start pulse.
				PacketStartTimestamp = LastTimeStamp;
				State = StateEnum::WaitingForPreAmbleEnd;

				// Let the Driver know we dropped a packet.
#if defined(PIM_USE_STATIC_CALLBACK)
				if (LostCallback != nullptr)
				{
					LostCallback(BitTimestamp);
				}
#else
				if (Callback != nullptr)
				{
					Callback->OnPacketLost(BitTimestamp);
				}
#endif
			}
			break;
		default:
			break;
		}
	}
};
#endif