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

int main()
{
	uint16_t lineCounter = 0;
	uint8_t  vSync = 0;
	uint16_t i = 0;
	uint8_t	 pixels = 1;
	uint16_t drawScreen = 0;
	uint16_t xPos = 0;
	uint16_t barStartPosition = 0;
	uint16_t widthOfBar = 30;
	uint16_t lineStartTriggered = 0;
	uint16_t timeInLine = 0;



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
			for (i = 0; i < 18; i++)// delay same amount to give proper back porch before drawing any pixels on the line
			{
				__asm__ __volatile__ ("nop");
			}

			lineStartTriggered = 1;
		}

		if (lineStartTriggered)
		{
			lineCounter++;
			switch (lineCounter)
			{
				case 1: vSync = 0; pixels = 0; break;
				case 10: drawScreen = 1; break; // only start pixel drawing after these many lines
				case 230: drawScreen = 0; break; // pixels off
				case MAX_LINE_BEFORE_BLANK-6: vSync = 1; drawScreen = 0; break;
				case MAX_LINE_BEFORE_BLANK: lineCounter = 0; vSync = 0; drawScreen = 0;break;
			}
			lineStartTriggered = 0;
			timeInLine = 0;
			xPos = 0;
		}

		xPos++;

		if (drawScreen)
		{
			if ((xPos > barStartPosition) && (xPos < barStartPosition+widthOfBar))
			{
				pixels = 1;
			}
			else
			{
				pixels = 0;
			}
		}
		// the problem is we don't really know where in the line we are, we need a blanking period at end of line as well
		if (pixels)
		{
			// turn white "pixel pin" on
			DDRB |= 1 << LUMINANCE_PIN;
			PORTB |= 1 << LUMINANCE_PIN;
		}
		else
		{
			DDRB &= ~(1 << LUMINANCE_PIN);
			PORTB &= ~(1 << LUMINANCE_PIN);
		}
	}
}
