// PacketReader.h
// Catches the pulse stream and builds up a packet buffer,
// as long as the incoming bits are valid, otherwise it resets.
// All work is done during interrupts.

#ifndef _PIM_PACKET_READER_h
#define _PIM_PACKET_READER_h

#include "Constants.h"
#include <Arduino.h>


#if !defined(PIM_USE_STATIC_CALLBACK)
class PacketReaderCallback
{
public:
	// After the callback, the buffer is considered free for the next incoming packet.
	virtual void OnPacketReceived(const uint32_t packetStartTimestamp) {}

	virtual void OnPacketLost(const uint32_t packetStartTimestamp) {}
};
#endif 

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

	volatile StateEnum State = StateEnum::Blanking;

	const uint8_t MaxDataBytes = 0;
	const uint8_t ReadPin = 0;

#if defined(PIM_USE_STATIC_CALLBACK)
	void (*ReceiveCallback)(const uint32_t packetStartTimestamp) = nullptr;
	void (*LostCallback)(const uint32_t packetStartTimestamp) = nullptr;
#else
	PacketReaderCallback* Callback = nullptr;
#endif

public:
	PacketReader(uint8_t* incomingBuffer, const uint8_t maxDataBytes, const uint8_t readPin)
		: IncomingBuffer(incomingBuffer)
		, MaxDataBytes(maxDataBytes)
		, ReadPin(readPin)
	{
	}

	void Start(
#if defined(PIM_USE_STATIC_CALLBACK)
		void (*receiveCallback)(const uint32_t packetStartTimestamp),
		void (*lostCallback)(const uint32_t packetStartTimestamp))
	{
		ReceiveCallback = receiveCallback;
		LostCallback = lostCallback;
#else
		PacketReaderCallback* callback)
	{
		Callback = callback;
#endif
		SetupInterrupt();
		Start();
	}

	void Start()
	{
		Attach();
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
		BlankReceive();
	}

	// Blank receiving during 
	void BlankReceive()
	{
		Detach();

		if (State == StateEnum::WaitingForPacketClear)
		{
			State = StateEnum::BlankingWithPendingPacket;
		}
		else
		{
			State = StateEnum::Blanking;
		}
	}

	const bool HasIncoming(uint8_t & incomingSize)
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

	// Has the writter blanked the reader?
	const bool IsBlanking()
	{
		switch (State)
		{
		case StateEnum::Blanking:
		case StateEnum::BlankingWithPendingPacket:
			return true;
		default:
			return false;
		}
	}

	const uint32_t GetLastTimeStamp()
	{
		return LastTimeStamp;
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
			if (ValidatePreamble(LastTimeStamp - PacketStartTimestamp))
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
			if (DecodeBit(LastTimeStamp - BitTimestamp, bit))
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
			if (DecodeBit(LastTimeStamp - BitTimestamp, bit))
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
						Detach();
						State = StateEnum::WaitingForPacketClear;
#if defined(PIM_USE_STATIC_CALLBACK)
#if defined(PIM_SAFETY_CHECKS)
						if (ReceiveCallback != nullptr)
#endif
						{
							ReceiveCallback(PacketStartTimestamp);
						}
#else
#if defined(PIM_SAFETY_CHECKS)
						if (Callback != nullptr)
#endif
						{
							Callback->OnPacketReceived(PacketStartTimestamp);
						}
#endif
					}
					else
					{
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
#if defined(PIM_SAFETY_CHECKS)
				if (LostCallback != nullptr)
#endif
				{
					LostCallback(BitTimestamp);
				}
#else
#if defined(PIM_SAFETY_CHECKS)
				if (Callback != nullptr)
#endif
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

private:
	// Defined in cpp.
	void Attach();
	void SetupInterrupt();

	void Detach()
	{
		detachInterrupt(digitalPinToInterrupt(ReadPin));
	}

private:
	const bool ValidatePreamble(const uint32_t pulseDuration)
	{
		return (pulseDuration < Constants::PreambleIntervalMax) &&
			(pulseDuration > Constants::PreambleIntervalMin);
	}

	const bool DecodeBit(const uint32_t pulseSeparation, bool& bit)
	{
		if (pulseSeparation < Constants::OneIntervalMax)
		{
			if (pulseSeparation > Constants::OneIntervalMin) {
				bit = true;
				return true;
			}
			else if (pulseSeparation > Constants::ZeroIntervalMin)
			{
				bit = false;
				return true;
			}
		}

		// Invalid bit pulse interval.
		return false;
	}
};
#endif