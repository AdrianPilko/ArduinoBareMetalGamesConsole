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
#define BASE_ALIEN_Y_1 (FIRST_LINE_DRAWN + 8)
#define BASE_ALIEN_Y_2 (FIRST_LINE_DRAWN + 24)
#define BASE_ALIEN_Y_3 (FIRST_LINE_DRAWN + 40)
#define BASE_ALIEN_Y_4 (FIRST_LINE_DRAWN + 56)
#define BASE_ALIEN_Y_5 (FIRST_LINE_DRAWN + 72)

#define HSYNC_BACKPORCH 18
#define HSYNC_FRONT_PORCH_2 15
#define INITIAL_ALIEN_MOVE_RATE 10u

#define WIDTH_ALL_ALIENS 30
#define MAX_X_PLAYER 80
#define MIN_X_PLAYER 10
#define MAX_X_ALIEN (MAX_X_PLAYER - WIDTH_ALL_ALIENS)
#define MIN_X_ALIEN MIN_X_PLAYER

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

uint8_t alienDirection = 1;
uint16_t lineCounter = 0;
uint16_t alienYBasePos = 0;
uint8_t vSync = 0;

typedef struct alienStruct
{
	uint8_t alien1 : 1;
	uint8_t alien2 : 1;
	uint8_t alien3 : 1;
	uint8_t alien4 : 1;
	uint8_t alien5 : 1;
} alienBitPack_t;

alienBitPack_t aliensBitPackStatus;

int main()
{

	int8_t alienToggleTrigger = 0;
	uint8_t alienLineCount = 0;
	uint8_t lineValidForFire = 0;
	uint8_t alienToggle = 0;
	uint16_t firePressed = 0;

	typedef enum {drawNothing=0, drawAlien, drawBarrier, drawPlayer, gameWon, gameLost} drawType_t;
	drawType_t drawType = drawNothing;
	uint8_t playerXPos = MIN_X_PLAYER+30;  // has to be non zero and less that 30
	int8_t alienXStartPos = MIN_X_ALIEN;
	uint8_t alienMoveThisTime = 0;
	uint8_t alienMoveRate = INITIAL_ALIEN_MOVE_RATE;

	aliensBitPackStatus.alien1 = 1;
	aliensBitPackStatus.alien2 = 1;
	aliensBitPackStatus.alien3 = 1;
	aliensBitPackStatus.alien4 = 1;
	aliensBitPackStatus.alien5 = 1;

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
		} else // hsync
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

		switch (drawType)
		{
			case gameWon:  PIXEL_ON(); // do this for now, later add proper a wining screen
				break;
			case gameLost:
				PIXEL_OFF();  // hold it black screen
				break;
			case drawAlien:

				// read out each line from memory and display
				for (int i = 0; i < MIN_DELAY + alienXStartPos; i++)
				{
					NOP_FOR_TIMING
				}

				{
					if (alienToggle == 0)
					{
						if (aliensBitPackStatus.alien1)
							alienDraw_1(alienLineCount);
						else
							alienDraw_blank(alienLineCount);

						if (aliensBitPackStatus.alien2)
							alienDraw_1(alienLineCount);
						else
							alienDraw_blank(alienLineCount);

						if (aliensBitPackStatus.alien3)
							alienDraw_1(alienLineCount);
						else
							alienDraw_blank(alienLineCount);
						if (aliensBitPackStatus.alien4)
							alienDraw_1(alienLineCount);
						else
							alienDraw_blank(alienLineCount);
						if (aliensBitPackStatus.alien5)
							alienDraw_1(alienLineCount);
						else
							alienDraw_blank(alienLineCount);
					}
					else
					{
						if (aliensBitPackStatus.alien1)
							alienDraw_2(alienLineCount);
						else
							alienDraw_blank(alienLineCount);
						if (aliensBitPackStatus.alien2)
							alienDraw_2(alienLineCount);
						else
							alienDraw_blank(alienLineCount);
						if (aliensBitPackStatus.alien3)
							alienDraw_2(alienLineCount);
						else
							alienDraw_blank(alienLineCount);
						if (aliensBitPackStatus.alien4)
							alienDraw_2(alienLineCount);
						else
							alienDraw_blank(alienLineCount);
						if (aliensBitPackStatus.alien5)
							alienDraw_2(alienLineCount);
						else
							alienDraw_blank(alienLineCount);
					}
					PIXEL_OFF_NO_NOP()
					alienLineCount++;
				}
				if (alienLineCount > 7) {
					alienLineCount = 0;
					drawType = drawNothing;
				}
				break;

			case drawBarrier:

				for (int i = 0; i < MIN_DELAY + 25; i++) {
					NOP_FOR_TIMING
				}
				PIXEL_ON();
				for (int i = 0; i < BARRIER_WIDTH; i++) {
					NOP_FOR_TIMING
				}
				PIXEL_OFF_NO_NOP();
				for (int i = 0; i < BARRIER_GAP_WIDTH; i++) {
					NOP_FOR_TIMING
				}
				PIXEL_ON();
				for (int i = 0; i < BARRIER_WIDTH; i++) {
					NOP_FOR_TIMING
				}
				PIXEL_OFF_NO_NOP();
				for (int i = 0; i < BARRIER_GAP_WIDTH; i++) {
					NOP_FOR_TIMING
				}
				PIXEL_ON();
				for (int i = 0; i < BARRIER_WIDTH; i++) {
					NOP_FOR_TIMING
				}
				PIXEL_OFF_NO_NOP();
				break;
			case drawPlayer:

				for (int i = 0; i < MIN_DELAY + playerXPos; i++) {
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
				NOP_FOR_TIMING
				PIXEL_OFF_NO_NOP();
			break;
			default: PIXEL_OFF(); break;
		}

		if ((firePressed == lineCounter+1) || (firePressed == lineCounter))
		{
			if (lineValidForFire == 1)
			{
				for (int i = 0; i < MIN_DELAY + playerXPos; i++)
				{
					NOP_FOR_TIMING
				}
				PIXEL_ON();
				NOP_FOR_TIMING
				NOP_FOR_TIMING
				NOP_FOR_TIMING
				PIXEL_OFF();
			}
		}

		lineCounter++;
		if ((BASE_ALIEN_Y_5+alienYBasePos > MAX_LINE_BEFORE_BLANK - 64) && (drawType != gameWon))
		{
			drawType = gameLost;
		}
		else if (drawType == gameWon)
		{
			// do nothing
		}
		else
		{
			if (lineCounter == BASE_ALIEN_Y_1+alienYBasePos) { drawType = drawAlien; lineValidForFire = 1;}
			if (lineCounter == BASE_ALIEN_Y_2+alienYBasePos) drawType = drawAlien;
			if (lineCounter == BASE_ALIEN_Y_3+alienYBasePos) drawType = drawAlien;
			if (lineCounter == BASE_ALIEN_Y_4+alienYBasePos) drawType = drawAlien;
			if (lineCounter == BASE_ALIEN_Y_5+alienYBasePos) drawType = drawAlien;
		}

		switch (lineCounter) {
		case 1:
			vSync = 0;

			if ((BASE_ALIEN_Y_5+alienYBasePos > MAX_LINE_BEFORE_BLANK - 64) && (drawType != gameWon))
			{
				drawType = gameLost;
			}
			else if (drawType == gameWon)
			{
				// do nothing
			}
			else
			{
				// Check if input control pins
				if (PIND & (1 << PD2)) {
					playerXPos = playerXPos - 1;
				}
				if (PIND & (1 << PD3)) {
					playerXPos = playerXPos + 1;
				}
				if (PIND & (1 << PD4))
				{
					if (firePressed > 0)
					{
						firePressed -= 10;
					}
				}
				else
				{
					firePressed = 250;  // this will clear on its own
				}

				if (firePressed > 0)
				{
					if (playerXPos == alienXStartPos)
					{
						aliensBitPackStatus.alien1 = 0;
					}
					else if (playerXPos+4 == alienXStartPos)
					{
						aliensBitPackStatus.alien2 = 0;
					}
					else if (playerXPos+8 == alienXStartPos)
					{
						aliensBitPackStatus.alien3 = 0;
					}
					else if (playerXPos+12 == alienXStartPos)
					{
						aliensBitPackStatus.alien4 = 0;
					}
					else if (playerXPos+16 == alienXStartPos)
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
						alienXStartPos++;
					}
					else
					{
						alienXStartPos--;
					}
					alienMoveThisTime = 0;
				}

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
					alienMoveRate = alienMoveRate - 1;
					if (alienMoveRate <= 2)
					{
						alienMoveRate = 2;
					}
				}

				if (alienToggleTrigger++ >= 30)
				{
					alienToggleTrigger = 0;
					alienToggle = 1 - alienToggle;
				}
			}

			break;
			/// moved other cases to if else, due to C not allowing non cost (ie y position in switch case)
		case (MAX_LINE_BEFORE_BLANK - 80):
			if ((drawType == gameWon) || (drawType == gameLost))
			{

			}
			else
			{
				drawType = drawBarrier;
			}

			break;
		case (MAX_LINE_BEFORE_BLANK - 73):
			if ((drawType == gameWon) || (drawType == gameLost))
			{

			}
			else
			{
				drawType = drawNothing;
			}
			break;
		case (MAX_LINE_BEFORE_BLANK - 64):
			if ((drawType == gameWon) || (drawType == gameLost))
			{

			}
			else
			{
				drawType = drawPlayer;
				lineValidForFire = 0;
			}
			break;
		case (MAX_LINE_BEFORE_BLANK - 57):
			if ((drawType == gameWon) || (drawType == gameLost))
			{

			}
			else
			{
				drawType = drawNothing;
			}
			break;
		case (MAX_LINE_BEFORE_BLANK - 40):
			if ((drawType == gameWon) || (drawType == gameLost))
			{

			}
			else
			{
				drawType = drawNothing;
			}
			break;
		case (MAX_LINE_BEFORE_BLANK - 6):
			if ((drawType == gameWon) || (drawType == gameLost))
			{

			}
			else
			{
				drawType = drawNothing;
				PIXEL_OFF_NO_NOP()
			}
			vSync = 1;
			break;
		case MAX_LINE_BEFORE_BLANK:
			lineCounter = 0;
			vSync = 0;
			break;

		}
	}
}
