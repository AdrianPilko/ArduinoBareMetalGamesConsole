// PAL video output using atmega328p.
// can be ran on arduino board, pin and 12 and 13 go to centre of a 10k pot, pin 12 through a 330 Ohm resistor.
// one side of the pot goes to VCC the other to ground and the centre of the pot also goes to the composite connector
// centre pin. if using atmega328p on bare board (not arduino) then its PB5 and PB4.

#include<avr/io.h>
#include <util/delay.h>

#define COMPOSITE_PIN PB5
#define LUMINANCE_PIN PB4
#define MAX_LINE_BEFORE_BLANK 312

#define HSYNC_BACKPORCH 18

// from experimenting a line started at linCounter = 25 appears right at top of screen one at 285 to 286 is bottom
// a delay in clock cycles of 14 to to 14+148 gives a useable horizontal line length
// this gives us a possible screen memory dimension of
#define XSIZE 100
#define YSIZE 200

int main()
{
	uint16_t lineCounter = 0;
	uint8_t  vSync = 0;
	uint8_t i = 0;
	uint8_t drawPixel = 0;
	uint8_t xStart = 80; // time in clock cycles to delay into line to start drawing a pixel
	uint8_t xEnd = 90; // time in clock cycles to delay into line to start drawing a pixel
	uint8_t screenMemory[YSIZE][XSIZE];


	DDRB |= 1 << COMPOSITE_PIN;
	DDRB |= 1 << LUMINANCE_PIN;

	TCCR1B = (1<<CS10); // switch off clock prescaller
	OCR1A = 1025; 	   //usec delay
	TCNT1 = 0;
	while(1)
	{
		// wait for hsync timer interrupt to trigger
		while((TIFR1 & (1<<OCF1A)) == 0)
		{
			// wait till the timer overflow flag is SET
		}

		// immediately reset the interrupt timer, previous version had this at the end
		// which made the whole thing be delayed when we're trying to maintain the 64us PAL line timing
		TCNT1 = 0;
		TIFR1 |= (1<<OCF1A) ; //clear timer1 overflow flag

		if (vSync > 0) // invert the line sync pulses when in vsync part of screen
		{
			DDRB &= ~(1 << COMPOSITE_PIN);
			for (i = 0; i < HSYNC_BACKPORCH; i++)
			{
				__asm__ __volatile__ ("nop");
			}
			PORTB &= 0 << COMPOSITE_PIN; // set all PORTB COMPOSITE_PIN to low, as DDRB in next statement is input this causes output to go low
			DDRB |= (1 << COMPOSITE_PIN);
		}
		else // hsync
		{
			// before the end of line (which in here is also effectively just the start of the line!) we need to hold at 300mV
			// and ensure no pixels
			DDRB &= ~(1 << COMPOSITE_PIN); // set COMPOSITE_PIN as input
			PORTB |= 1 << COMPOSITE_PIN; // set PORTB COMPOSITE_PIN to high, as DDRB in input this causes output to go high
			DDRB &= ~(1 << LUMINANCE_PIN);
			PORTB &= ~(1 << LUMINANCE_PIN);

			for (i = 0; i < HSYNC_BACKPORCH; i++)// delay same amount to give proper back porch before drawing any pixels on the line
			{
				__asm__ __volatile__ ("nop");
			}

			// Hold the output to the composite connector low, the zero volt hsync
			PORTB &= 0 << COMPOSITE_PIN; // set all PORTB COMPOSITE_PIN to low, as DDRB in next statement is input this causes output to go low
			DDRB |= (1 << COMPOSITE_PIN); // set COMPOSITE_PIN as output
			for (i = 0; i < HSYNC_BACKPORCH; i++)
			{
				__asm__ __volatile__ ("nop");
			}

			// hold output to composite connector to 300mV
			DDRB &= ~(1 << COMPOSITE_PIN); // set COMPOSITE_PIN as input
			PORTB |= 1 << COMPOSITE_PIN; // set PORTB COMPOSITE_PIN to high, as DDRB in input this causes output to go high
			for (i = 0; i < HSYNC_BACKPORCH; i++)// delay same amount to give proper back porch before drawing any pixels on the line
			{
				__asm__ __volatile__ ("nop");
			}


			// test to just draw a the smallest pixel possible
			// none of this is good for doing real screen drawing
			// the whole line writing must be a total of less than (64 - (18 * 3)) = 10usec
			if (drawPixel)
			{
				for (i = 0; i < xStart; i++)// delay in line 1 = 1/16000000 =0.0000000625 assuming nop = 1 clock cycle
				{
					__asm__ __volatile__ ("nop");
				}
				DDRB |= 1 << LUMINANCE_PIN;
				PORTB |= 1 << LUMINANCE_PIN;

				for (i = 0; i < xEnd; i++)// delay in line 1 = 1/16000000 =0.0000000625 assuming nop = 1 clock cycle
				{
					__asm__ __volatile__ ("nop");
				}

				DDRB &= ~(1 << LUMINANCE_PIN);
				PORTB &= ~(1 << LUMINANCE_PIN);
			}
		}
		lineCounter++;
		switch (lineCounter)
		{
			case 1: vSync = 0; break;
			//case 25: drawPixel = 1; xStart = 30; xEnd = 100; break;
			//case 26: drawPixel = 0; break;
			case 285: drawPixel = 1; xStart = 14; xEnd = 148; break;
			case 286: drawPixel = 0; break;

			case MAX_LINE_BEFORE_BLANK-6: vSync = 1; drawPixel = 0; break;
			case MAX_LINE_BEFORE_BLANK: lineCounter = 0; vSync = 0; break;
		}
	}
}
