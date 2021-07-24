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

#define PIM_NO_CHECKS
#include <PulseIntervalModulator.h>

#define _TASK_OO_CALLBACKS
#include <TaskSchedulerDeclarations.h>

template<const uint8_t MaxPacketSize, const uint8_t ReadPin, const uint8_t WritePin>
class PulsePacketTaskDriver : private Task, virtual public PacketReaderCallback, virtual public PacketWriterCallback
{
private:
	struct InterruptFlagsType
	{
		bool PacketLost = false;
		bool PacketReceived = false;
		bool PacketSent = false;
	};

	volatile InterruptFlagsType InterruptFlags;

	uint32_t IncomingStartTimestamp = 0;
	uint8_t IncomingPacket[Constants::MaxDataBytes];
	uint8_t OutgoingPacket[Constants::MaxDataBytes];

	PacketReader<MaxPacketSize, ReadPin> Reader;
	PacketWriter<MaxPacketSize, WritePin> Writer;

public:
	// Virtual calls to be overriden.
	virtual void OnDriverPacketReceived(const uint32_t startTimestamp, const uint8_t packetSize) {}

	virtual void OnDriverPacketLost(const uint32_t startTimestamp) {}

	virtual void OnDriverPacketSent() {}

public:
	PulsePacketTaskDriver(Scheduler* scheduler)
		: Task(0, TASK_FOREVER, scheduler, false)
		, PacketReaderCallback()
		, PacketWriterCallback()
		, Reader(IncomingPacket)
		, Writer()
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
			OnDriverPacketSent();
		}
		else
		{
			Task::disable();
			return false;
		}

		return true;
	}

	void OnWriterInterrupt()
	{
		Writer.OnWriterInterrupt();
	}

	void OnReaderInterrupt()
	{
		Reader.OnPulse();
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

	const bool CanSend()
	{
		return Reader.CanSend();
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