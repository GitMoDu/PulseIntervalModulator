// Constants.h

#ifndef _CONSTANTS_h
#define _CONSTANTS_h

#include <stdint.h>

// Enable use of static callbacks, instead of interface.
// #define PIM_USE_STATIC_CALLBACK

// Remove checks for a faster operation, once flow is validated.
// #define PIM_NO_CHECKS

class Constants 
{
public:

	static const uint8_t MinDataBytes = 1; // 0b000000
	static const uint8_t MaxDataBytes = 64; // 0b111111 + 1
	static const uint8_t HeaderBits = 6;

	// All intervals in micro-seconds.
	static const uint32_t PreambleInterval = 100;
	static const uint32_t ZeroInterval = 50;
	static const uint32_t OneInterval = 75;

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