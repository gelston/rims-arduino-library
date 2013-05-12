/*
  UIRims.h
  
  Librairie pour gérer le display et keypad
  pour le Rims
  
  Francis Gagnon
*/


#ifndef UIRims_h
#define UIRims_h

#define KEYNONE 0
#define KEYUP 1
#define KEYDOWN 2
#define KEYLEFT 3
#define KEYRIGHT 4
#define KEYSELECT 5

#define DEFAULTSP 20

#include "Arduino.h"
#include "LiquidCrystal.h"

class UIRims
{
	
public:
	UIRims(LiquidCrystal* lcd, byte col, byte row, byte pinLight,
	     byte pinKeysAnalog);
	void setTempSP(float tempCelcius);
	void setTempPV(float tempCelcius);
	float getSetPoint();
	
	byte _readKeys();
	void _printStrLCD(String mess, byte col, byte row);
	void _printFloatLCD(float val, byte col, byte row);
	void _setCursorPosition(byte col, byte row);
	float _celciusToFahrenheit(float celcius);
	float _incDecSetPoint(float curSetPoint, boolean positive);

	LiquidCrystal* _lcd;
	byte _cursorCol;
	byte _cursorRow;
	byte _pinKeysAnalog;
};

#endif