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

#define DEFAULTSP 68 // Celsius
#define DEFAULTTIME 5400 // seconds

#include "Arduino.h"
#include "LiquidCrystal.h"

class UIRims
{
	
public:
	UIRims(LiquidCrystal* lcd, byte col, byte row, byte pinLight,
	     byte pinKeysAnalog);
	void showTempScreen();
	void showTimeFlowScreen();
	void setTempSP(float tempCelcius);
	void setTempPV(float tempCelcius);
	void setTime(int timeSec);
	
	byte readKeys();
	
	float askSetPoint(); // Celsius
	int askTime(); // seconds
protected:
	byte _readKeysADC();
	void _printStrLCD(String mess, byte col, byte row);
	void _printFloatLCD(float val, int width, int prec,
						byte col, byte row);
	void _setCursorPosition(byte col, byte row);
	float _celciusToFahrenheit(float celcius);
	float _incDecValue(float value, int digitPosition, 
					   boolean increase,
					   float lowerBound,float upperBound);
	float _applyValueModifs(float value,byte dotPosition, 
							boolean increase,
							float lowerBound, float upperBound,
							boolean timeFormat);
	float _askValue(byte begin, byte end, 
					byte dotPosition, byte row,
					float defaultVal,
				    float lowerBound, float upperBound,
					boolean timeFormat);
private:
	LiquidCrystal* _lcd;
	byte _cursorCol;
	byte _cursorRow;
	byte _pinKeysAnalog;
	boolean _waitNone;
};

#endif