/*
  UIRims.cpp - UIRims.h definition
*/

#include "Arduino.h"
#include "UIRims.h"

/*************************************************************
 ************************************************************* 
 * UIRims
 *     UI library for Rims
 ************************************************************* 
 *************************************************************/

/*
============================================================
TITLE : UIRims (constructor)
DESC : UIRims constructor
============================================================
*/
UIRims::UIRims(LiquidCrystal lcd,byte col,byte row, byte pinKeysAnalog,
			   byte pinLight, byte pinSpeaker)
: _lcd(lcd), _pinKeysAnalog(pinKeysAnalog),_waitNone(true),
  _cursorCol(0), _cursorRow(0), _pinLight(pinLight), 
  _pinSpeaker(pinSpeaker),
  _lastRefreshSP(0), _lastRefreshPV(0), 
  _lastRefreshTime(0), _lastRefreshFlow(0),
  _tempSP(0), _tempPV(0), _time(0), _flow(0)
{
	pinMode(pinLight,OUTPUT);
	digitalWrite(pinLight,HIGH);
	_lcd.begin(col,row);
	_lcd.clear();
	byte okChar[8] = {
		B01110,
		B10001,
		B01110,
		B00000,
		B11111,
		B00100,
		B11011,
		B00000
	};
	byte upChar[8] = {
		B00100,
		B01110,
		B11111,
		B00100,
		B00100,
		B00100,
		B00100,
		B00000
	};
	byte downChar[8] = {
		B00100,
		B00100,
		B00100,
		B00100,
		B11111,
		B01110,
		B00100,
		B00000
	};
	_lcd.createChar(1,okChar);
	_lcd.createChar(2,upChar);
	_lcd.createChar(3,downChar);
}

/*
============================================================
TITLE : showTempScreen
DESC : Show temperature (set point and process value) screen
       on _lcd
============================================================
*/
void UIRims::showTempScreen()
{
	_tempScreenShown = true;
	_lcd.clear();
	_printStrLCD("SP:00.0\xdf""C(000\xdf""F)",0,0);
	_printStrLCD("PV:00.0\xdf""C(000\xdf""F)",0,1);
	this->setTempSP(_tempSP,false);
	this->setTempPV(_tempPV,false);
}

/*
============================================================
TITLE : showTimeFlowScreen
DESC : Show remaining time and flow screen on _lcd
============================================================
*/
void UIRims::showTimeFlowScreen()
{
	_tempScreenShown = false;
	_lcd.clear();
	_printStrLCD("time:000m00s    ",0 ,0);
	_printStrLCD(String("flow:00.0L/min \x03"),0,1);
	this->setTime(_time,false);
	this->setFlow(_flow,false);
}

/*
============================================================
TITLE : switchScreen
DESC : Toggle between tempScreen and timeFlowScreen on
       _lcd
============================================================
*/
void UIRims::switchScreen()
{
	if(_tempScreenShown) this->showTimeFlowScreen();
	else this->showTempScreen();
}

/*
============================================================
TITLE : readKeysADC
DESC : Read keys without software debouce. If waitNone
       is true (true by default), KEYNONE must be detected
       to return anything else than KEYNONE.
============================================================
*/
byte UIRims::readKeysADC(boolean waitNone)
{
	byte res;
	int adcKeyVal = analogRead(_pinKeysAnalog);  
	if (adcKeyVal > 1000) res = KEYNONE;
	else if (adcKeyVal < 50) res = KEYRIGHT;
	else if (adcKeyVal < 195) res = KEYUP;
	else if (adcKeyVal < 380) res = KEYDOWN;
	else if (adcKeyVal < 555) res = KEYLEFT;
	else if (adcKeyVal < 790) res = KEYSELECT;
	if(waitNone)
	{
		if(res==KEYNONE and _waitNone) _waitNone = false;
		else if(res!=KEYNONE)
		{
			if(_waitNone) res = KEYNONE;
			else _waitNone = true;
		}
	}
	else _waitNone = true;
	return res;
}

/*
============================================================
TITLE : _waitForKeyChange
DESC : Read keys with software debounce
============================================================
*/
byte UIRims::_waitForKeyChange()
{
	boolean keyConfirmed = false, keyDetected = false;
	byte lastKey = this->readKeysADC(false), currentKey;
	unsigned long refTime = millis(), currentTime;
	while(not keyConfirmed)
	{
		currentTime = millis();
		currentKey = this->readKeysADC(false);
		if(keyDetected)
		{
			if(currentTime - refTime >= KEYDEBOUNCETIME) keyConfirmed = true;
		}
		if(currentKey != lastKey)
		{
			keyDetected = true;
			refTime = currentTime;
			lastKey = currentKey;
		}
	}
	return currentKey;
}

/*
============================================================
TITLE : _waitTime
DESC : Pause the Arduino for the given timeInMilliSec
============================================================
*/

void UIRims::_waitTime(unsigned long timeInMilliSec)
{
	unsigned long startTime = millis();
	while(millis() - startTime <= timeInMilliSec) continue;
}

/*
============================================================
TITLE : _printStrLCD
DESC : Show mess at column col and row row on _lcd
============================================================
*/
void UIRims::_printStrLCD(String mess, byte col, byte row)
{
	_lcd.setCursor(col,row);
	_lcd.print(mess);
	_lcd.setCursor(_cursorCol,_cursorRow);
}


/*
============================================================
TITLE : _printFloatLCD
DESC : Show a floating number at column col and row row
       on _lcd. Width and prec indicate minimum width
       (including the dot) and digit after point respectively.
============================================================
*/
void UIRims::_printFloatLCD(float val, int width, int prec,
							byte col, byte row)
{
	char myFloatStr[17];
	String res(dtostrf(val,width,prec,myFloatStr));
	res.replace(' ','0');
	if(res.length() > width)
	{
		res = String(dtostrf(val,width,0,myFloatStr));
	}
	_lcd.setCursor(col,row);
	_lcd.print(res);
	_lcd.setCursor(_cursorCol,_cursorRow);
}

/*
============================================================
TITLE : _setCursorPosition
DESC : Set _lcd cursor at given column col and given row row
============================================================
*/
void UIRims::_setCursorPosition(byte col, byte row)
{
	_cursorCol = col;
	_cursorRow = row;
	_lcd.setCursor(col,row);
}

/*
============================================================
TITLE : _celciusToFahrenheit
DESC : Convert celcius temp to fahrenheit temp
============================================================
*/
float UIRims::_celciusToFahrenheit(float celcius)
{
	return ((9.0/5.0)*celcius)+32.0;
}

/*
============================================================
TITLE : setTempSP
DESC : Set a new set point temperature. If tempScreen is 
       shown, it will be updated on the lcd _lcd else
       it will be memorized for when it will be shown.
       If waitRefresh is true (default is true) it will
       wait LCDREFRESHTIME mSec before updating
       _lcd.
============================================================
*/
void UIRims::setTempSP(float tempCelcius, boolean waitRefresh)
{
	_tempSP = tempCelcius;
	if(_tempScreenShown)
	{
		boolean refreshLCD = false;
		unsigned long currentTime = millis();
		if(not waitRefresh)	refreshLCD = true;
		else if(currentTime - _lastRefreshSP >= LCDREFRESHTIME) \
				refreshLCD = true;
		if(refreshLCD)
		{
			_lastRefreshSP = currentTime;
			float tempFahren = _celciusToFahrenheit(tempCelcius);
			_printFloatLCD(tempCelcius,4,1,3,0);
			_printFloatLCD(tempFahren,3,0,10,0);
		}
	}
}

/*
============================================================
TITLE : setTempPV
DESC : Set a new process value temperature. If tempScreen is 
       shown, it will be updated on the lcd _lcd else
       it will be memorized for when it will be shown.
       If waitRefresh is true (default is true) it will
       wait LCDREFRESHTIME mSec before updating
       _lcd.
============================================================
*/
void UIRims::setTempPV(float tempCelcius, boolean waitRefresh)
{
	_tempPV = tempCelcius;
	if(_tempScreenShown)
	{
		boolean refreshLCD = false;
		unsigned long currentTime = millis();
		if(not waitRefresh)	refreshLCD = true;
		else if(currentTime - _lastRefreshPV >= LCDREFRESHTIME) \
				refreshLCD = true;
		if(refreshLCD)
		{
			_lastRefreshPV = currentTime;
			float tempFahren = _celciusToFahrenheit(tempCelcius);
			_printFloatLCD(tempCelcius,4,1,3,1);
			_printFloatLCD(tempFahren,3,0,10,1);
		}
	}
}


/*
============================================================
TITLE : setTime
DESC : Set a new remaining time. If timeFlowScreen is 
       shown, it will be updated on the lcd _lcd else
       it will be memorized for when it will be shown.
       If waitRefresh is true (default is true) it will
       wait LCDREFRESHTIME mSec before updating
       _lcd.
============================================================
*/
void UIRims::setTime(unsigned int timeSec, boolean waitRefresh)
{
	_time = timeSec;
	if(not _tempScreenShown)
	{
		boolean refreshLCD = false;
		unsigned long currentTime = millis();
		if(not waitRefresh)	refreshLCD = true;
		else if(currentTime - _lastRefreshTime >= LCDREFRESHTIME) \
				refreshLCD = true;
		if(refreshLCD)
		{
			_lastRefreshTime = currentTime;
			int minutes = timeSec / 60;
			int seconds = timeSec % 60;
			_printFloatLCD(minutes,3,0,5,0);
			_printFloatLCD(seconds,2,0,9,0);
		}
	}
}

/*
============================================================
TITLE : setFlow
DESC : Set a new flow value. If timeFlowScreen is 
       shown, it will be updated on the lcd _lcd else
       it will be memorized for when it will be shown.
       If waitRefresh is true (default is true) it will
       wait LCDREFRESHTIME mSec before updating
       _lcd.
============================================================
*/
void UIRims::setFlow(float flow, boolean waitRefresh)
{
	_flow = flow;
	if(not _tempScreenShown)
	{
		boolean refreshLCD = false;
		unsigned long currentTime = millis();
		if(not waitRefresh)	refreshLCD = true;
		else if(currentTime - _lastRefreshFlow >= LCDREFRESHTIME) \
				refreshLCD = true;
		if(refreshLCD)
		{
			_lastRefreshFlow = currentTime;
			_printFloatLCD(constrain(flow,0,99.9),4,1,5,1);
			if(flow>=FLOWLOWBOUND and flow<=FLOWUPBOUND) \
					_printStrLCD("\x01",15,1);
			else if(flow<FLOWLOWBOUND) _printStrLCD("\x03",15,1);
			else _printStrLCD("\x02",15,1);
		}
	}
}

/*
============================================================
TITLE : _incDecValue
DESC : Increse or decreasw a floating point value on _lcd.
       dotPosition give the position (column) of the point mark.
       lowerBound and upperBound block is the incresing/decreasing
       limit of the value. timeFormat treats the value (in seconds) as
       a time with minutes and secondes.
============================================================
*/
float UIRims::_incDecValue(float value,byte dotPosition, boolean increase,
						   float lowerBound, float upperBound,
						   boolean timeFormat)
{
	float res,constrainedRes;
	int digitPosition, way = (increase) ? (+1) : (-1);
	if(_cursorCol<dotPosition)
	{
		digitPosition = (dotPosition - _cursorCol) - 1;
	}
	else
	{
		digitPosition = (dotPosition - _cursorCol);
	}
	if(not timeFormat)
	{
		res = value + (way*pow(10,digitPosition));
	}
	else
	{
		if(digitPosition<0)
		{
			res = value + (way*pow(10,digitPosition + 2));
		}
		else
		{
			res = value + (way*60*pow(10,digitPosition));
		}
	}
	constrainedRes = (constrain(res,lowerBound,upperBound)!= res) ? \
	                 (value) : (res);
	if(not timeFormat) this->setTempSP(constrainedRes,false);
	else this->setTime(constrainedRes,false);
	return constrainedRes;
}

/*
============================================================
TITLE : _moveCursorLR
DESC : Move the cursor left or right on _lcd. begin
       and end indicate the position (column) of the beginning
       and ending of the cursor bounds. dotPosition is the 
       position (column) of the dot to skip it. row indicate
       the row of the cursor on _lcd
============================================================
*/
void UIRims::_moveCursorLR(byte begin, byte end, byte dotPosition,
						   byte row,boolean left)
{
	int way = (left) ? (-1) : (+1);
	if((_cursorCol > begin and left) or \
	   (_cursorCol < end and not left))
	{
		if (_cursorCol == dotPosition - (1*way))
		{
			_setCursorPosition(
				_cursorCol + (2*way) , row);
		}
		else
		{
			_setCursorPosition(
				_cursorCol + (1*way) , row);
		}
	}
}

/*
============================================================
TITLE : _askValue
DESC : Ask a value on _lcd. begin and end is the cursor
       bounds (columns) for the value. dotPosition is 
       the point mark position. row is the row on _lcd.
       defaultVal is the starting value. lowerBound and upperBound
       is the value's limits. timeFormat treats the value as
       a time with minutes and seconds.
============================================================
*/
float UIRims::_askValue(byte begin, byte end, 
						byte dotPosition, byte row,
						float defaultVal,
						float lowerBound, float upperBound,
						boolean timeFormat)
{
	boolean valSelected = false;
	float value = defaultVal;
	timeFormat ? this->setTime(defaultVal,false) : \
	             this->setTempSP(defaultVal,false);
	_setCursorPosition(dotPosition-1,row);
	_lcd.blink();
	_waitTime(500);
	while(not valSelected)
	{
		switch(_waitForKeyChange())
		{
			case KEYNONE :
				break;
			case KEYUP :
				value = _incDecValue(value,dotPosition,true,
									lowerBound,upperBound,timeFormat);
				break;
			case KEYDOWN :
				value = _incDecValue(value,dotPosition,false,
									lowerBound,upperBound,timeFormat);
				break;
			case KEYLEFT :
				_moveCursorLR(begin,end,dotPosition,
									row,true);
				break;
			case KEYRIGHT :
				_moveCursorLR(begin,end,dotPosition,
									row,false);
				break;
			case KEYSELECT :
				valSelected = true;
				_lcd.noBlink();
				_setCursorPosition(0,0);
				break;
		}
	}
	return value;
}

/*
============================================================
TITLE : askSetPoint
DESC : Ask set point temperature with the UI.
============================================================
*/
float UIRims::askSetPoint(float defaultVal)
{
	float res;
	this->showTempScreen();
	_printStrLCD("                 ",0,1);
	res = _askValue(3,6,5,0,defaultVal,0.0,99.9,false);
	return res;
}

/*
============================================================
TITLE : askTime
DESC : Ask timer time on the UI.
============================================================
*/
unsigned int UIRims::askTime(unsigned int defaultVal)
{
	unsigned int res;
	this->showTimeFlowScreen();
	_printStrLCD("                 ",0,1);
	res = _askValue(5,10,8,0,defaultVal,0,59999,true);
	return res;
}

/*
============================================================
TITLE : showEnd
DESC : Show the ending screen.
============================================================
*/
void UIRims::showEnd()
{
	unsigned long refTime, currentTime;
	_lcd.clear();
	_printStrLCD("finished!",0,0);
	_waitTime(500);
	refTime = currentTime = millis();
	boolean lightState = true;
	if(_pinSpeaker != -1) tone(_pinSpeaker,1000,500);
	while(this->readKeysADC() == KEYNONE)
	{
		currentTime = millis();
		if(currentTime-refTime>=500)
		{
			refTime = currentTime;
			lightState = not lightState;
			if(_pinSpeaker != -1 and lightState) 
			{
				tone(_pinSpeaker,1000,500);
			}
			digitalWrite(_pinLight,lightState);
		}
	}
	digitalWrite(_pinLight,HIGH);
}

/*
============================================================
TITLE : showPumpWarning
DESC : Show the pump switching warning.
============================================================
*/
void UIRims::showPumpWarning()
{
	_lcd.clear();
	_printStrLCD("start pump! [OK]",0,0);
	_printStrLCD(String("flow:00.0L/min \x03"),0,1);
	_waitTime(500);
}

/*
============================================================
TITLE : showHeaterWarning
DESC : Show the heater switching warning.
============================================================
*/
void UIRims::showHeaterWarning()
{
	_lcd.clear();
	_printStrLCD("start heater!",0,0);
	_printStrLCD("[OK]",0,1);
	_waitTime(500);
}

/*
============================================================
TITLE : showErrorPV
DESC : Show error mess (2 chars max) for the
       process value on _lcd.
============================================================
*/
void UIRims::showErrorPV(String mess)
{
	if(_tempScreenShown)
	{
		if(mess.length() > 2)
		{
			mess = String(mess).substring(0,2);
		}
		_printStrLCD(String(" #")+mess,3,1);
		_printStrLCD(String("#")+mess,10,1);
	}
}