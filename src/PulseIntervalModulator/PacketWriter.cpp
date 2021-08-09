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

void PacketWriter::SetupInterrupt()
{
	PulseIntervalModulatorWriter = this;
}
#elif defined(ARDUINO_ARCH_STM32F1)
void OnWriterTimerInterrupt()
{
	PulseIntervalModulatorWriter->OnWriterInterrupt();
}

void PacketWriter::SetupInterrupt()
{
	TimerWrapper.ConfigureTimer(OnWriterTimerInterrupt);
}
#endif