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

#include "Arduino.h"
#include "LiquidCrystal.h"

class UIRims
{
	
public:
	
	UIRims(LiquidCrystal* lcd, byte col, byte row, byte pinLight,
	     byte pinKeysAnalog);
	
	// === INFO DIALOGS ===
	void showTempScreen();
	void showTimeFlowScreen();
	void switchScreen();
	boolean getTempScreenShown();
	void showEnd();
	
	// === VARIABLES ===
	void setTempSP(float tempCelcius);
	void setTempPV(float tempCelcius);
	void setTime(unsigned int timeSec);
	void setFlow(float flow); //liter/min
	
	// === KEYS READER ===
	byte readKeysADC();
	
	// === SETUP DIALOGS ===
	float askSetPoint(float defaultVal); // Celsius
	unsigned int askTime(unsigned int defaultVal); // seconds

	// === ERROR HANDLING ===
	void showErrorPV(String mess);
	
	
protected:
	
	byte _waitForKeyChange();
	
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
	boolean _tempScreenShown;
};

#endif