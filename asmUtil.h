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
#define NOP_FOR_TIMING asm volatile ("nop");

#define FIVE_NOP_FOR_TIMING \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop");

#define TEN_NOP_FOR_TIMING \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop");

#define HUNDRED_NOP_FOR_TIMING \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop"); \
asm volatile ("nop");

void alienDraw_1(const uint8_t alienLineCount)
{
	switch (alienLineCount)
	{
		case 0:
			PIXEL_ON() //0b10000001,
			PIXEL_OFF()
			PIXEL_OFF()
			PIXEL_OFF()
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_ON()
			PIXEL_OFF_NO_NOP()break;
		case 1:
			PIXEL_OFF() //0b01111110,
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_ON()
			NOP_FOR_TIMING
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			break;
		case 2:
			NOP_FOR_TIMING //01011010
			PIXEL_ON()
			NOP_FOR_TIMING
			PIXEL_OFF()
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_OFF()
			PIXEL_ON()
			NOP_FOR_TIMING
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			break;
		case 3:
			NOP_FOR_TIMING//0b01111110,
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_ON()
			NOP_FOR_TIMING
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			break;
		case 4:
			NOP_FOR_TIMING  //0b00111100,
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_ON()
			NOP_FOR_TIMING
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_ON()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			break;
		case 5: // legs
			PIXEL_OFF() //0b00100100,
			PIXEL_OFF_NO_NOP()
			PIXEL_ON()
			PIXEL_OFF()
			PIXEL_OFF()
			PIXEL_ON()
			PIXEL_OFF()
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			break;
		case 6:
			PIXEL_OFF() //	0b01000010,
			PIXEL_ON()
			PIXEL_OFF_NO_NOP()
			PIXEL_OFF()
			PIXEL_OFF()
			PIXEL_OFF_NO_NOP()
			PIXEL_ON()
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			break;
		case 7:
			PIXEL_OFF_NO_NOP()
			PIXEL_ON()  //	0b10000001}
			PIXEL_OFF_NO_NOP()
			PIXEL_OFF()
			PIXEL_OFF_NO_NOP()
			PIXEL_OFF_NO_NOP()
			PIXEL_OFF_NO_NOP()
			PIXEL_OFF_NO_NOP()
			PIXEL_OFF_NO_NOP()
			PIXEL_ON()
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			break;
		default:PIXEL_OFF_NO_NOP() break;
	};
}


void alienDraw_2(const uint8_t alienLineCount)
{
	switch (alienLineCount)
	{
		case 0:   // ears
			PIXEL_OFF_NO_NOP()//0b01000010,
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_ON()
			PIXEL_OFF()
			PIXEL_OFF()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_ON()
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			//NOP_FOR_TIMING
			//NOP_FOR_TIMING
			break;
		case 1:  // top of head
			PIXEL_OFF() //0b01111110,
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_ON()
			NOP_FOR_TIMING
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING

			break;
		case 2: // eyes
			NOP_FOR_TIMING //01011010
			PIXEL_ON()
			NOP_FOR_TIMING
			PIXEL_OFF()
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_OFF()
			PIXEL_ON()
			PIXEL_OFF_NO_NOP()
//			NOP_FOR_TIMING
			break;
		case 3: // mouth
			NOP_FOR_TIMING//0b01111110,
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_ON()
			PIXEL_ON()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_ON()
			PIXEL_ON()
			NOP_FOR_TIMING
			PIXEL_OFF_NO_NOP()
			//NOP_FOR_TIMING
			//NOP_FOR_TIMING
			//NOP_FOR_TIMING
			break;
		case 4: // bottom of body
			NOP_FOR_TIMING  //0b00111100,
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_ON()
			NOP_FOR_TIMING
			PIXEL_ON()
			PIXEL_ON()
			PIXEL_ON()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			break;
		case 5:   // the legs
			PIXEL_OFF() //0b00100100,
			PIXEL_OFF_NO_NOP()
			PIXEL_ON()
			PIXEL_OFF()
			PIXEL_OFF()
			PIXEL_ON()
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			break;
		case 6:
			PIXEL_OFF() //	0b01000010,
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_ON()
			NOP_FOR_TIMING
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_ON()
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			break;
		case 7:
			PIXEL_OFF_NO_NOP()
			PIXEL_OFF()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			PIXEL_ON()
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			PIXEL_ON()
			PIXEL_OFF_NO_NOP()
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			NOP_FOR_TIMING
			break;
		default:PIXEL_OFF_NO_NOP() break;
	};
}

// this is purely to maintain timing of a gap with no alien
void alienDraw_blank(const int alienLineCount)
{
	switch (alienLineCount)
	{
		case 0:
			TEN_NOP_FOR_TIMING
			TEN_NOP_FOR_TIMING
			FIVE_NOP_FOR_TIMING
			break;
		case 1:
			TEN_NOP_FOR_TIMING
			TEN_NOP_FOR_TIMING
			FIVE_NOP_FOR_TIMING
			break;
		case 2:
			TEN_NOP_FOR_TIMING
			TEN_NOP_FOR_TIMING
			FIVE_NOP_FOR_TIMING
			break;
		case 3:
			TEN_NOP_FOR_TIMING
			TEN_NOP_FOR_TIMING
			FIVE_NOP_FOR_TIMING
			break;
		case 4:
			TEN_NOP_FOR_TIMING
			TEN_NOP_FOR_TIMING
			FIVE_NOP_FOR_TIMING
			break;
		case 5: // legs
			TEN_NOP_FOR_TIMING
			TEN_NOP_FOR_TIMING
			FIVE_NOP_FOR_TIMING
			break;
		case 6:
			TEN_NOP_FOR_TIMING
			TEN_NOP_FOR_TIMING
			FIVE_NOP_FOR_TIMING
			break;
		case 7:
			TEN_NOP_FOR_TIMING
			TEN_NOP_FOR_TIMING
			FIVE_NOP_FOR_TIMING
			break;
		default:PIXEL_OFF_NO_NOP() break;
	};
}

inline void putCharOneLine(const uint8_t charBits)
{
	if (charBits & 0b10000000) PIXEL_ON() else PIXEL_OFF()
	if (charBits & 0b01000000) PIXEL_ON() else PIXEL_OFF()
	if (charBits & 0b00100000) PIXEL_ON() else PIXEL_OFF()
	if (charBits & 0b00010000) PIXEL_ON() else PIXEL_OFF()
	if (charBits & 0b00001000) PIXEL_ON() else PIXEL_OFF()
	if (charBits & 0b00000100) PIXEL_ON() else PIXEL_OFF()
	if (charBits & 0b00000010) PIXEL_ON() else PIXEL_OFF()
	if (charBits & 0b00000001) PIXEL_ON() else PIXEL_OFF()
	PIXEL_OFF_NO_NOP()
}

// you won is 5 chars (o dup)
// 012 415  <---- this is it in index into the "chars"
const uint8_t chars[5][8]=
{
{0b10000010,
 0b10000010,
 0b01000100,
 0b00100100,
 0b00010000,
 0b00010000,
 0b00010000,
 0b00010000},

{0b00111000,
 0b01000100,
 0b10000010,
 0b10000010,
 0b10000010,
 0b10000010,
 0b01000100,
 0b00111000},

{0b10000010,
 0b10000010,
 0b10000010,
 0b10000010,
 0b10000010,
 0b10000010,
 0b01000100,
 0b00111000},

{0b10000010,
 0b10000010,
 0b10000010,
 0b10000010,
 0b10000010,
 0b10010010,
 0b01010100,
 0b00101000},

{0b10000010,
 0b11000010,
 0b10100010,
 0b10010010,
 0b10001010,
 0b10000110,
 0b10000010,
 0b00000000},
};

inline void drawGameWon(const int charLineCount)
{
	//012 415 == you won in my char set!
	if ((charLineCount < 8) && (charLineCount >= 0))
	{
		putCharOneLine(chars[0][charLineCount]);
		putCharOneLine(chars[1][charLineCount]);
		putCharOneLine(chars[2][charLineCount]);
		putCharOneLine(chars[3][charLineCount]);
		putCharOneLine(chars[1][charLineCount]);
		putCharOneLine(chars[4][charLineCount]);
	}
}

inline void delayLoop(uint8_t delay)
{
	do
	{
		NOP_FOR_TIMING
	}
	while (delay-- > 0);
}

inline void checkHitAlien (uint8_t *rowStatusAddresses,
				uint8_t fireXPos,
				uint8_t fireYPos,
				uint8_t AlienXPos,
				uint8_t AlienYPos,
				uint8_t mask,
				int *firePressed,
				uint8_t *kill)
{
	if ((fireXPos >= AlienXPos) && (fireXPos-7 <= AlienXPos)
		 && (fireYPos >= AlienYPos) && (fireYPos-20 < AlienYPos))
	{
		if (*rowStatusAddresses & mask)
		{
			(*kill)++;
			*firePressed=0;
		}
		*rowStatusAddresses &= ~(mask);
	}
}
