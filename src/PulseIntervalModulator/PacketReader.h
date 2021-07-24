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

template<const uint8_t MaxDataBytes>
class PacketReader
{
private:
	uint8_t IncomingBuffer[MaxDataBytes];
	uint8_t IncomingIndex = 0;
	volatile uint8_t IncomingSize = 0;

	uint8_t BitBuffer = 0;
	uint8_t BitIndex = 0;
	volatile uint32_t PacketStartTimestamp = 0;
	uint32_t BitTimestamp = 0;

	enum StateEnum
	{
		Blanking,
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


	const uint8_t ReadPin;

public:
	PacketReader(const uint8_t readPin)
		: ReadPin(readPin)
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
	}

	void Stop()
	{
		detachInterrupt(digitalPinToInterrupt(ReadPin));
		BlankReceive();
	}

	// Extra check to be called from the loop.
	void CheckForStuckPacket()
	{
		if (State == StateEnum::WaitingForDataBits &&
			(micros() - PacketStartTimestamp) > Constants::PacketTimeoutMicros)
		{
			// Stuck packet detected, restarting.
			Start();
		}
	}

	// Returns true if in the middle of receiving a packet.
	// Returns false if it is otherwise safe to transmit.
	const bool IsBusy()
	{
		switch (State)
		{
		case StateEnum::Blanking:
			return false;
		case StateEnum::WaitingForPreAmbleStart:
			return false;
		case StateEnum::WaitingForPreAmbleEnd:
			return true;
		case StateEnum::WaitingForHeaderEnd:
			return true;
		case StateEnum::WaitingForDataBits:
			return true;
		case StateEnum::WaitingForPacketClear:
			return false;
		default:
			return false;
		}
	}

	void BlankReceive()
	{
		State = StateEnum::Blanking;
	}

	const bool GetIncoming(uint8_t* rawPacket, uint8_t& length)
	{
		if (State == StateEnum::WaitingForPacketClear && IncomingSize > 0)
		{
			length = IncomingSize;
			for (uint8_t i = 0; i < length; i++)
			{
				rawPacket[i] = IncomingBuffer[i];
			}

			return true;
		}
		else
		{
			return false;
		}
	}

	void OnPulse()
	{
		uint32_t timestamp = micros();
		bool bit = false;
		switch (State)
		{
		case StateEnum::Blanking:
			// Ignore.
			break;
		case StateEnum::WaitingForPreAmbleStart:

			PacketStartTimestamp = timestamp;
			State = StateEnum::WaitingForPreAmbleEnd;
			break;
		case StateEnum::WaitingForPreAmbleEnd:
			if (Constants::ValidatePreamble(timestamp - PacketStartTimestamp))
			{
				// Preamble header detected.
				BitTimestamp = timestamp;

				// Take this time to reset the incoming buffer.
				IncomingIndex = 0;
				IncomingSize = 0;
				BitIndex = 0;

				State = StateEnum::WaitingForHeaderEnd;
			}
			else
			{
				// Restart assuming the last pulse was a start pulse.
				PacketStartTimestamp = timestamp;
				State = StateEnum::WaitingForPreAmbleEnd;
			}
			break;
		case StateEnum::WaitingForHeaderEnd:
			if (Constants::DecodeBit(timestamp - BitTimestamp, bit))
			{
				BitTimestamp = timestamp;

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
						PacketStartTimestamp = timestamp;
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
				PacketStartTimestamp = timestamp;
				State = StateEnum::WaitingForPreAmbleEnd;
			}
			break;
		case StateEnum::WaitingForDataBits:
			if (Constants::DecodeBit(timestamp - BitTimestamp, bit))
			{
				BitTimestamp = timestamp;

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
				PacketStartTimestamp = timestamp;
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