# Pulse Interval Modulator

Arduino based Pulse Interval Modulator/Demodulator

Demodulator
Catches the pulse stream and builds up a packet buffer, as long as the incoming bits are valid, otherwise it resets.
All work is done during interrupts.

Modulator
Bit bangs out the packets using rolling Timer0 PWM interrupt on Channel A. 
Does not affect millis(), micros() or delay(). Channel B is still free.
All work is done during interrupts.