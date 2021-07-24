// Constants.h

#ifndef _CONSTANTS_h
#define _CONSTANTS_h

#include <stdint.h>

// Enable use of static callbacks, instead of interface.
// #define PIM_USE_STATIC_CALLBACK


#ifndef PIM_USE_STATIC_CALLBACK
class PacketWriterCallback
{
public:
	virtual void OnPacketSent() {}
};


class PacketReaderCallback
{
public:
	// After the callback, the buffer is considered free for the next incoming packet.
	virtual void OnPacketReceived(const uint32_t packetStartTimestamp) {}

	virtual void OnPacketLost(const uint32_t packetStartTimestamp) {}
};
#endif 

class Constants {
public:

	static const uint8_t MinDataBytes = 1; // 0b00000
	static const uint8_t MaxDataBytes = 64; // 0b11111 + 1
	static const uint8_t HeaderBits = 6;

	// All times in micro-seconds.
	static const uint32_t PreambleInterval = 100;
	static const uint32_t ZeroInterval = 50;
	static const uint32_t OneInterval = 75;

	// A 64 byte packet takes around ~26ms, pre amble included.
	static const uint32_t PacketTimeoutMicros = 30000;

	static const uint8_t IntervalTolerance = 14;

	static const uint32_t ZeroIntervalMin = ZeroInterval - IntervalTolerance;
	static const uint32_t ZeroIntervalMax = ZeroInterval + IntervalTolerance;

	static const uint32_t OneIntervalMin = OneInterval - IntervalTolerance;
	static const uint32_t OneIntervalMax = OneInterval + IntervalTolerance;

	static const uint32_t PreambleIntervalMin = PreambleInterval - IntervalTolerance;
	static const uint32_t PreambleIntervalMax = PreambleInterval + IntervalTolerance;

	static const bool DecodeBit(const uint32_t pulseSeparation, bool& bit)
	{
		if (pulseSeparation < OneIntervalMax)
		{
			if (pulseSeparation > OneIntervalMin) {
				bit = true;
				return true;
			}
			else if (pulseSeparation > ZeroIntervalMin)
			{
				bit = false;
				return true;
			}
		}

		return false;
	}

	static const bool EncodeBit(const uint32_t pulseSeparation, bool& bit)
	{
		if (pulseSeparation < OneIntervalMax)
		{
			if (pulseSeparation > OneIntervalMin) {
				bit = true;
				return true;
			}
			else if (pulseSeparation > ZeroIntervalMin)
			{
				bit = false;
				return true;
			}
		}

		return false;
	}

	static const bool ValidatePreamble(const uint32_t pulseDuration)
	{
		return (pulseDuration < PreambleIntervalMax) &&
			(pulseDuration > PreambleIntervalMin);
	}
};

#endif