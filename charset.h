#include <avr/pgmspace.h>

PROGMEM const uint8_t alphafonts[26+9][8]=
{{0b00000000,
	0b00010000,
	0b00101000,
	0b01000100,
	0b01111100,
	0b01000100,
	0b01000100,
	0b00000000}
,
	{0b00000000,
	0b01111000,
	0b01000100,
	0b01111000,
	0b01000100,
	0b01000100,
	0b01111000,
	0b00000000}
,
	{0b00000000,
	0b00111000,
	0b01000100,
	0b01000000,
	0b01000000,
	0b01000100,
	0b00111000,
	0b00000000}
,
	{0b00000000,
	0b01111000,
	0b01000100,
	0b01000010,
	0b01000010,
	0b01000100,
	0b01111000,
	0b00000000}
,
	{0b00000000,
	0b01111100,
	0b01000000,
	0b01000000,
	0b01111100,
	0b01000000,
	0b01111100,
	0b00000000}
,
	{0b00000000,
	0b01111100,
	0b01000000,
	0b01000000,
	0b01111100,
	0b01000000,
	0b01000000,
	0b00000000}
,
	{0b00000000,
	0b00111000,
	0b01000100,
	0b01000000,
	0b01001100,
	0b01000100,
	0b00111000,
	0b00000000}
,
	{0b00000000,
	0b01000100,
	0b01000100,
	0b01000100,
	0b01111100,
	0b01000100,
	0b01000100,
	0b00000000}
,
	{0b00000000,
	 0b00111000,
	 0b00010000,
	 0b00010000,
	 0b00010000,
	 0b00010000,
	 0b00111000,
	 0b00000000}
,
	{0b00000000,
	 0b00000100,
	 0b00000100,
	 0b00000100,
	 0b00000100,
	 0b01000100,
	 0b00111000,
	 0b00000000}
,
	{0b00000000,
	 0b01000100,
	 0b01000100,
	 0b01001000,
	 0b01110000,
	 0b01001000,
	 0b01000100,
	 0b00000000}
,
	{0b00000000,
	 0b01000000,
	 0b01000000,
	 0b01000000,
	 0b01000000,
	 0b01000000,
	 0b00111100,
	 0b00000000}
,
	{0b00000000,
	 0b10000010,
	 0b11000110,
	 0b10101010,
	 0b10010010,
	 0b10000010,
	 0b10000010,
	 0b00000000}
,
	{0b00000000,
	 0b10000100,
	 0b11000100,
	 0b10100100,
	 0b10010100,
	 0b10001100,
	 0b10000100,
	 0b00000000}
,
	{0b00000000,
	 0b00111000,
	 0b01000100,
	 0b01000100,
	 0b01000100,
	 0b01000100,
	 0b00111000,
	 0b00000000}
,
	{0b00000000,
	 0b00111000,
	 0b01000100,
	 0b01000100,
	 0b01111000,
	 0b01000000,
	 0b01000000,
	 0b00000000}
,
	{0b00000000,
	 0b01111000,
	 0b10000100,
	 0b10000100,
	 0b10010100,
	 0b10001000,
	 0b01110100,
	 0b00000010}
,
	{0b00000000,
	 0b00111000,
	 0b01000100,
	 0b01000100,
	 0b01111000,
	 0b01001000,
	 0b01000100,
	 0b00000000}
,
	{0b00000000,
	 0b00111000,
	 0b01000100,
	 0b00100000,
	 0b00001000,
	 0b01000100,
	 0b00111000,
	 0b00000000}
,
	{0b00000000,
	 0b01111100,
	 0b00010000,
	 0b00010000,
	 0b00010000,
	 0b00010000,
	 0b00010000,
	 0b00000000}
,
	{0b00000000,
	 0b01000100,
	 0b01000100,
	 0b01000100,
	 0b01000100,
	 0b01000100,
	 0b00111000,
	 0b00000000}
,
  {0b00000000,
	 0b01000100,
	 0b01000100,
	 0b01000100,
	 0b01000100,
	 0b00101000,
	 0b00010000,
	 0b00000000}
,
	 {0b00000000,
	 0b01000010,
	 0b01000010,
	 0b01000010,
     0b01011010,
	 0b01100110,
	 0b01000010,
	 0b00000000}
,
	{0b01000100,
	 0b01000100,
	 0b01000100,
	 0b00101000,
	 0b00010000,
	 0b00101000,
	 0b01000100,
	 0b01000100}
,
	 {0b00000000,
	 0b01000100,
	 0b01000100,
	 0b01111100,
	 0b00010000,
	 0b00010000,
	 0b00010000,
	 0b00000000}
,
	 {0b00000000,
	 0b01111100,
	 0b00000100,
	 0b00001000,
	 0b00010000,
	 0b00100000,
	 0b01111100,
	 0b00000000}
,
	{0b00000000,
	 0b01111110,
	 0b01000010,
	 0b01000010,
	 0b01000010,
	 0b01000010,
	 0b01111110,
	 0b00000000
	}
,
   {0b11111111,
	0b10000001,
	0b10111101,
	0b10100101,
	0b10100101,
	0b10111101,
	0b10000001,
	0b11111111}
,
   {0b11111111,
	0b10000001,
	0b01000010,
	0b00011000,
	0b00011000,
	0b01000010,
	0b10000001,
	0b11111111}
,
	{0b11111111,
	0b11111111,
	0b11111111,
	0b11111111,
	0b11111111,
	0b11111111,
	0b11111111,
	0b11111111}
,
	{0b00000000,
	0b00000000,
	0b00000000,
	0b00000000,
	0b00000000,
	0b00000000,
	0b00000000,
	0b00000000}
,
	{0b00000000,
	0b00000000,
	0b00000000,
	0b00000000,
	0b00000000,
	0b00000110,
	0b00000010,
	0b00000100}
,
	{0b00000000,
	0b00011000,
	0b00011000,
	0b00011000,
	0b00011000,
	0b00000000,
	0b00011000,
	0b00000000}
,
   {0b10000001,
	0b01111110,
	0b01011010,
	0b01111110,
	0b00111100,
	0b00100100,
	0b01000010,
	0b10000001}
,
   {0b01000010,
	0b11111111,
	0b01011010,
	0b01100110,
	0b00111100,
	0b00100100,
	0b01000010,
	0b00100100}
};