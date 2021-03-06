//
// Example of Task Driver, using the interface callbacks.
// Reads and Writes in Half-Duplex mode, avoiding collisions.
// Is based on OOP Class for Task Scheduler (https://github.com/arkhipenko/TaskScheduler).
//

#define DEBUG_LOG

#define _TASK_OO_CALLBACKS
#include <TaskScheduler.h>

// #define PIM_USE_STATIC_CALLBACK must be disabled in Constants.
#include <PulsePacketTaskDriver.h>

// Process scheduler.
Scheduler SchedulerBase;

const uint8_t MaxPacketSize = 32;
const uint8_t ReadPin = 2;
const uint8_t WritePin = 7;

static const uint8_t TimerIndex = 1;// Timer 1 does not conflict with SPI.
static const uint8_t TimerChannelIndex = TIMER_CH1; // Pin (D27) PA8. 

PulsePacketTaskDriver<MaxPacketSize> Driver(&SchedulerBase, ReadPin, WritePin, TimerIndex, TimerChannelIndex);


class SenderTask : public Task
{
private:
	static const uint32_t SendPeriodMillis = 500;

	PulsePacketTaskDriver<MaxPacketSize>* PacketDriver = nullptr;

	uint8_t Message[6] = { 'H', 'e', 'l', 'l', 'o', '!' };

public:
	SenderTask(Scheduler* scheduler, PulsePacketTaskDriver<MaxPacketSize>* packetDriver)
		: Task(SendPeriodMillis, TASK_FOREVER, scheduler, false)
		, PacketDriver(packetDriver)
	{
	}

	bool Callback()
	{
		PacketDriver->SendPacket(Message, 6);

		return true;
	}
} SenderTask(&SchedulerBase, &Driver);


void setup()
{
#ifdef DEBUG_LOG
	Serial.begin(115200);
#endif

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