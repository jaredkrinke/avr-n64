// NOTE: This implementation assumes >= 4 MHz clock speed and optimized output (including loop unrolling)
#define F_CPU 4000000UL // 4 MHz clock speed

#include <avr/io.h>
#include <util/delay.h>
#include <avr/sfr_defs.h>

#define MODIFY_BIT(p, b, v) (p) = ((p) & ~_BV(b)) | (((v) == 0) ? 0 : _BV(b))
#define _NOP() asm("nop;\n");

// Wait for next (microsecond) tick; call after a 2 cycle instruction
inline void waitForNextTick()
{
    for (int i = 0; i < ((F_CPU / 1000000) - 2); i++) {
        _NOP();
    }
}

// A short wait, just to ensure pin voltage has changed (~0.25 microseconds)
inline void waitShort()
{
    for (int i = 0; i < (((F_CPU / 2500000) > 0) ? (F_CPU / 2500000) : 1); i++) {
        _NOP();
    }
}

// Skip an entire (microsecond) tick
inline void skipTick()
{
    _NOP();
    _NOP();
    waitForNextTick();
}

// Write a single bit to the data line
inline void writeBit(unsigned char bit)
{
    // Drive low for 1 tick
    MODIFY_BIT(DDRB, 0, 1); // sbi (2 cycles)
    waitForNextTick();

    // Drive data value for 2 ticks
    MODIFY_BIT(DDRB, 0, (bit == 0) ? 1 : 0);
    waitForNextTick();
    MODIFY_BIT(DDRB, 0, (bit == 0) ? 1 : 0);
    waitForNextTick();

    // Drive high for 1 tick
    MODIFY_BIT(DDRB, 0, 0);
    waitForNextTick();
}

// Write the host stop bit to the data line to end a message
inline void writeStopBit()
{
    // Low for 1 tick, high for 2 ticks
    MODIFY_BIT(DDRB, 0, 1);
    waitForNextTick();
    MODIFY_BIT(DDRB, 0, 0);
    waitForNextTick();
    MODIFY_BIT(DDRB, 0, 0);
}

// Read a bit from the data line, but don't do anything with it
inline void skipReadBit()
{
    // Wait for data line to be driven low (and then skip 1 tick)
    loop_until_bit_is_clear(PINB, 0); // sbic (1 - 3 cycles), rjmp (2 cycles)
    waitForNextTick();
    skipTick();

    // Wait for data line to be back high
    loop_until_bit_is_set(PINB, 0);
}

// Read a bit from the data line and modify a corresponding output pin on port D
inline void readAndOutputBit(unsigned char bit)
{
    loop_until_bit_is_clear(PINB, 0);
    waitForNextTick();
    waitShort();
    MODIFY_BIT(PORTD, bit, bit_is_set(PINB, 0));
    loop_until_bit_is_set(PINB, 0);
}

void n64GetStateC()
{
    // Send "poll" message (0x01)
    writeBit(0);
    writeBit(0);
    writeBit(0);
    writeBit(0);
    writeBit(0);
    writeBit(0);
    writeBit(0);
    writeBit(1);

    writeStopBit();

    // Response
    skipReadBit();          // A
    readAndOutputBit(7);    // B
    skipReadBit();          // Z
    skipReadBit();          // Start
    readAndOutputBit(0);    // Up
    readAndOutputBit(3);    // Down
    readAndOutputBit(1);    // Left
    readAndOutputBit(2);    // Right
}

int main()
{
    // NEVER modify PORTB! We must only drive the line low and never high.
    PORTB = 0x00;

    // Port D is for output to LEDs
    DDRD = 0xff;

    // Short delay on boot
    _delay_ms(5);

    while (1)
    {
        n64GetStateC();
        _delay_ms(50);
    }
}
