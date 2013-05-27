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

#define LCDREFRESHTIME 500 // mSec

#define FLOWLOWBOUND 2.0
#define FLOWUPBOUND 4.0

#include "Arduino.h"
#include "LiquidCrystal.h"

class UIRims
{
	
public:
	
	UIRims(LiquidCrystal lcd, byte col, byte row, byte pinLight,
	     byte pinKeysAnalog);
	
	// === INFO DIALOGS ===
	void showTempScreen();
	void showTimeFlowScreen();
	void switchScreen();
	boolean getTempScreenShown();
	void showEnd();
	
	// === VARIABLE SETTER ===
	void setTempSP(float tempCelcius, boolean waitRefresh = true);
	void setTempPV(float tempCelcius, boolean waitRefresh = true);
	void setTime(unsigned int timeSec, boolean waitRefresh = true);
	void setFlow(float flow, boolean waitRefresh = true); //liter/min
	
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
	
	LiquidCrystal _lcd;
	byte _cursorCol;
	byte _cursorRow;
	byte _pinKeysAnalog;
	byte _pinLight;
	boolean _tempScreenShown;
	
	unsigned long _lastRefreshSP;
	unsigned long _lastRefreshPV;
	unsigned long _lastRefreshTime;
	unsigned long _lastRefreshFlow;
	
	float _tempSP;
	float _tempPV;
	unsigned int _time;
	float _flow;
};

#endif