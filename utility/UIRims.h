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
	void showEnd();
	
	void setTempSP(float tempCelcius);
	void setTempPV(float tempCelcius);
	void setTime(unsigned int timeSec);
	void setFlow(int flow); //liter/min
	
	byte waitForKeyChange();
	
	float askSetPoint(float defaultVal); // Celsius
	int askTime(float defaultVal); // seconds
	
	
protected:
	
	byte _readKeysADC();
	void _printStrLCD(String mess, byte col, byte row);
	void _printFloatLCD(float val, int width, int prec,
						byte col, byte row);
	void _setCursorPosition(byte col, byte row);
	float _celciusToFahrenheit(float celcius);
	void _moveCursorLR(byte begin, byte end, byte dotPosition,
					   byte row,boolean left);
	float _incDecValue(float value,byte dotPosition, 
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
	byte _pinLight;
};

#endif