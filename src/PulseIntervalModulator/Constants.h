// Constants.h

#ifndef _PIM_CONSTANTS_h
#define _PIM_CONSTANTS_h


// Enable use of static callbacks, instead of interface.
#define PIM_USE_STATIC_CALLBACK

// Remove checks for a faster operation, once flow is validated.
#define PIM_SAFETY_CHECKS

#include <stdint.h>

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

	// Make sure we wait at least a bit over a pre amble before sending again.
	static const uint32_t ReceiveSilenceInterval = PreambleIntervalMax + IntervalTolerance;

	// Make sure we wait at least a bit over a pre amble before sending again.
	static const uint32_t SendSilenceInterval = PreambleIntervalMax + IntervalTolerance;
};
#endif