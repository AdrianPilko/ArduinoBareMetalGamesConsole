// This is meant to be run on an Arduino UNO board
// pin and 12 and 13 go to centre of a 10k pot, pin 12 through a 330 Ohm resistor.
// one side of the pot goes to VCC the other to ground and the centre of the pot also goes to the composite connector
// centre pin. if using atmega328p on bare board (not arduino) then its PB5 and PB4.

// connect pin2 3 and 4 through ground for right, left and fire button

/// The code is written deliberately to ensure correct timing, so instead of for or while, do while loops
/// in several places, especially for delays, just directly unrolled loops of asm nop are used (nop = no operation)
/// On arduino a NOP is 1 clock cycles, so gives a timing of
/// 1 / 16MHz = 1 / 16000000 = 0.0000000625seconds = 0.0625microseconds
/// each line of the screen must take 64microseconds
// also switch staements are used to speed up the logic where possible, and more memory is used for
// structures to save execution time - all this to say the code may look messy but try doing it other ways
// without using just assembly code (might be the final option!)

#include <avr/power.h>
#include<avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>
#include "charset.h"
#include "asmUtil.h"

#define COMPOSITE_PIN PB5
#define LUMINANCE_PIN PB4
#define MAX_LINE_BEFORE_BLANK 300
#define FIRST_LINE_DRAWN 50
#define LAST_LINE_DRAWN 262
#define HALF_SCREEN 110
#define PLAYER_WIDTH 16
#define BARRIER_WIDTH 20
#define MIN_DELAY 5
#define BASE_ALIEN_OFFSET FIRST_LINE_DRAWN+4
#define BASE_ALIEN_Y_1 (70)
#define BASE_ALIEN_Y_2 (80)
#define BASE_ALIEN_Y_3 (90)
#define BASE_ALIEN_Y_4 (100)
#define BASE_ALIEN_Y_5 (110)
#define BASE_ALIEN_Y_6 (120)

#define HSYNC_BACKPORCH 18
#define HSYNC_FRONT_PORCH_2 15
#define INITIAL_ALIEN_MOVE_RATE 10

#define WIDTH_ALL_ALIENS 30
#define MAX_X_PLAYER 125
#define MIN_X_PLAYER 1
#define MAX_X_ALIEN 77
#define MIN_X_ALIEN 1

// from experimenting a line started at linCounter = 25 appears right at top of screen one at 285 to 286 is bottom
// a delay in clock cycles of 14 to plus a delay of 148 clock cycles gives a usable horizontal line length

// XSIZE is in bytes, and currently due to lack of time (spare clock cycles) only able todo 8 *8(bits) pixels = 64
// will need some serious optimisation later!
#define XSIZE 8
#define YSIZE (HALF_SCREEN*2)

#define LINE_INC 8
#define LINE_INC_ALIENS 12

static uint8_t alienLineCount = 0;

int main()
{
	int alienDirection = 1;
	int vSync = 0;
	int lineCounter = 0;
	int lineValidForFire = 0;

	typedef struct alienStruct
	{
		uint32_t alien_row1_col1 : 1;
		uint32_t alien_row1_col2 : 1;
		uint32_t alien_row1_col3 : 1;
		uint32_t alien_row1_col4 : 1;
		uint32_t alien_row1_col5 : 1;
		uint32_t alien_row1_col6 : 1;

		uint32_t alien_row2_col1 : 1;
		uint32_t alien_row2_col2 : 1;
		uint32_t alien_row2_col3 : 1;
		uint32_t alien_row2_col4 : 1;
		uint32_t alien_row2_col5 : 1;
		uint32_t alien_row2_col6 : 1;

		uint32_t alien_row3_col1 : 1;
		uint32_t alien_row3_col2 : 1;
		uint32_t alien_row3_col3 : 1;
		uint32_t alien_row3_col4 : 1;
		uint32_t alien_row3_col5 : 1;
		uint32_t alien_row3_col6 : 1;

		uint32_t alien_row4_col1 : 1;
		uint32_t alien_row4_col2 : 1;
		uint32_t alien_row4_col3 : 1;
		uint32_t alien_row4_col4 : 1;
		uint32_t alien_row4_col5 : 1;
		uint32_t alien_row4_col6 : 1;

		uint32_t alien_row5_col1 : 1;
		uint32_t alien_row5_col2 : 1;
		uint32_t alien_row5_col3 : 1;
		uint32_t alien_row5_col4 : 1;
		uint32_t alien_row5_col5 : 1;
		uint32_t alien_row5_col6 : 1;
	} alienBitPack_t;


	alienBitPack_t aliensBitPackStatus;

	int alienToggleTrigger = 0;

	int alienToggle = 0;
	int firePressed = 0;
	int alienYBasePos = 0;

	typedef enum {drawNothing=0,
		drawAlien_row1,
		drawAlien_row2,
		drawAlien_row3,
		drawAlien_row4,
		drawAlien_row5,
		drawAlien_row6,
		gameWon} drawType_t;
	drawType_t drawType = drawNothing;
	int playerXPos = MIN_X_PLAYER+68;  // has to be non zero and less that 30
	int alienXStartPos = MIN_X_ALIEN+10;
	int alienMoveThisTime = 0;
	int alienMoveRate = 1;


	aliensBitPackStatus.alien_row1_col1= 1;
	aliensBitPackStatus.alien_row1_col2= 1;
	aliensBitPackStatus.alien_row1_col3= 1;
	aliensBitPackStatus.alien_row1_col4= 1;
	aliensBitPackStatus.alien_row1_col5= 1;
	aliensBitPackStatus.alien_row1_col6= 1;

	aliensBitPackStatus.alien_row2_col1= 1;
	aliensBitPackStatus.alien_row2_col2= 1;
	aliensBitPackStatus.alien_row2_col3= 1;
	aliensBitPackStatus.alien_row2_col4= 1;
	aliensBitPackStatus.alien_row2_col5= 1;
	aliensBitPackStatus.alien_row2_col6= 1;

	aliensBitPackStatus.alien_row3_col1= 1;
	aliensBitPackStatus.alien_row3_col2= 1;
	aliensBitPackStatus.alien_row3_col3= 1;
	aliensBitPackStatus.alien_row3_col4= 1;
	aliensBitPackStatus.alien_row3_col5= 1;
	aliensBitPackStatus.alien_row3_col6= 1;

	aliensBitPackStatus.alien_row4_col1= 1;
	aliensBitPackStatus.alien_row4_col2= 1;
	aliensBitPackStatus.alien_row4_col3= 1;
	aliensBitPackStatus.alien_row4_col4= 1;
	aliensBitPackStatus.alien_row4_col5= 1;
	aliensBitPackStatus.alien_row4_col6= 1;

	aliensBitPackStatus.alien_row5_col1= 1;
	aliensBitPackStatus.alien_row5_col2= 1;
	aliensBitPackStatus.alien_row5_col3= 1;
	aliensBitPackStatus.alien_row5_col4= 1;
	aliensBitPackStatus.alien_row5_col5= 1;
	aliensBitPackStatus.alien_row5_col6= 1;

	clock_prescale_set(clock_div_1);

	DDRB |= 1 << COMPOSITE_PIN;
	DDRB |= 1 << LUMINANCE_PIN;

	DDRD &= ~(1 << PD2);	//Pin 2 input (move right)
	PORTD |= (1 << PD2);    //Pin 2 input

	DDRD &= ~(1 << PD3);	//Pin 3 input (move left)
	PORTD |= (1 << PD3);    //Pin 3 input

	DDRD &= ~(1 << PD4);	//Pin 4 input (fire button)
	PORTD |= (1 << PD4);    //Pin 4 input

	TCCR1B = (1 << CS10); // switch off clock prescaller
	OCR1A = 1025; //timer interrupt cycles which gives rise to 64usec line sync time
	TCNT1 = 0;
	while (1) {
		// wait for hsync timer interrupt to trigger
		while ((TIFR1 & (1 << OCF1A)) == 0) {
			// wait till the timer overflow flag is SET
		}

		// immediately reset the interrupt timer, previous version had this at the end
		// which made the whole thing be delayed when we're trying to maintain the 64us PAL line timing
		TCNT1 = 0;
		TIFR1 |= (1 << OCF1A); //clear timer1 overflow flag

		if (vSync > 0) // invert the line sync pulses when in vsync part of screen
				{
			DDRB = 0;
			for (int i = 0; i < HSYNC_BACKPORCH + 10; i++) {
				NOP_FOR_TIMING
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
			for (int i = 0; i < 1; i++) {
				NOP_FOR_TIMING
			}

			// Hold the output to the composite connector low, the zero volt hsync
			PORTB = 0;
			DDRB |= (1 << COMPOSITE_PIN); // set COMPOSITE_PIN as output
			for (int i = 0; i < HSYNC_BACKPORCH; i++) {
				NOP_FOR_TIMING
			}

			// hold output to composite connector to 300mV
			DDRB &= ~(1 << COMPOSITE_PIN); // set COMPOSITE_PIN as input
			PORTB = 1;
			for (int i = 0; i < HSYNC_BACKPORCH; i++) // delay same amount to give proper back porch before drawing any pixels on the line
					{
				NOP_FOR_TIMING
			}
		}

		// the whole line writing must be a total of less than (64 - (18 * 3)) = 10usec =
		// a delay in line 1 = 1/16000000 =0.0000000625 assuming nop = 1 clock cycle
		//drawType = drawNothing;
		switch (drawType)
		{
			case drawAlien_row1:
				delayLoop(alienXStartPos);
				if (alienToggle == 0)
				{
					if (aliensBitPackStatus.alien_row1_col1)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row1_col2)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row1_col3)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row1_col4)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row1_col5)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row1_col6)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
				}
				else
				{
					if (aliensBitPackStatus.alien_row1_col1)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row1_col2)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row1_col3)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row1_col4)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row1_col5)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row1_col6)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
				}
				PIXEL_OFF_NO_NOP()
				alienLineCount++;

				if (alienLineCount > 7) {
					alienLineCount = 0;
					drawType = drawNothing;
					PIXEL_OFF_NO_NOP();
				}
				break;
			case drawAlien_row2:
				delayLoop(alienXStartPos);
				if (alienToggle == 0)
				{
					if (aliensBitPackStatus.alien_row2_col1)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row2_col2)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row2_col3)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row2_col4)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row2_col5)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row2_col6)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
				}
				else
				{
					if (aliensBitPackStatus.alien_row2_col1)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row2_col2)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row2_col3)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row2_col4)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row2_col5)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row2_col6)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
				}
				PIXEL_OFF_NO_NOP()
				alienLineCount++;

				if (alienLineCount > 7) {
					alienLineCount = 0;
					drawType = drawNothing;
					PIXEL_OFF_NO_NOP();
				}
				break;
			case drawAlien_row3:
				delayLoop(alienXStartPos);
				if (alienToggle == 0)
				{
					if (aliensBitPackStatus.alien_row3_col1)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row3_col2)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row3_col3)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row3_col4)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row3_col5)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row3_col6)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
				}
				else
				{
					if (aliensBitPackStatus.alien_row3_col1)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row3_col2)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row3_col3)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row3_col4)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row3_col5)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row3_col6)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
				}
				PIXEL_OFF_NO_NOP()
				alienLineCount++;

				if (alienLineCount > 7) {
					alienLineCount = 0;
					drawType = drawNothing;
					PIXEL_OFF_NO_NOP();
				}
				break;
			case drawAlien_row4:
				delayLoop(alienXStartPos);
				if (alienToggle == 0)
				{
					if (aliensBitPackStatus.alien_row4_col1)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row4_col2)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row4_col3)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row4_col4)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row4_col5)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row4_col6)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
				}
				else
				{
					if (aliensBitPackStatus.alien_row4_col1)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row4_col2)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row4_col3)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row4_col4)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row4_col5)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row4_col6)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
				}
				PIXEL_OFF_NO_NOP()
				alienLineCount++;

				if (alienLineCount > 7) {
					alienLineCount = 0;
					drawType = drawNothing;
					PIXEL_OFF_NO_NOP();
				}
				break;
			case drawAlien_row5:
				delayLoop(alienXStartPos);
				if (alienToggle == 0)
				{
					if (aliensBitPackStatus.alien_row5_col1)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row5_col2)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row5_col3)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row5_col4)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row5_col5)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row5_col6)
					{
						 alienDraw_1(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
				}
				else
				{
					if (aliensBitPackStatus.alien_row5_col1)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row5_col2)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row5_col3)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row5_col4)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row5_col5)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
					if (aliensBitPackStatus.alien_row5_col6)
					{
						 alienDraw_2(alienLineCount);
					}
					else alienDraw_blank(alienLineCount);
				}
				PIXEL_OFF_NO_NOP()
				alienLineCount++;

				if (alienLineCount > 7) {
					alienLineCount = 0;
					drawType = drawNothing;
					PIXEL_OFF_NO_NOP();
				}
				break;
		}
		//if ((BASE_ALIEN_Y_5+alienYBasePos > MAX_LINE_BEFORE_BLANK - 64) && (drawType != gameWon))
		//{
		//	drawType = gameLost;
		//}
		//else if (drawType == gameWon)
		//if (drawType == gameWon)
		//{
			// do nothing
		//}
		//else
		// row_1 is top most
		if (lineCounter == BASE_ALIEN_Y_1+alienYBasePos) drawType = drawAlien_row1;
		if (lineCounter == BASE_ALIEN_Y_2+alienYBasePos) drawType = drawAlien_row2;
		if (lineCounter == BASE_ALIEN_Y_3+alienYBasePos) drawType = drawAlien_row3;
		if (lineCounter == BASE_ALIEN_Y_4+alienYBasePos) drawType = drawAlien_row4;
		if (lineCounter == BASE_ALIEN_Y_5+alienYBasePos) drawType = drawAlien_row5;

		lineCounter++;

		switch (lineCounter)
		{
		case 1:
			vSync = 0;
			break;
		case FIRST_LINE_DRAWN:
			PIXEL_ON()
			delayLoop(125);
			PIXEL_OFF()
			break;
		case FIRST_LINE_DRAWN+1:
			//if ((BASE_ALIEN_Y_5+alienYBasePos > MAX_LINE_BEFORE_BLANK - 64) && (drawType != gameWon))
			//{
			//	drawType = gameLost;
			//}
			//else if (drawType == gameWon)
			//{
				// do nothing
			//}
			//else
			{
				// Check if input control pins
				if (PIND & (1 << PD2)) {
					playerXPos = playerXPos - 2;
				}
				if (PIND & (1 << PD3)) {
					playerXPos = playerXPos + 2;
				}
				if (PIND & (1 << PD4))
				{

				}
				else
				{
					firePressed = 10;
				}
#if 0
				if (firePressed > 0)
				{
					if (playerXPos == alienXStartPos)
					{
						aliensBitPackStatus.alien_row1_col1 = 0;
					}
					else if (playerXPos == alienXStartPos+2)
					{
						aliensBitPackStatus.alien2 = 0;
					}
					else if (playerXPos == alienXStartPos+4)
					{
						aliensBitPackStatus.alien3 = 0;
					}
					else if (playerXPos == alienXStartPos+6)
					{
						aliensBitPackStatus.alien4 = 0;
					}
					else if (playerXPos == alienXStartPos+8)
					{
						aliensBitPackStatus.alien5 = 0;
					}
				}
				if ((aliensBitPackStatus.alien1 == 0) &&
					(aliensBitPackStatus.alien2 == 0) &&
					(aliensBitPackStatus.alien3 == 0) &&
					(aliensBitPackStatus.alien4 == 0) &&
					(aliensBitPackStatus.alien5 == 0))
				{
					drawType = gameWon;
				}
#endif
				if (playerXPos > MAX_X_PLAYER) {
					playerXPos = MAX_X_PLAYER-1;
				}
				if (playerXPos < MIN_X_PLAYER) {
					playerXPos = MIN_X_PLAYER+1;
				}
				/// all the alien init gumbins!

				alienMoveThisTime = alienMoveThisTime + 1;

				if (alienMoveThisTime >= alienMoveRate)
				{
					if (alienDirection == 1)
					{
						alienXStartPos+=alienMoveRate;
					}
					else
					{
						alienXStartPos-=alienMoveRate;
					}
					alienMoveThisTime = 0;
				}

				alienXStartPos = playerXPos;

				if (alienXStartPos > MAX_X_ALIEN) // remember that this is only the X position of left most alien
				{
					alienDirection = 0;
					alienXStartPos = MAX_X_ALIEN;
					// move aliens down by one line (getting closer to you!)
					alienYBasePos += 4;
				}
				if (alienXStartPos < MIN_X_ALIEN)
				{
					alienDirection = 1;
					alienXStartPos = MIN_X_ALIEN;
					// move aliens down by one line (getting closer to you!)
					alienYBasePos += 4;

					// speed up the aliens
					alienMoveRate = alienMoveRate + 1;
					if (alienMoveRate > 16)
					{
						alienMoveRate = 16;
					}
				}

				if (alienToggleTrigger++ >= 20 - alienMoveRate)
				{
					alienToggleTrigger = 0;
					alienToggle = 1 - alienToggle;
				}
			}
			break;

		case (MAX_LINE_BEFORE_BLANK - 80):
			//TEN_NOP_FOR_TIMING;
			TEN_NOP_FOR_TIMING;
			TEN_NOP_FOR_TIMING;
			TEN_NOP_FOR_TIMING;
			FIVE_NOP_FOR_TIMING;
			PIXEL_ON();
			delayLoop(BARRIER_WIDTH);
			PIXEL_OFF_NO_NOP();
			delayLoop(BARRIER_WIDTH);
			PIXEL_ON();
			delayLoop(BARRIER_WIDTH);
			PIXEL_OFF_NO_NOP();
			delayLoop(BARRIER_WIDTH);
			PIXEL_ON();
			delayLoop(BARRIER_WIDTH);
			PIXEL_OFF_NO_NOP();
			break;
		case (MAX_LINE_BEFORE_BLANK - 79):
			break;
		case (MAX_LINE_BEFORE_BLANK - 78):
			break;
		case (MAX_LINE_BEFORE_BLANK - 77):
			TEN_NOP_FOR_TIMING;
			TEN_NOP_FOR_TIMING;
			TEN_NOP_FOR_TIMING;
			TEN_NOP_FOR_TIMING;
			NOP_FOR_TIMING;
			NOP_FOR_TIMING;
		    NOP_FOR_TIMING;
			PIXEL_ON();
			delayLoop(BARRIER_WIDTH);
			PIXEL_OFF_NO_NOP();
			delayLoop(BARRIER_WIDTH);
			PIXEL_ON();
			delayLoop(BARRIER_WIDTH);
			PIXEL_OFF_NO_NOP();
			delayLoop(BARRIER_WIDTH);
			PIXEL_ON();
			delayLoop(BARRIER_WIDTH);
			PIXEL_OFF_NO_NOP();
			break;
		case (MAX_LINE_BEFORE_BLANK - 66):    /// code to fire
			if (firePressed > 0)
			{
				delayLoop(playerXPos);
				PIXEL_ON();
				NOP_FOR_TIMING;
				PIXEL_OFF_NO_NOP();
				firePressed--;
			}
			break;
		case (MAX_LINE_BEFORE_BLANK - 65):    /// code to fire
			if (firePressed > 0)
			{
				TEN_NOP_FOR_TIMING;
				FIVE_NOP_FOR_TIMING;
				NOP_FOR_TIMING;
				NOP_FOR_TIMING;
				NOP_FOR_TIMING;
				NOP_FOR_TIMING;
				delayLoop(playerXPos);
				PIXEL_ON();
				NOP_FOR_TIMING;
				PIXEL_OFF_NO_NOP();
				firePressed--;
			}
			break;
		case (MAX_LINE_BEFORE_BLANK - 64):    /// code to draw player
			delayLoop(playerXPos);
			PIXEL_ON();
			delayLoop(PLAYER_WIDTH);
			PIXEL_OFF_NO_NOP();
			lineValidForFire = 0;
			break;
		case (MAX_LINE_BEFORE_BLANK - 63):
			break;
		case (MAX_LINE_BEFORE_BLANK - 62):
			delayLoop(playerXPos);
			PIXEL_ON();
			delayLoop(PLAYER_WIDTH);
			PIXEL_OFF_NO_NOP();
			lineValidForFire = 0;
			break;
		case (MAX_LINE_BEFORE_BLANK - 61):
			break;
		case (MAX_LINE_BEFORE_BLANK - 60):  // last line of the player draw
			delayLoop(playerXPos);
			PIXEL_ON();
			delayLoop(PLAYER_WIDTH);
			TEN_NOP_FOR_TIMING;
			FIVE_NOP_FOR_TIMING;
			NOP_FOR_TIMING;
			NOP_FOR_TIMING;
			PIXEL_OFF_NO_NOP();
			break;
		case (MAX_LINE_BEFORE_BLANK - 50):
			break;
		case (MAX_LINE_BEFORE_BLANK - 48):
			PIXEL_ON()
			delayLoop(125);
			PIXEL_OFF()
			break;
		case (MAX_LINE_BEFORE_BLANK - 7):
			vSync = 1;
			break;
		case MAX_LINE_BEFORE_BLANK:
			lineCounter = 0;
			vSync = 0;
			break;
		}
	}
}
