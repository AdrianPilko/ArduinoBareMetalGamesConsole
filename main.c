// can be ran on arduino board, pin and 12 and 13 go to centre of a 10k pot, pin 12 through a 330 Ohm resistor.
// one side of the pot goes to VCC the other to ground and the centre of the pot also goes to the composite connector
// centre pin. if using atmega328p on bare board (not arduino) then its PB5 and PB4.

// connect pin2 3 and 4 through ground for right, left and fire button

#include <avr/power.h>
#include<avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>
#include "charset.h"
#include "asmUtil.h"

#define COMPOSITE_PIN PB5
#define LUMINANCE_PIN PB4
#define MAX_LINE_BEFORE_BLANK 312
#define FIRST_LINE_DRAWN 42
#define LAST_LINE_DRAWN 262
#define HALF_SCREEN 110
#define PLAYER_WIDTH 15
#define BARRIER_WIDTH 20
#define BARRIER_GAP_WIDTH 20
#define MIN_DELAY 5


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

uint8_t alienX = 0;  // top most alien x pos
uint8_t alienY = 0;
uint8_t keepXCount = 0;
uint8_t alienToggle = 0;
uint8_t alienLineCount = 0;
uint8_t alienVertCount = 0;
uint8_t firePressed = 0;
uint8_t lineValidForFire = 0;

int main()
{
	uint8_t alienToggleCountDown = 3;
	uint8_t drawBarrier = 0;
	uint8_t drawPlayer = 0;
	uint16_t playerXPos = 30;  // has to be non zero and less that 30

	uint16_t alienXStartPos = 5;
	uint16_t alienDirection = 1;
	uint8_t alienMoveThisTime = 0;

	uint16_t lineCounter = 0;
	uint8_t  vSync = 0;
	uint8_t i = 0;
	uint8_t drawAliens = 0;

	clock_prescale_set(clock_div_1);


	DDRB |= 1 << COMPOSITE_PIN;
	DDRB |= 1 << LUMINANCE_PIN;

    DDRD &= ~(1 << PD2);	//Pin 2 input (move right)
    PORTD |= (1 << PD2);    //Pin 2 input

    DDRD &= ~(1 << PD3);	//Pin 3 input (move left)
    PORTD |= (1 << PD3);    //Pin 3 input

    DDRD &= ~(1 << PD4);	//Pin 4 input (fire button)
    PORTD |= (1 << PD4);    //Pin 4 input

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

		if (drawAliens == 1)
		{
			// read out each line from memory and display
			for (i = 0; i < MIN_DELAY+alienXStartPos; i++)
			{
				__asm__ __volatile__ ("nop");
			}

			{
				if (alienToggle == 0)
				{
					alienDraw_1(alienLineCount);
					alienDraw_1(alienLineCount);
					alienDraw_1(alienLineCount);
					alienDraw_1(alienLineCount);
					alienDraw_1(alienLineCount);
					alienDraw_1(alienLineCount);
				}
				else
				{
					alienDraw_2(alienLineCount);
					alienDraw_2(alienLineCount);
					alienDraw_2(alienLineCount);
					alienDraw_2(alienLineCount);
					alienDraw_2(alienLineCount);
					alienDraw_2(alienLineCount);
				}
				PIXEL_OFF_NO_NOP()
				alienLineCount++;
			}
			if (alienLineCount > 7)
			{
				alienLineCount = 0;
			}
			alienVertCount++;
		}
		else if (drawBarrier == 1)
		{
			for (i = 0; i < MIN_DELAY+25; i++)
			{
				__asm__ __volatile__ ("nop");
			}
			PIXEL_ON();
			for (i = 0; i < BARRIER_WIDTH; i++)
			{
				__asm__ __volatile__ ("nop");
			}
			PIXEL_OFF_NO_NOP();
			for (i = 0; i < BARRIER_GAP_WIDTH; i++)
			{
				__asm__ __volatile__ ("nop");
			}
			PIXEL_ON();
			for (i = 0; i < BARRIER_WIDTH; i++)
			{
				__asm__ __volatile__ ("nop");
			}
			PIXEL_OFF_NO_NOP();
			for (i = 0; i < BARRIER_GAP_WIDTH; i++)
			{
				__asm__ __volatile__ ("nop");
			}
			PIXEL_ON();
			for (i = 0; i < BARRIER_WIDTH; i++)
			{
				__asm__ __volatile__ ("nop");
			}
			PIXEL_OFF_NO_NOP();
		}
		else if (drawPlayer)
		{
			for (i = 0; i < MIN_DELAY + playerXPos; i++)
			{
				NOP_FOR_TIMING
			}
			PIXEL_ON();

			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_OFF_NO_NOP();
		}
		else if (firePressed == 1)
		{
			if (lineValidForFire == 1)
			{
				for (i = 0; i < MIN_DELAY + playerXPos+1; i++)
				{
					NOP_FOR_TIMING
				}
				PIXEL_ON();
				PIXEL_OFF_NO_NOP();
			}
		}


		switch (lineCounter++)
		{
			case 1:
				vSync = 0;
				break;
			case FIRST_LINE_DRAWN+20:
				drawAliens = 1;
				alienLineCount = 0;
				alienVertCount = 0;
				lineValidForFire = 1;


		        // Check if input control pins
		        if (PIND & (1 << PD2))
			    {
		        	playerXPos = playerXPos - 1;
		        }
		        if (PIND & (1 << PD3))
		        {
		        	playerXPos = playerXPos + 1;
		        }
		        if (PIND & (1 << PD4))
		        {
		        	firePressed = 0;
		        }
		        else
		        {
		        	firePressed = 1;
		        }

				if (playerXPos >=68)
				{
					playerXPos = 67;
				}
				if (playerXPos <= 0)
				{
					playerXPos = 1;
				}

				if (alienMoveThisTime++ == 10)
				{
					alienXStartPos+=alienDirection;
					alienMoveThisTime = 0;
				}

				if (alienXStartPos >= 35)
				{
					alienDirection = -1;
					alienXStartPos = 34;
				}
				if (alienXStartPos == 0)
				{
					alienDirection = 1;
					alienXStartPos = 1;
				}


				if (alienToggleCountDown-- == 0)
				{
					alienToggleCountDown = TOGGLE_RATE;
					alienToggle = 1 - alienToggle;
				}
				break;
			case (FIRST_LINE_DRAWN+28) :
				drawAliens = 0;
				alienVertCount = 0;
				break;
			case (FIRST_LINE_DRAWN+38) :
				drawAliens = 1;
				alienVertCount = 0;
				break;
			case (FIRST_LINE_DRAWN+46) :
				drawAliens = 0;
				alienVertCount = 0;
				break;
			case (FIRST_LINE_DRAWN+56) :
				drawAliens = 1;
				alienVertCount = 0;
				break;
			case (FIRST_LINE_DRAWN+64) :
				drawAliens = 0;
				alienVertCount = 0;
				break;
			case (FIRST_LINE_DRAWN+74) :
				drawAliens = 1;
				alienVertCount = 0;
				break;
			case (FIRST_LINE_DRAWN+82) :
				drawAliens = 0;
				alienVertCount = 0;
				break;
			case (MAX_LINE_BEFORE_BLANK-100) :
				drawAliens = 0;
				alienVertCount = 0;
				break;
			case (MAX_LINE_BEFORE_BLANK-80) :
		        drawBarrier = 1;
				lineValidForFire = 1;
				break;
			case (MAX_LINE_BEFORE_BLANK-73) :
		        drawBarrier = 0;
				lineValidForFire = 1;
				break;
			case (MAX_LINE_BEFORE_BLANK-64) :
		        drawPlayer = 1;
				lineValidForFire = 0;
				break;
			case (MAX_LINE_BEFORE_BLANK-57) :
				drawPlayer = 0;
				break;
			case (MAX_LINE_BEFORE_BLANK-40):
				break;
			case (MAX_LINE_BEFORE_BLANK-6):
				PIXEL_OFF_NO_NOP();
				vSync = 1; break;
			case MAX_LINE_BEFORE_BLANK:
				lineCounter = 0; vSync = 0;
				break;

		}
	}
}
