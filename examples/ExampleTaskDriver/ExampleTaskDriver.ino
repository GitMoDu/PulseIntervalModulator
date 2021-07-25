//
// Example of Task Driver, using the interface callbacks.
// Reads and Writes in Half-Duplex mode, avoiding collisions.
// Is based on OOP Class for Task Scheduler (https://github.com/arkhipenko/TaskScheduler).
//

#define DEBUG_LOG

#include <PulsePacketTaskDriver.h>

#define _TASK_OO_CALLBACKS
#include <TaskScheduler.h>


// Process scheduler.
Scheduler SchedulerBase;

const uint8_t MaxPacketSize = 32;
const uint8_t ReadPin = 2;
const uint8_t WritePin = 7;

PulsePacketTaskDriver<MaxPacketSize, ReadPin, WritePin> Driver(&SchedulerBase);


class SenderTask : public Task
{
private:
	static const uint32_t SendPeriodMillis = 500;

	PulsePacketTaskDriver<MaxPacketSize, ReadPin, WritePin>* PacketDriver = nullptr;

	uint8_t Message[6] = { 'H', 'e', 'l', 'l', 'o', '!'};

public:
	SenderTask(Scheduler* scheduler, PulsePacketTaskDriver<MaxPacketSize, ReadPin, WritePin>* packetDriver)
		: Task(SendPeriodMillis, TASK_FOREVER, scheduler, false)
		, PacketDriver(packetDriver)
	{
	}

	bool Callback()
	{
		PacketDriver->SendPacket(Message, 6);
	}
} SenderTask(&SchedulerBase, &Driver);


void setup()
{
#ifdef DEBUG_LOG
	Serial.begin(115200);
#endif

	attachInterrupt(digitalPinToInterrupt(ReadPin), OnReaderPulse, RISING);
	Driver.Start();

	SenderTask.enable();

#ifdef DEBUG_LOG
	Serial.println(F("ExampleTaskDriver Start."));
#endif
}

void loop()
{
	SchedulerBase.execute();
}

void OnReaderPulse()
{
	Driver.OnReaderInterrupt();
}

ISR(TIMER0_COMPA_vect)
{
	Driver.OnWriterInterrupt();
}
