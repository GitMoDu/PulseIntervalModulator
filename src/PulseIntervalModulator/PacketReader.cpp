// 
// 
// 

#include "PacketReader.h"

PacketReader* PulseIntervalModulatorReceiver = nullptr;

void OnReaderPulse()
{
	PulseIntervalModulatorReceiver->OnPulse();
}

void PacketReader::SetupInterrupt()
{
	PulseIntervalModulatorReceiver = this;
	pinMode(ReadPin, INPUT);
}

void PacketReader::Attach()
{
	attachInterrupt(digitalPinToInterrupt(ReadPin), OnReaderPulse, RISING);
}