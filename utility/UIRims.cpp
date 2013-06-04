/*
  UIRims.cpp - UIRims.h definition
*/

#include "Arduino.h"
#include "UIRims.h"

/*
============================================================
TITLE : UIRims (constructor)
DESC : UIRims constructor
============================================================
*/
UIRims::UIRims(LiquidCrystal lcd,byte col,byte row, byte pinLight,
		       byte pinKeysAnalog)
: _lcd(lcd), _pinKeysAnalog(pinKeysAnalog),_waitNone(true),
  _cursorCol(0), _cursorRow(0), _pinLight(pinLight),
  _lastRefreshSP(0), _lastRefreshPV(0), 
  _lastRefreshTime(0), _lastRefreshFlow(0),
  _tempSP(0), _tempPV(0), _time(0), _flow(0)
{
	pinMode(pinLight,OUTPUT);
	digitalWrite(pinLight,HIGH);
	this->_lcd.begin(col,row);
	this->_lcd.clear();
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
	this->_lcd.createChar(1,okChar);
	this->_lcd.createChar(2,upChar);
	this->_lcd.createChar(3,downChar);
}

/*
============================================================
TITLE : showTempScreen
DESC : Show temperature (set point and process value) screen
       on this->_lcd
============================================================
*/
void UIRims::showTempScreen()
{
	this->_tempScreenShown = true;
	this->_lcd.clear();
	this->_printStrLCD(
		String("SP:00.0") + (char)223 +
		String("C(000") + (char)223 + String("F)"),0,0);
	this->_printStrLCD(
	    String("PV:00.0") + (char)223 +
	    String("C(000") + (char)223 + String("F)"),0,1);
	this->setTempSP(this->_tempSP,false);
	this->setTempPV(this->_tempPV,false);
}

/*
============================================================
TITLE : showTimeFlowScreen
DESC : Show remaining time and flow screen on this->_lcd
============================================================
*/
void UIRims::showTimeFlowScreen()
{
	this->_tempScreenShown = false;
	this->_lcd.clear();
	this->_printStrLCD("time:000m00s",0 ,0);
	this->_printStrLCD(String("flow:00.0L/min "+(char)0),0,1);
	this->setTime(this->_time,false);
	this->setFlow(this->_flow,false);
}

/*
============================================================
TITLE : switchScreen
DESC : Toggle between tempScreen and timeFlowScreen on
       this->_lcd
============================================================
*/
void UIRims::switchScreen()
{
	if(this->_tempScreenShown) this->showTimeFlowScreen();
	else this->showTempScreen();
}

/*
============================================================
TITLE : getTempScreenShown
DESC : Return a boolean indicating if tempScreen is shown now.
============================================================
*/
boolean UIRims::getTempScreenShown()
{
	return this->_tempScreenShown;
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
	int adcKeyVal = analogRead(this->_pinKeysAnalog);  
	if (adcKeyVal > 1000) res = KEYNONE;
	else if (adcKeyVal < 50) res = KEYRIGHT;
	else if (adcKeyVal < 195) res = KEYUP;
	else if (adcKeyVal < 380) res = KEYDOWN;
	else if (adcKeyVal < 555) res = KEYLEFT;
	else if (adcKeyVal < 790) res = KEYSELECT;
	if(waitNone)
	{
		if(res==KEYNONE and this->_waitNone) this->_waitNone = false;
		else if(res!=KEYNONE)
		{
			if(this->_waitNone) res = KEYNONE;
			else this->_waitNone = true;
		}
	}
	else this->_waitNone = true;
	return res;
}

/*
============================================================
TITLE : _waitForKeyChange
DESC : Read jeys with software debounce
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
			if(currentTime - refTime >= 10) keyConfirmed = true;
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
TITLE : _printStrLCD
DESC : Show mess at column col and row row on this->_lcd
============================================================
*/
void UIRims::_printStrLCD(String mess, byte col, byte row)
{
	this->_lcd.setCursor(col,row);
	this->_lcd.print(mess);
	this->_lcd.setCursor(this->_cursorCol,this->_cursorRow);
}


/*
============================================================
TITLE : _printFloatLCD
DESC : Show a floating number at column col and row row
       on this->_lcd. Width and prec indicate minimum width
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
	this->_lcd.setCursor(col,row);
	this->_lcd.print(res);
	this->_lcd.setCursor(this->_cursorCol,this->_cursorRow);
}

/*
============================================================
TITLE : _setCursorPosition
DESC : Set this->_lcd cursor at given column col and given row row
============================================================
*/
void UIRims::_setCursorPosition(byte col, byte row)
{
	this->_cursorCol = col;
	this->_cursorRow = row;
	this->_lcd.setCursor(col,row);
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
       shown, it will be updated on the lcd this->_lcd else
       it will be memorized for when it will be shown.
       If waitRefresh is true (default is true) it will
       wait LCDREFRESHTIME mSec before updating
       this->_lcd.
============================================================
*/
void UIRims::setTempSP(float tempCelcius, boolean waitRefresh)
{
	this->_tempSP = tempCelcius;
	if(this->_tempScreenShown)
	{
		boolean refreshLCD = false;
		unsigned long currentTime = millis();
		if(not waitRefresh)	refreshLCD = true;
		else if(currentTime - this->_lastRefreshSP >= LCDREFRESHTIME) \
				refreshLCD = true;
		if(refreshLCD)
		{
			this->_lastRefreshSP = currentTime;
			float tempFahren = this->_celciusToFahrenheit(tempCelcius);
			this->_printFloatLCD(tempCelcius,4,1,3,0);
			this->_printFloatLCD(tempFahren,3,0,10,0);
		}
	}
}

/*
============================================================
TITLE : setTempPV
DESC : Set a new process value temperature. If tempScreen is 
       shown, it will be updated on the lcd this->_lcd else
       it will be memorized for when it will be shown.
       If waitRefresh is true (default is true) it will
       wait LCDREFRESHTIME mSec before updating
       this->_lcd.
============================================================
*/
void UIRims::setTempPV(float tempCelcius, boolean waitRefresh)
{
	this->_tempPV = tempCelcius;
	if(this->_tempScreenShown)
	{
		boolean refreshLCD = false;
		unsigned long currentTime = millis();
		if(not waitRefresh)	refreshLCD = true;
		else if(currentTime - this->_lastRefreshPV >= LCDREFRESHTIME) \
				refreshLCD = true;
		if(refreshLCD)
		{
			this->_lastRefreshPV = currentTime;
			float tempFahren = this->_celciusToFahrenheit(tempCelcius);
			this->_printFloatLCD(tempCelcius,4,1,3,1);
			this->_printFloatLCD(tempFahren,3,0,10,1);
		}
	}
}


/*
============================================================
TITLE : setTime
DESC : Set a new remaining time. If timeFlowScreen is 
       shown, it will be updated on the lcd this->_lcd else
       it will be memorized for when it will be shown.
       If waitRefresh is true (default is true) it will
       wait LCDREFRESHTIME mSec before updating
       this->_lcd.
============================================================
*/
void UIRims::setTime(unsigned int timeSec, boolean waitRefresh)
{
	this->_time = timeSec;
	if(not this->_tempScreenShown)
	{
		boolean refreshLCD = false;
		unsigned long currentTime = millis();
		if(not waitRefresh)	refreshLCD = true;
		else if(currentTime - this->_lastRefreshTime >= LCDREFRESHTIME) \
				refreshLCD = true;
		if(refreshLCD)
		{
			this->_lastRefreshTime = currentTime;
			int minutes = timeSec / 60;
			int seconds = timeSec % 60;
			this->_printFloatLCD(minutes,3,0,5,0);
			this->_printFloatLCD(seconds,2,0,9,0);
		}
	}
}

/*
============================================================
TITLE : setFlow
DESC : Set a new flow value. If timeFlowScreen is 
       shown, it will be updated on the lcd this->_lcd else
       it will be memorized for when it will be shown.
       If waitRefresh is true (default is true) it will
       wait LCDREFRESHTIME mSec before updating
       this->_lcd.
============================================================
*/
void UIRims::setFlow(float flow, boolean waitRefresh)
{
	this->_flow = flow;
	if(not this->_tempScreenShown)
	{
		boolean refreshLCD = false;
		unsigned long currentTime = millis();
		if(not waitRefresh)	refreshLCD = true;
		else if(currentTime - this->_lastRefreshFlow >= LCDREFRESHTIME) \
				refreshLCD = true;
		if(refreshLCD)
		{
			this->_lastRefreshFlow = currentTime;
			this->_printFloatLCD(constrain(flow,0,99.9),4,1,5,1);
			if(flow>=FLOWLOWBOUND and flow<=FLOWUPBOUND) \
					this->_printStrLCD("\x01",15,1);
			else if(flow<FLOWLOWBOUND) this->_printStrLCD("\x03",15,1);
			else this->_printStrLCD("\x02",15,1);
		}
	}
}
/*
============================================================
TITLE : _incDecValue
DESC : Increse or decreasw a floating point value on this->_lcd.
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
	if(this->_cursorCol<dotPosition)
	{
		digitPosition = (dotPosition - this->_cursorCol) - 1;
	}
	else
	{
		digitPosition = (dotPosition - this->_cursorCol);
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
DESC : Move the cursor left or right on this->_lcd. begin
       and end indicate the position (column) of the beginning
       and ending of the cursor bounds. dotPosition is the 
       position (column) of the dot to skip it. row indicate
       the row of the cursor on this->_lcd
============================================================
*/
void UIRims::_moveCursorLR(byte begin, byte end, byte dotPosition,
						   byte row,boolean left)
{
	int way = (left) ? (-1) : (+1);
	if((this->_cursorCol > begin and left) or \
	   (this->_cursorCol < end and not left))
	{
		if (this->_cursorCol == dotPosition - (1*way))
		{
			this->_setCursorPosition(
				this->_cursorCol + (2*way) , row);
		}
		else
		{
			this->_setCursorPosition(
				this->_cursorCol + (1*way) , row);
		}
	}
}

/*
============================================================
TITLE : _askValue
DESC : Ask a value on this->_lcd. begin and end is the cursor
       bounds (columns) for the value. dotPosition is 
       the point mark position. row is the row on this->_lcd.
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
	this->_setCursorPosition(dotPosition-1,row);
	this->_lcd.blink();
	while(not valSelected)
	{
		switch(this->_waitForKeyChange())
		{
			case KEYNONE :
				break;
			case KEYUP :
				value = this->_incDecValue(value,dotPosition,true,
									lowerBound,upperBound,timeFormat);
				break;
			case KEYDOWN :
				value = this->_incDecValue(value,dotPosition,false,
									lowerBound,upperBound,timeFormat);
				break;
			case KEYLEFT :
				this->_moveCursorLR(begin,end,dotPosition,
									row,true);
				break;
			case KEYRIGHT :
				this->_moveCursorLR(begin,end,dotPosition,
									row,false);
				break;
			case KEYSELECT :
				valSelected = true;
				this->_lcd.noBlink();
				this->_setCursorPosition(0,0);
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
	this->_printStrLCD("                 ",0,1);
	res = this->_askValue(3,6,5,0,defaultVal,0.0,99.9,false);
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
	this->_printStrLCD("                 ",0,1);
	res = this->_askValue(5,10,8,0,defaultVal,0,59999,true);
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
	this->_lcd.clear();
	this->_printStrLCD("finished!",0,0);
	refTime = currentTime = millis();
	boolean lightState = true;
	while(this->readKeysADC() == KEYNONE)
	{
		currentTime = millis();
		if(currentTime-refTime>=500)
		{
			refTime = millis();
			lightState = not lightState;
			digitalWrite(this->_pinLight,lightState);
		}
	}
	digitalWrite(this->_pinLight,HIGH);
}

/*
============================================================
TITLE : showPumpWarning
DESC : Show the pump switching warning.
============================================================
*/
void UIRims::showPumpWarning()
{
	this->_lcd.clear();
	this->_printStrLCD("start pump! [OK]",0,0);
	this->_printStrLCD(String("flow:00.0L/min "+(char)0),0,1);
}

/*
============================================================
TITLE : showHeaterWarning
DESC : Show the heater switching warning.
============================================================
*/
void UIRims::showHeaterWarning()
{
	this->_lcd.clear();
	this->_printStrLCD("start heater!",0,0);
	this->_printStrLCD("[OK]",0,1);
}

/*
============================================================
TITLE : showErrorPV
DESC : Show error mess (2 chars max) for the
       process value on this->_lcd.
============================================================
*/
void UIRims::showErrorPV(String mess)
{
	if(this->_tempScreenShown)
	{
		if(mess.length() > 2)
		{
			mess = String(mess).substring(0,2);
		}
		this->_printStrLCD(String(" #")+mess,3,1);
		this->_printStrLCD(String("#")+mess,10,1);
	}
}