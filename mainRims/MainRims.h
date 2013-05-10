/*
  Rims.h
  
  Librairie pour un système RIMS 
  (recirculation infusion mash system)
  Gestion du LCD ainsi que du PID en température
  
  Francis Gagnon
*/


#ifndef Rims_h
#define Rims_h

#define KEYNONE 0
#define KEYUP 1
#define KEYDOWN 2
#define KEYLEFT 3
#define KEYRIGHT 4
#define KEYSELECT 5

#include "Arduino.h"
#include "LiquidCrystal.h"

class Rims
{
	
public:
	Rims(LiquidCrystal* lcd, byte col, byte row, byte pinLight,
	     byte pinKeysAnalog);
	void printStrLCD(String mess);
	void start();
	
	byte _readKeys();
	void _printIntLCD(int val, byte col, 
					  byte row,byte curCol, byte curRow);
	

	LiquidCrystal* _lcd;
	byte _pinKeysAnalog;
};

#endif