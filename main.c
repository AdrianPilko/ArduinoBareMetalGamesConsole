// can be ran on arduino board, pin and 12 and 13 go to centre of a 10k pot, pin 12 through a 330 Ohm resistor.
// one side of the pot goes to VCC the other to ground and the centre of the pot also goes to the composite connector
// centre pin. if using atmega328p on bare board (not arduino) then its PB5 and PB4.

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
#define TOGGLE_RATE 45

uint8_t alienX = 0;  // top most alien x pos
uint8_t alienY = 0;
uint8_t keepXCount = 0;
uint8_t alienToggle = 0;
uint8_t alienLineCount = 0;
uint8_t alienVertCount = 0;

// theBytes is the definition of the sprite, but we only draw one line at once, hence "line" is
// also passed in
void drawSprite(const uint8_t theBits)
{
	if (theBits & 0b10000000) PIXEL_ON() else PIXEL_OFF()
	if (theBits & 0b01000000) PIXEL_ON() else PIXEL_OFF()
	if (theBits & 0b00100000) PIXEL_ON() else PIXEL_OFF()
	if (theBits & 0b00010000) PIXEL_ON() else PIXEL_OFF()
	if (theBits & 0b00001000) PIXEL_ON() else PIXEL_OFF()
	if (theBits & 0b00000100) PIXEL_ON() else PIXEL_OFF()
	if (theBits & 0b00000010) PIXEL_ON() else PIXEL_OFF()
	if (theBits & 0b00000001) PIXEL_ON() else PIXEL_OFF()
}


int main()
{
	uint8_t alienToggleCountDown = 3;
	uint8_t drawBarrier = 0;
	uint8_t drawPlayer = 0;
	uint16_t playerXPos = 30;  // has to be non zero and less that 30
	uint16_t playerDirection = 1;

	uint8_t alienInLineOn = 0;
	uint16_t alienXStartPos = 20;
	uint16_t alienDirection = 1;
	uint8_t alienMoveThisTime = 0;

	uint16_t lineCounter = 0;
	uint8_t  vSync = 0;
	uint8_t i = 0;
	uint8_t drawAliens = 0;

	clock_prescale_set(clock_div_1);


	DDRB |= 1 << COMPOSITE_PIN;
	DDRB |= 1 << LUMINANCE_PIN;

    DDRD &= ~(1 << PD2);	//Pin 2 input
    PORTD |= (1 << PD2);    //Pin 2 input

    DDRD &= ~(1 << PD3);	//Pin 2 input
    PORTD |= (1 << PD3);    //Pin 2 input

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

		if (drawAliens)
		{
			// read out each line from memory and display
			for (i = 0; i < 15+alienXStartPos; i++)
			{
				__asm__ __volatile__ ("nop");
			}

			if (alienInLineOn == 1)
			{
				if (alienToggle == 0)
				{
					alienDraw_2(alienLineCount);
					alienDraw_2(alienLineCount);
					alienDraw_2(alienLineCount);
					alienDraw_2(alienLineCount);
				}
				else
				{
					alienDraw_2(alienLineCount);
					alienDraw_2(alienLineCount);
					alienDraw_2(alienLineCount);
					alienDraw_2(alienLineCount);
				}

				PIXEL_OFF_NO_NOP()
				for (i = 0; i < 20; i++)
				{
					__asm__ __volatile__ ("nop");
				}
				alienLineCount++;
				if (alienLineCount > 7)
				{
					alienLineCount = 0;
					alienInLineOn = 0;
				}
				alienVertCount++;
			}
		}

		if (drawBarrier == 1)
		{
			for (i = 0; i < 50; i++)
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
		if (drawPlayer)
		{
			for (i = 0; i < 15 + playerXPos; i++)
			{
				asm volatile ("nop");
			}
			PIXEL_ON();

			for (i = 0; i < PLAYER_WIDTH ; i++)
			{
				asm volatile ("nop");
			}
			PIXEL_OFF_NO_NOP();
		}


		switch (lineCounter++)
		{
			case 1:
				vSync = 0;
				break;
			case FIRST_LINE_DRAWN+20:
				drawAliens = 1;
				alienInLineOn = 1;
				alienLineCount = 0;
				alienVertCount = 0;

				playerXPos += playerDirection;

				if (playerXPos >= 64)
				{
					playerDirection = -1;
					playerXPos = 63;
				}
				if (playerXPos <= 0)
				{
					playerDirection = 1;
					playerXPos = 1;
				}

				if (alienMoveThisTime++ == 6)
				{
					//alienXStartPos+=alienDirection;
					alienMoveThisTime = 0;
				}

				if (alienXStartPos >= 32)
				{
					alienDirection = -1;
					alienXStartPos = 63;
				}
				if (alienXStartPos <= 0)
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
			case (MAX_LINE_BEFORE_BLANK-100) :
				drawAliens = 0;
				alienVertCount = 0;
				break;
			case (MAX_LINE_BEFORE_BLANK-80) :
		        drawBarrier = 1;
				break;
			case (MAX_LINE_BEFORE_BLANK-73) :
		        drawBarrier = 0;
				break;
			case (MAX_LINE_BEFORE_BLANK-64) :
		        drawPlayer = 1;
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
