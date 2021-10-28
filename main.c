#include<avr/io.h>
#include <util/delay.h>

#define COMPOSITE_PIN PD5
#define HSYNC_BACKPORCH 5

int main()
{
	DDRB |= 1 << 5;         //configure led as input
	TCCR1B = (1<<CS10);// | (1<<CS12); //set the pre-scalar as 1024
	OCR1A = 512; 	   //512usec delay
	TCNT1 = 0;
	while(1)
	{
		//If flag is set toggle the led
		while((TIFR1 & (1<<OCF1A)) == 0)
		{
				// wait till the timer overflow flag is SET
		}
		PORTB = 0;
		DDRB |= 1 << 5;         //configure led as input
		_delay_us(HSYNC_BACKPORCH);
		DDRB &= ~(1 << 5);         //configure led as input
		PORTB = 1;

		TCNT1 = 0;
		TIFR1 |= (1<<OCF1A) ; //clear timer1 overflow flag
	}
}
