#include<avr/io.h>
#include <util/delay.h>

#define COMPOSITE_PIN PB5
#define LUMINANCE_PIN PB4
#define MAX_LINE_BEFORE_BLANK 312

#define HSYNC_BACKPORCH 4

int main()
{
	uint16_t lineCounter = 0;
	uint8_t vSync = 0;
	uint8_t i = 0;
	uint8_t	pixels = 1;

	DDRB |= 1 << COMPOSITE_PIN;         //configure led as input
	DDRB |= 1 << LUMINANCE_PIN;

	TCCR1B = (1<<CS10);// | (1<<CS12); //set the pre-scalar as 1024
	OCR1A = 1025; 	   //usec delay
	TCNT1 = 0;
	while(1)
	{
		//If flag is set toggle the led
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
			for (i = 0; i < 18; i++)
			{
				__asm__ __volatile__ ("nop");
			}
			PORTB &= 0 << COMPOSITE_PIN; // set all PORTB COMPOSITE_PIN to low, as DDRB in next statement is input this causes output to go low
			DDRB |= (1 << COMPOSITE_PIN);
		}
		else
		{
			// before the end of line (which in here is also effectively just the start of the line!) we need to hold at 300mV
			// and ensure no pixels
			DDRB &= ~(1 << COMPOSITE_PIN); // set COMPOSITE_PIN as input
			PORTB |= 1 << COMPOSITE_PIN; // set PORTB COMPOSITE_PIN to high, as DDRB in input this causes output to go high
			DDRB &= ~(1 << LUMINANCE_PIN);
			PORTB &= ~(1 << LUMINANCE_PIN);

			for (i = 0; i < 18; i++)// delay same amount to give proper back porch before drawing any pixels on the line
			{
				__asm__ __volatile__ ("nop");
			}

			// Hold the output to the composite connector low, the zero volt hsync
			PORTB &= 0 << COMPOSITE_PIN; // set all PORTB COMPOSITE_PIN to low, as DDRB in next statement is input this causes output to go low
			DDRB |= (1 << COMPOSITE_PIN); // set COMPOSITE_PIN as output
			for (i = 0; i < 18; i++)
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
		}

		// the problem is we don't really know where in the line we are, we need a blanking period at end of line as well
		if (pixels)
		{
			DDRB |= 1 << LUMINANCE_PIN;
			PORTB |= 1 << LUMINANCE_PIN;
		}
		else
		{
			DDRB &= ~(1 << LUMINANCE_PIN);
			PORTB &= ~(1 << LUMINANCE_PIN);
		}

		lineCounter++;
		switch (lineCounter)
		{
			case 1: vSync = 0; pixels = 0; break;
			case 20: pixels = 1; break; // only start pixel drawing after these many lines
			case 50: pixels = 0; break; // pixels off
			case MAX_LINE_BEFORE_BLANK-6: vSync = 1; pixels = 0; break;
			case MAX_LINE_BEFORE_BLANK: lineCounter = 0; vSync = 0; pixels = 0;break;
		}
	}
}
