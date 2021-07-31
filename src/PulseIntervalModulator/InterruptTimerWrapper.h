// InterruptTimerWrapper.h

#ifndef _INTERRUPT_TIMER_WRAPPER_h
#define _INTERRUPT_TIMER_WRAPPER_h



#if defined(ARDUINO_AVR_ATTINYX5)
#elif defined(ARDUINO_ARCH_AVR)
#else
#error No timer wrapper implementation .
#endif	

#include <Arduino.h>

// This class should re-use the same timer used for the natives "micros()" call.
class InterruptTimerWrapper
{
private:
#if defined(ARDUINO_AVR_ATTINYX5)
	static const uint8_t TimerClocksDivisor = 8;
	enum InterruptDuration
	{
		PreAmble = 10,
		Zero = 4,
		One = 7
	};
#elif defined(ARDUINO_ARCH_AVR)
	enum InterruptDuration
	{
		PreAmble = 23,
		Zero = 9,
		One = 17
	};
	static const uint8_t TimerClocksDivisor = 64;
#endif


public:
	static constexpr uint16_t GetClocksFromMicros(const uint32_t delayMicros)
	{
		return (clockCyclesPerMicrosecond() * delayMicros) / TimerClocksDivisor;
	}

	static void DetachInterrupt()
	{
#if defined(ARDUINO_AVR_ATTINYX5)
		// Disable interrupt.
		TIMSK &= ~(1 << OCIE0A);

		// Clear interrupt flag.
		TIFR |= (1 << OCF0A);
		OCR0A = 0;
#elif defined(ARDUINO_ARCH_AVR)
		// Disable interrupt.
		TIMSK0 &= ~(1 << OCIE0A);

		// Clear interrupt flag.
		TIFR0 |= (1 << OCF0A);
		OCR0A = 0;
#endif
	}

	static void ConfigureTimer()
	{
		// Set Wave Form Generator bits to Normal mode. //Fast PWM, 0xFF Top.
		noInterrupts();
		uint8_t oldSREG = SREG;
		TCCR0A &= ~((1 << WGM01) + (1 << WGM00));
		TCCR0A |= ((0 << WGM01) + (0 << WGM00));
		TCCR0B &= ~(1 << WGM02);

		// Compare Output Mode, non-PWM Mode.
		TCCR0A &= ~((1 << COM0A1) + (1 << COM0A0));
		TCCR0B &= ~((1 << COM0B1) + (1 << COM0B0));

#if defined(ARDUINO_AVR_ATTINYX5)
		TIMSK |= TOIE0;
#elif defined(ARDUINO_ARCH_AVR)
		TIMSK0 |= TOIE0;
#endif	
		SREG = oldSREG;
		interrupts();
	}

	static void AttachInterrupt()
	{
		DetachInterrupt();
		ConfigureTimer();
#if defined(ARDUINO_AVR_ATTINYX5)
		TIMSK &= ~(1 << OCIE0A);
#elif defined(ARDUINO_ARCH_AVR)
		TIMSK0 &= ~(1 << OCIE0A);
#endif
	}

	static void InterruptAfterOne()
	{
		InterruptAfterClocks((uint8_t)InterruptDuration::One);
	}

	static void InterruptAfterZero()
	{
		InterruptAfterClocks((uint8_t)InterruptDuration::Zero);
	}

	static void InterruptAfterClocks(const uint8_t clocks)
	{
#if defined(ARDUINO_AVR_ATTINYX5)
		// Clear the interrupt flag while we setup the next one.
		TIFR |= (1 << OCF0A);

		// Set the new compare vale.
		OCR0A = (TCNT0 + clocks) % UINT8_MAX;

		// Enable interrupt.
		TIMSK |= (1 << OCIE0A);
#elif defined(ARDUINO_ARCH_AVR)
		// Clear the interrupt flag while we setup the next one.
		TIFR0 |= (1 << OCF0A);

		// Set the new compare vale.
		OCR0A = (TCNT0 + clocks) % UINT8_MAX;

		// Enable interrupt.
		TIMSK0 |= (1 << OCIE0A);
#endif
	}

	static void InterruptAfterPreamble()
	{
		InterruptAfterClocks((uint8_t)InterruptDuration::PreAmble);
	}
};
#endif