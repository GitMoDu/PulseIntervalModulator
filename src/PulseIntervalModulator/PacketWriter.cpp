// 
// 
// 

#include "PacketWriter.h"

PacketWriter* PulseIntervalModulatorWriter = nullptr;


ISR(TIMER0_COMPA_vect)
{
	PulseIntervalModulatorWriter->OnWriterInterrupt();
}

void PacketWriter::SetupInterrupt()
{
	PulseIntervalModulatorWriter = this;
}
