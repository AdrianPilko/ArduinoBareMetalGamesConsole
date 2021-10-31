// PAL video output using atmega328p.
// can be ran on arduino board, pin and 12 and 13 go to centre of a 10k pot, pin 12 through a 330 Ohm resistor.
// one side of the pot goes to VCC the other to ground and the centre of the pot also goes to the composite connector
// centre pin. if using atmega328p on bare board (not arduino) then its PB5 and PB4.

#include<avr/io.h>
#include <util/delay.h>

#define COMPOSITE_PIN PB5
#define LUMINANCE_PIN PB4
#define MAX_LINE_BEFORE_BLANK 312
#define FIRST_LINE_DRAWN 28
#define LAST_LINE_DRAWN 200
#define HALF_SCREEN 100


#define HSYNC_BACKPORCH 18
#define HSYNC_FRONT_PORCH_2 15

// from experimenting a line started at linCounter = 25 appears right at top of screen one at 285 to 286 is bottom
// a delay in clock cycles of 14 to plus a delay of 148 clock cycles gives a usable horizontal line length

// XSIZE is in bytes, and currently due to lack of time (spare clock cycles) only able todo 8 *8(bits) pixels = 64
// will need some serious optimisation later!
#define XSIZE 8
#define YSIZE (HALF_SCREEN*2)



int main()
{
	uint16_t lineCounter = 0;
	uint8_t  vSync = 0;
	uint8_t i = 0;
	uint8_t drawPixelsOnLine = 0;
	uint8_t afterStartOfLineSync = 0;
	uint8_t screenMemory[YSIZE][XSIZE];
	uint8_t screenLine = 0;
	uint8_t pixelOn = 1;
	uint8_t lineLength = 0;
	uint8_t everyOther = 0;
	uint8_t yCounter = 0;

	// looking at the data sheet not sure if you have to do this in 3 instructions or not
	CLKPR = 0b10000000; // set the CLKPCE bit to enable the clock prescaler to be changed
	CLKPR = 0b10000000; // zero the CLKPS3 CLKPS2 CLKPS1 CLKPS0 bits to disable and clock division
	CLKPR = 0b00000000; // clear CLKPCE bit

	// setup a 10 line checkerboard in memory for testing
	for (uint8_t y = 0; y < YSIZE; y+=20)
	{
		for (uint8_t x = 0; x < XSIZE; x++)
		{
			screenMemory[y][x]  = 0b00001111;
			screenMemory[y+1][x] = 0b00001111;
			screenMemory[y+2][x] = 0b00001111;
			screenMemory[y+3][x] = 0b00001111;
			screenMemory[y+4][x] = 0b00001111;
			screenMemory[y+5][x] = 0b00001111;
			screenMemory[y+6][x] = 0b00001111;
			screenMemory[y+7][x] = 0b00001111;
			screenMemory[y+8][x] = 0b00001111;
			screenMemory[y+9][x] = 0b00001111;

			screenMemory[y+10][x]  = 0b11110000;
			screenMemory[y+11][x]  = 0b11110000;
			screenMemory[y+12][x]  = 0b11110000;
			screenMemory[y+13][x]  = 0b11110000;
			screenMemory[y+14][x]  = 0b11110000;
			screenMemory[y+15][x]  = 0b11110000;
			screenMemory[y+16][x]  = 0b11110000;
			screenMemory[y+17][x]  = 0b11110000;
			screenMemory[y+18][x]  = 0b11110000;
			screenMemory[y+19][x]  = 0b11110000;
		}
	}



	DDRB |= 1 << COMPOSITE_PIN;
	DDRB |= 1 << LUMINANCE_PIN;

	TCCR1B = (1<<CS10); // switch off clock prescaller
	OCR1A = 1025; 	   //timer interrupt cycles which gives rise to 64usec line sync time
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
			for (i = 0; i < HSYNC_BACKPORCH+10; i++)// delay same amount to give proper back porch before drawing any pixels on the line
			{
				__asm__ __volatile__ ("nop");
			}
			afterStartOfLineSync = 1;
		}

		// the whole line writing must be a total of less than (64 - (18 * 3)) = 10usec =
		// a delay in line 1 = 1/16000000 =0.0000000625 assuming nop = 1 clock cycle


		// flag "everyOther" ensures we update half the pixels on alternate frames
		// this is similar to the PAL interlacing, and gives us more clock cycles for
		// game code!
		if ((drawPixelsOnLine) && (everyOther))
		{
			uint8_t * loopPtrMax = screenMemory[yCounter+1];
			uint8_t * OneLine = screenMemory[yCounter];
			do {
				if (*OneLine & (1 <<  0))
					{DDRB |= (1 << LUMINANCE_PIN); PORTB |= (1 << LUMINANCE_PIN);}
				else
					{DDRB &= ~(1 << LUMINANCE_PIN); PORTB &= ~(1 << LUMINANCE_PIN); }
				if (*OneLine & (1 <<  1))
					{DDRB |= (1 << LUMINANCE_PIN); PORTB |= (1 << LUMINANCE_PIN);}
				else
					{DDRB &= ~(1 << LUMINANCE_PIN); PORTB &= ~(1 << LUMINANCE_PIN); }
				if (*OneLine & (1 <<  2))
					{DDRB |= (1 << LUMINANCE_PIN); PORTB |= (1 << LUMINANCE_PIN);}
				else
					{DDRB &= ~(1 << LUMINANCE_PIN); PORTB &= ~(1 << LUMINANCE_PIN); }
				if (*OneLine & (1 <<  3))
					{DDRB |= (1 << LUMINANCE_PIN); PORTB |= (1 << LUMINANCE_PIN);}
				else
					{DDRB &= ~(1 << LUMINANCE_PIN); PORTB &= ~(1 << LUMINANCE_PIN); }
				if (*OneLine & (1 <<  4))
					{DDRB |= (1 << LUMINANCE_PIN); PORTB |= (1 << LUMINANCE_PIN);}
				else
					{DDRB &= ~(1 << LUMINANCE_PIN); PORTB &= ~(1 << LUMINANCE_PIN); }
				if (*OneLine & (1 <<  5))
					{DDRB |= (1 << LUMINANCE_PIN); PORTB |= (1 << LUMINANCE_PIN);}
				else
					{DDRB &= ~(1 << LUMINANCE_PIN); PORTB &= ~(1 << LUMINANCE_PIN); }
				if (*OneLine & (1 <<  6))
					{DDRB |= (1 << LUMINANCE_PIN); PORTB |= (1 << LUMINANCE_PIN);}
				else
					{DDRB &= ~(1 << LUMINANCE_PIN); PORTB &= ~(1 << LUMINANCE_PIN); }
				if (*OneLine & (1 <<  7))
					{DDRB |= (1 << LUMINANCE_PIN); PORTB |= (1 << LUMINANCE_PIN);}

				OneLine++;
			}
			while (OneLine < loopPtrMax);
		}
		DDRB &= ~(1 << LUMINANCE_PIN); // pixel off
		PORTB &= ~(1 << LUMINANCE_PIN); // pixel off
		lineCounter++;
		if (drawPixelsOnLine) yCounter++;

		switch (lineCounter)
		{
			case 1: vSync = 0; break;
			case FIRST_LINE_DRAWN: screenLine = 0; drawPixelsOnLine = 1; break;
			case LAST_LINE_DRAWN: drawPixelsOnLine = 0; yCounter = 0;
								DDRB &= ~(1 << LUMINANCE_PIN); // pixel off
								PORTB &= ~(1 << LUMINANCE_PIN); // pixel off
			break;
			case MAX_LINE_BEFORE_BLANK-6: vSync = 1; drawPixelsOnLine = 0; break;
			case MAX_LINE_BEFORE_BLANK: lineCounter = 0; vSync = 0; everyOther = 1 - everyOther; break;
		}
	}
}


#if 0
else
{
	DDRB |= (1 << LUMINANCE_PIN); // pixel on
	PORTB |= (1 << LUMINANCE_PIN); // pixel on
	for (i = 0; i < 205; i++)
	{
		__asm__ __volatile__ ("nop");
	}

	DDRB &= ~(1 << LUMINANCE_PIN); // pixel off
	PORTB &= ~(1 << LUMINANCE_PIN); // pixel off
}

#endif
