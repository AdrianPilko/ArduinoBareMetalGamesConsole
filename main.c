// PAL video output using atmega328p.
// can be ran on arduino board, pin and 12 and 13 go to centre of a 10k pot, pin 12 through a 330 Ohm resistor.
// one side of the pot goes to VCC the other to ground and the centre of the pot also goes to the composite connector
// centre pin. if using atmega328p on bare board (not arduino) then its PB5 and PB4.

#include <avr/power.h>
#include<avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>
#include "charset.h"

#define COMPOSITE_PIN PB5
#define LUMINANCE_PIN PB4
#define MAX_LINE_BEFORE_BLANK 312
#define FIRST_LINE_DRAWN 42
#define LAST_LINE_DRAWN 262
#define HALF_SCREEN 110


#define HSYNC_BACKPORCH 18
#define HSYNC_FRONT_PORCH_2 15

// from experimenting a line started at linCounter = 25 appears right at top of screen one at 285 to 286 is bottom
// a delay in clock cycles of 14 to plus a delay of 148 clock cycles gives a usable horizontal line length

// XSIZE is in bytes, and currently due to lack of time (spare clock cycles) only able todo 8 *8(bits) pixels = 64
// will need some serious optimisation later!
#define XSIZE 8
#define YSIZE (HALF_SCREEN*2)

#define SQUARE 26
#define SQUARE_WITH_DOT 27
#define EGG_TIMER 28
#define SOLID_ON 29
#define SPACE 30
#define COMMA 31
#define EXCLAMATION 32
#define ALIEN_1 33
#define ALIEN_2 34

#define LINE_INC 8
#define LINE_INC_ALIENS 12
#define TOGGLE_RATE 32

uint8_t screenMemory[YSIZE][XSIZE];

inline void putCharXY(uint8_t x,uint8_t y, uint8_t character)
{
	for (uint8_t i = 0; i < 8; i++)
	{
		screenMemory[y+i][x] = alphafonts[character][i];
	}
}

inline uint8_t convertToMyCharSet(char charToConvert)
{
	uint8_t rv = (uint8_t)charToConvert - 65;

	if ((rv > 25+9) || (rv < 0))
	{ // check for special char
		switch (charToConvert)
		{
			case ',' : rv = COMMA; break;
			case '!' : rv = EXCLAMATION; break;
			case ' ' : rv = SPACE; break;
			case '[' : rv = SQUARE; break;
			case '}' : rv = SQUARE_WITH_DOT; break;
			case '#' : rv = SOLID_ON; break;
			case '£' : rv = ALIEN_1; break;
			case '$' : rv = ALIEN_2; break;
			default: rv = 4;  break; // "E for error!"
		};
	}

	return rv;
}

uint8_t alienX = 0;  // top most alien x pos
uint8_t alienY = 0;
uint8_t keepXCount = 0;
uint8_t alienToggle = 0;

void animateAliens()
{

	keepXCount++;
	if (keepXCount == 3)
	{
		keepXCount = 0;

		uint8_t lineTemp = alienY;
		putCharXY(alienX,lineTemp,convertToMyCharSet(' '));
		lineTemp+= LINE_INC_ALIENS;
		putCharXY(alienX,lineTemp,convertToMyCharSet(' '));
		lineTemp+= LINE_INC_ALIENS;
		putCharXY(alienX,lineTemp,convertToMyCharSet(' '));
		lineTemp+= LINE_INC_ALIENS;
		putCharXY(alienX,lineTemp,convertToMyCharSet(' '));
		lineTemp+= LINE_INC_ALIENS;
		putCharXY(alienX,lineTemp,convertToMyCharSet(' '));
		lineTemp+= LINE_INC_ALIENS;
		putCharXY(alienX,lineTemp,convertToMyCharSet(' '));

		alienX = alienX + 1;
		if (alienX > 6) alienX = 0;
	}

	if (alienToggle)
	{

		uint8_t lineTemp = alienY;
		putCharXY(alienX,lineTemp,convertToMyCharSet('$'));
		lineTemp+= LINE_INC_ALIENS;
		putCharXY(alienX,lineTemp,convertToMyCharSet('$'));
		lineTemp+= LINE_INC_ALIENS;
		putCharXY(alienX,lineTemp,convertToMyCharSet('$'));
		lineTemp+= LINE_INC_ALIENS;
		putCharXY(alienX,lineTemp,convertToMyCharSet('$'));
		lineTemp+= LINE_INC_ALIENS;
		putCharXY(alienX,lineTemp,convertToMyCharSet('$'));
		lineTemp+= LINE_INC_ALIENS;
		putCharXY(alienX,lineTemp,convertToMyCharSet('$'));
	}
	else
	{
		uint8_t lineTemp = alienY;
		putCharXY(alienX,lineTemp,convertToMyCharSet('£'));
		lineTemp+= LINE_INC_ALIENS;
		putCharXY(alienX,lineTemp,convertToMyCharSet('£'));
		lineTemp+= LINE_INC_ALIENS;
		putCharXY(alienX,lineTemp,convertToMyCharSet('£'));
		lineTemp+= LINE_INC_ALIENS;
		putCharXY(alienX,lineTemp,convertToMyCharSet('£'));
		lineTemp+= LINE_INC_ALIENS;
		putCharXY(alienX,lineTemp,convertToMyCharSet('£'));
		lineTemp+= LINE_INC_ALIENS;
		putCharXY(alienX,lineTemp,convertToMyCharSet('£'));
	}
	alienToggle = 1 - alienToggle;
}


void clearScreen()
{
	memset (&screenMemory[0][0],0,sizeof(uint8_t) * YSIZE * XSIZE );
}

int main()
{
	uint8_t alienToggleCountDown = 128;
	uint8_t printScreen = 2;

	uint16_t lineCounter = 0;
	uint8_t  vSync = 0;
	uint8_t i = 0;
	uint8_t drawPixelsOnLine = 0;
	uint8_t yCounter = 0;

	clock_prescale_set(clock_div_1);

	clearScreen();

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
			DDRB = 0;
			for (i = 0; i < HSYNC_BACKPORCH+10; i++)
			{
				__asm__ __volatile__ ("nop");
			}
			PORTB = 0;
			DDRB = 1;
		}
		else // hsync
		{
			// before the end of line (which in here is also effectively just the start of the line!) we need to hold at 300mV
			// and ensure no pixels
			DDRB = 0;
			PORTB = 1;
			for (i = 0; i < 1; i++)
			{
				__asm__ __volatile__ ("nop");
			}

			// Hold the output to the composite connector low, the zero volt hsync
			PORTB = 0;
			DDRB |= (1 << COMPOSITE_PIN); // set COMPOSITE_PIN as output
			for (i = 0; i < HSYNC_BACKPORCH; i++)
			{
				__asm__ __volatile__ ("nop");
			}

			// hold output to composite connector to 300mV
			DDRB &= ~(1 << COMPOSITE_PIN); // set COMPOSITE_PIN as input
			PORTB = 1;
			for (i = 0; i < HSYNC_BACKPORCH; i++)// delay same amount to give proper back porch before drawing any pixels on the line
			{
				__asm__ __volatile__ ("nop");
			}
		}

		// the whole line writing must be a total of less than (64 - (18 * 3)) = 10usec =
		// a delay in line 1 = 1/16000000 =0.0000000625 assuming nop = 1 clock cycle

		//#define  LUM_ON DDRB = PORTB = 0b11000;
		// cycles i.e. LUM_ON and LUM_OFF must take exactly same time
#define PIXEL_OFF() \
		asm volatile ( \
		 "ldi r16, 0\n" \
		 "out %[port1], r16\n" \
		 "nop\n"    \
		 "nop\n"    \
		 :  \
		 : [port1] "I" (_SFR_IO_ADDR(DDRB)) \
		 );
#define PIXEL_OFF_NO_NOP() \
		asm volatile ( \
		 "ldi r16, 0\n" \
		 "out %[port1], r16\n" \
		 :  \
		 : [port1] "I" (_SFR_IO_ADDR(DDRB)) \
		 );


#define PIXEL_ON() \
		asm volatile ( \
		 "ldi r16, 0b11000\n" \
		 "out %[port1], r16\n" \
		 "out %[port2], r16\n"    \
		 :  \
		 : [port1] "I" (_SFR_IO_ADDR(PORTB)), [port2] "I" (_SFR_IO_ADDR(DDRB)) \
		 );


		if (drawPixelsOnLine)
		{
			// read out each line from memory and display
			for (i = 0; i < 20; i++)
			{
				__asm__ __volatile__ ("nop");
			}

		    register uint8_t *loopPtrMax = &screenMemory[yCounter][XSIZE - 1];
		    register uint8_t *OneLine = &screenMemory[yCounter][0];
		    register uint8_t theBits = *OneLine;

			do {
				if (theBits & 0b10000000) PIXEL_ON() else PIXEL_OFF_NO_NOP()
				if (theBits & 0b01000000) PIXEL_ON() else PIXEL_OFF_NO_NOP()
				if (theBits & 0b00100000) PIXEL_ON() else PIXEL_OFF_NO_NOP()
				if (theBits & 0b00010000) PIXEL_ON() else PIXEL_OFF_NO_NOP()
				if (theBits & 0b00001000) PIXEL_ON() else PIXEL_OFF_NO_NOP()
				if (theBits & 0b00000100) PIXEL_ON() else PIXEL_OFF_NO_NOP()
				if (theBits & 0b00000010) PIXEL_ON() else PIXEL_OFF_NO_NOP()
				if (theBits & 0b00000001) PIXEL_ON() else PIXEL_OFF_NO_NOP()
				PIXEL_OFF_NO_NOP()
	 			OneLine++;
				theBits = *OneLine;
			}while (OneLine < loopPtrMax);
			PIXEL_OFF_NO_NOP()

			yCounter++;
		}



		lineCounter++;

		if (drawPixelsOnLine)
		{
			if (printScreen == 2) // print hello, world
			{

				uint8_t lineTemp = 0;
				putCharXY(0,lineTemp,convertToMyCharSet('S'));
				lineTemp+=LINE_INC;
				putCharXY(0,lineTemp,convertToMyCharSet('P'));
				lineTemp+=LINE_INC;
				putCharXY(0,lineTemp,convertToMyCharSet('A'));
				lineTemp+=LINE_INC;
				putCharXY(0,lineTemp,convertToMyCharSet('C'));
				lineTemp+=LINE_INC;
				putCharXY(0,lineTemp,convertToMyCharSet('E'));
				lineTemp+=LINE_INC;
				putCharXY(0,lineTemp,convertToMyCharSet('T'));
				lineTemp+=LINE_INC;
				putCharXY(0,lineTemp,convertToMyCharSet('I'));
				lineTemp+=LINE_INC;
				putCharXY(0,lineTemp,convertToMyCharSet('M'));
				lineTemp+=LINE_INC;
				putCharXY(0,lineTemp,convertToMyCharSet('E'));
				lineTemp+=LINE_INC;
				lineTemp+=LINE_INC;
				putCharXY(0,lineTemp,convertToMyCharSet('I'));
				lineTemp+=LINE_INC;
				putCharXY(0,lineTemp,convertToMyCharSet('N'));
				lineTemp+=LINE_INC;
				putCharXY(0,lineTemp,convertToMyCharSet('V'));
				lineTemp+=LINE_INC;
				putCharXY(0,lineTemp,convertToMyCharSet('A'));
				lineTemp+=LINE_INC;
				putCharXY(0,lineTemp,convertToMyCharSet('D'));
				lineTemp+=LINE_INC;
				putCharXY(0,lineTemp,convertToMyCharSet('E'));
				lineTemp+=LINE_INC;
				putCharXY(0,lineTemp,convertToMyCharSet('R'));
				lineTemp+=LINE_INC;
				putCharXY(0,lineTemp,convertToMyCharSet('S'));
				lineTemp+=LINE_INC;
				putCharXY(0,lineTemp,convertToMyCharSet('!'));
				printScreen = 128; // only do once
			}
		}

		switch (lineCounter)
		{
			case 1:
				vSync = 0;
				break;
			case FIRST_LINE_DRAWN+1: drawPixelsOnLine = 1; break;
			//case (MAX_LINE_BEFORE_BLANK - 80) :
			//	drawPixelsOnLine = 1; break;
			//case (MAX_LINE_BEFORE_BLANK - 72) :
			//	drawPixelsOnLine = 0; break;
			case LAST_LINE_DRAWN:
				drawPixelsOnLine = 0; yCounter = 0; PIXEL_OFF(); 	break;
			case (MAX_LINE_BEFORE_BLANK-40):
				drawPixelsOnLine = 0; break;
			case (MAX_LINE_BEFORE_BLANK-6):
				vSync = 1; drawPixelsOnLine = 0; break;
			case MAX_LINE_BEFORE_BLANK:
				lineCounter = 0; vSync = 0;

				if (alienToggleCountDown-- == 0)
				{
					static uint8_t firstTime = 0;
					if (firstTime == 0)
					{
						firstTime = 1;
						clearScreen();
					}
					animateAliens();
					alienToggleCountDown = TOGGLE_RATE;
				}
				break;

		}


		//if (scrollDown == MAX_LINE_BEFORE_BLANK - 80)
		//{

		//}
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
