// 
// 
// 

#include "PacketWriter.h"

PacketWriter* PulseIntervalModulatorWriter = nullptr;

#if defined(ARDUINO_ARCH_AVR)
ISR(TIMER0_COMPA_vect)
{
	PulseIntervalModulatorWriter->OnWriterInterrupt();
}
#endif

void PacketWriter::SetupInterrupt()
{
	PulseIntervalModulatorWriter = this;
}
