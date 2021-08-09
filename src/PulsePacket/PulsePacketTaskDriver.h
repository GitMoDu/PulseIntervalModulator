// PulsePacketTaskDriver
// Implementation of PulseIntervalModulator with a cooperative task scheduler.
// Half-Duplex communication.
// With buffered input and output packets.
// Collision avoidance.
// Driver callbacks running on main loop.
// 
// Depends Task Scheduler (https://github.com/arkhipenko/TaskScheduler)

#ifndef _EXAMPLEDRIVERCLASS_h
#define _EXAMPLEDRIVERCLASS_h

#include <PulseIntervalModulator.h>

#define _TASK_OO_CALLBACKS
#include <TaskSchedulerDeclarations.h>


template<const uint8_t MaxPacketSize>
class PulsePacketTaskDriver : protected Task, virtual public PacketReaderCallback, virtual public PacketWriterCallback
{
private:
	struct InterruptFlagsType
	{
		volatile bool PacketLost = false;
		volatile bool PacketReceived = false;
		volatile bool PacketSent = false;
	};

	PacketReader Reader;
	PacketWriter Writer;

	InterruptFlagsType InterruptFlags;

protected:
	volatile uint32_t IncomingStartTimestamp = 0;
	volatile uint32_t LastWriterTimestamp = 0;
	uint8_t IncomingPacket[MaxPacketSize];
	uint8_t OutgoingPacket[MaxPacketSize];

protected:
	// Virtual calls to be overriden.
	virtual void OnDriverPacketReceived(const uint32_t startTimestamp, const uint8_t packetSize) {}

	virtual void OnDriverPacketLost(const uint32_t startTimestamp) {}

	virtual void OnDriverPacketSent() {}

public:
#if defined(ARDUINO_ARCH_AVR)
	PulsePacketTaskDriver(Scheduler* scheduler, const uint8_t readPin, const uint8_t writePin)
#elif defined(ARDUINO_ARCH_STM32F1)
	PulsePacketTaskDriver(Scheduler* scheduler, const uint8_t readPin, const uint8_t writePin, const uint8_t timerIndex, const uint8_t timerChannel)
#endif
		: PacketReaderCallback()
		, PacketWriterCallback()
		, Task(0, TASK_FOREVER, scheduler, false)
		, Reader(IncomingPacket, MaxPacketSize, readPin)
#if defined(ARDUINO_ARCH_AVR)
		, Writer(MaxPacketSize, writePin)
#elif defined(ARDUINO_ARCH_STM32F1)
		, Writer(MaxPacketSize, writePin, timerIndex, timerChannel)
#endif
		, InterruptFlags()
	{}

	bool Callback()
	{
		if (InterruptFlags.PacketLost)
		{
			InterruptFlags.PacketLost = false;
			OnDriverPacketLost(IncomingStartTimestamp);
		}
		else if (InterruptFlags.PacketReceived)
		{
			InterruptFlags.PacketReceived = false;

			uint8_t incomingSize = 0;

			if (Reader.HasIncoming(incomingSize))
			{
				OnDriverPacketReceived(IncomingStartTimestamp, incomingSize);
			}

			// Restore Reader after consuming the packet.
			Reader.Restore();
		}
		else if (InterruptFlags.PacketSent)
		{
			InterruptFlags.PacketSent = false;
			LastWriterTimestamp = micros();
			OnDriverPacketSent();
		}
		else
		{
			Task::disable();
			return false;
		}

		return true;
	}

	// Must perform interrupt attach before starting.
	// attachInterrupt(digitalPinToInterrupt(ReadPin), OnPulse, RISING);
	void Start()
	{
		Reader.Start(this);
		Writer.Start(this);
	}

	void Stop()
	{
		Reader.Stop();
		Writer.Stop();
	}

	// Returns false if in the middle of receiving or sending a packet.
	// Returns true the minimum silenceInterval has been observed in both ways.
	const bool CanSend()
	{
		uint32_t now = micros();
		return !Reader.IsBlanking() // Has the writter blanked the reader?
			&& (now - Reader.GetLastTimeStamp() > Constants::ReceiveSilenceInterval) // Has enough time passed since last pulse in?
			&& (now - LastWriterTimestamp > Constants::SendSilenceInterval); // Has enough time passed since last pulse out?
	}

	// Must check with CanSend() right before this call.
	void SendPacket(uint8_t* packetData, const uint8_t packetSize)
	{
		// Blank reader to ignore cross-talk.
		Reader.BlankReceive();

		// Copy to out buffer.
		for (uint8_t i = 0; i < packetSize; i++)
		{
			OutgoingPacket[i] = packetData[i];
		}

		// Start sending in the background.
		Writer.SendPacket(OutgoingPacket, packetSize);
	}

	virtual void OnPacketReceived(const uint32_t startTimestamp)
	{
		// Flag event and wake up task.
		InterruptFlags.PacketReceived = true;
		IncomingStartTimestamp = startTimestamp;

		Task::enable();
	}

	virtual void OnPacketLost(const uint32_t startTimestamp)
	{
		// Flag event and wake up task.
		InterruptFlags.PacketLost = true;
		IncomingStartTimestamp = startTimestamp;

		Task::enable();
	}

	virtual void OnPacketSent()
	{
		// Restore Reader after blanking during sending.
		Reader.Restore();

		// Flag event and wake up task.
		InterruptFlags.PacketSent = true;

		Task::enable();
	}
};
#endif