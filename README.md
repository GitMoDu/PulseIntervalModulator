# Pulse Interval Modulator

Arduino based Pulse Interval Modulator/Demodulator.

Encodes/Decodes a simple custom protocol based on the interval between pulses.



![](https://github.com/GitMoDu/PulseIntervalModulator/blob/master/Media/example_1_byte.png?raw=true)

Example 1 Byte packet.


## Demodulator

Catches the pulse stream and builds up a packet buffer, as long as the incoming bits are valid, otherwise it resets.
All work is done during interrupts.

## Modulator
Bit bangs out the packets using rolling Timer0 PWM interrupt on Channel A. 
Does not affect millis(), micros() or delay(). Channel B is still free.
All work is done during interrupts.

## Protocol

- Initial pulse to start preamble.
- Pulse on preamble interval.
- Encoded pulses with size of packet (6 bits).
- Encoded pulses with data bits.
