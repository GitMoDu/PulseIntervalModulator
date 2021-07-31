// PacketWriter.h
// Bit bangs out the packets using rolling Timer0 interrupts on Channel A.
// Channel B is still free.
// Does not affect millis(), micros() or delay().
// All work is done during interrupts.
#ifndef _PIM_PACKET_WRITER_h
#define _PIM_PACKET_WRITER_h

#include <Fast.h>
#if !defined(_FAST_h)
#error Gotta go fast! (.h)
#error Arduino general digitalWrite support not included, make a pull request.
#endif


#include "Constants.h"
#include "InterruptTimerWrapper.h"


#if !defined(PIM_USE_STATIC_CALLBACK)
class PacketWriterCallback
{
public:
	virtual void OnPacketSent() {}
};
#endif 

class PacketWriter
{
private:
	FastOut PinOut;

	enum WriteState
	{
		Done = 0,
		WritingHeader = 1,
		WritingDataBits = 2
	};

	volatile WriteState State = WriteState::Done;

	uint8_t* RawOutputData = nullptr;
	volatile uint8_t PacketSize = 0;
	volatile uint8_t RawOutputByte = 0;
	volatile uint8_t RawOutputBit = 0;

#if defined(PIM_USE_STATIC_CALLBACK)
	void (*Callback)(void) = nullptr;
#else
	PacketWriterCallback* Callback = nullptr;
#endif

	const uint8_t MaxDataBytes = 0;

public:
	PacketWriter(const uint8_t maxDataBytes, const uint8_t writePin)
		: PinOut(writePin, false)
		, MaxDataBytes(maxDataBytes)
	{
	}

#if defined(PIM_USE_STATIC_CALLBACK)
	void Start(void (*callback)(void))
#else
	void Start(PacketWriterCallback* callback)
#endif
	{
		Callback = callback;
		SetupInterrupt();
		Start();
	}

	void Start()
	{
		State = WriteState::Done;
		InterruptTimerWrapper::AttachInterrupt();
	}

	void Stop()
	{
		State = WriteState::Done;
		InterruptTimerWrapper::DetachInterrupt();
	}

	void OnWriterInterrupt()
	{
		PulseOut();
		switch (State)
		{
		case WriteState::Done:
			// Last pulse is out.
#if defined(PIM_SAFETY_CHECKS)
			if (Callback != nullptr)
#endif
			{
#if defined(PIM_USE_STATIC_CALLBACK)
				Callback();
#else
				Callback->OnPacketSent();
#endif
			}
			InterruptTimerWrapper::DetachInterrupt();
			break;
		case WriteState::WritingHeader:
			// Sending header with packet size MSB first.
			// Remove MinDataBytes, according to specification.
			if ((PacketSize - Constants::MinDataBytes) & (1 << (Constants::HeaderBits - 1 - RawOutputBit)))
			{
				InterruptTimerWrapper::InterruptAfterOne();
			}
			else
			{
				InterruptTimerWrapper::InterruptAfterZero();
			}
			RawOutputBit++;

			if (RawOutputBit > (Constants::HeaderBits - 1))
			{
				// Header almost done, start pushing on the next interrupt.
				State = WriteState::WritingDataBits;
				RawOutputBit = 0;
			}
			break;
		case WriteState::WritingDataBits:
			// Sending data with MSB first.
			if ((RawOutputData[RawOutputByte] >> (7 - RawOutputBit)) & 0x01)
			{
				InterruptTimerWrapper::InterruptAfterOne();
			}
			else
			{
				InterruptTimerWrapper::InterruptAfterZero();
			}
			RawOutputBit++;

			if (RawOutputBit > 7)
			{
				// One full byte written.
				RawOutputByte++;
				RawOutputBit = 0;

				if (RawOutputByte > (PacketSize - Constants::MinDataBytes))
				{
					// Bits almost done, just transmit the last bit.
					State = WriteState::Done;
				}
			}
			break;
		default:
			break;
		}
	}

private:
	// Defined in cpp.
	void SetupInterrupt();

private:
	void PulseOut()
	{
		PinOut.PulseHigh();
	}

public:
	// packetData must not be a valid array.
	void SendPacket(uint8_t* packetData, const uint8_t packetSize)
	{
#if defined(PIM_SAFETY_CHECKS)
		if (packetData == nullptr || packetSize > MaxDataBytes || packetSize < Constants::MinDataBytes)
		{
			return;
		}
#endif 
		RawOutputData = packetData;

		PacketSize = packetSize;

		RawOutputByte = 0;
		RawOutputBit = 0;

		// PreAmble and Packet start sequence.
		State = WriteState::WritingHeader;
		PulseOut();
		InterruptTimerWrapper::InterruptAfterPreamble();
	}
};
#endif