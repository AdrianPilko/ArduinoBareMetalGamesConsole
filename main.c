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

	DDRB |= 1 << COMPOSITE_PIN;         //configure led as input
	DDRB |= 1 << LUMINANCE_PIN;

	TCCR1B = (1<<CS10);// | (1<<CS12); //set the pre-scalar as 1024
	OCR1A = 805; 	   //usec delay
	TCNT1 = 0;
	while(1)
	{
		//If flag is set toggle the led
		while((TIFR1 & (1<<OCF1A)) == 0)
		{
				// wait till the timer overflow flag is SET
		}


		if (vSync) // invert the line sync pulses when in vsync part of screen
		{
			DDRB &= ~(1 << COMPOSITE_PIN);
			for (i = 0; i < 18; i++)
			{
				__asm__ __volatile__ ("nop");
			}
			PORTB = 0;
			DDRB |= (1 << COMPOSITE_PIN);
		}
		else
		{
			PORTB = 0; // set all PORTB to low, as DDRB in next statement is input this causes output to go low
			DDRB |= (1 << COMPOSITE_PIN); // set COMPOSITE_PIN as output
			for (i = 0; i < 18; i++)
			{
				__asm__ __volatile__ ("nop");
			}
			DDRB &= ~(1 << COMPOSITE_PIN); // set COMPOSITE_PIN as input
			PORTB = 1; // set all PORTB to high, as DDRB in input this causes output to go high
		}
		lineCounter++;
		switch (lineCounter)
		{
			case 1: vSync = 0; break;
			case MAX_LINE_BEFORE_BLANK-6: vSync = 1; break;
			case MAX_LINE_BEFORE_BLANK: lineCounter = 0; vSync = 0; break;
		}

		TCNT1 = 0;
		TIFR1 |= (1<<OCF1A) ; //clear timer1 overflow flag
	}
}
