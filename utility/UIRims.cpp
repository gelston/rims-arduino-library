/*
  UIRims.cpp - Définition de UIRims.h
*/

#include "Arduino.h"
#include "UIRims.h"

/*
============================================================
TITLE : UIRims (constructeur)
INPUT : int pin (entrée 0-3.3V)
OUTPUT : void
DESC : Constructeur d'un objet UIRims
============================================================
*/
UIRims::UIRims(LiquidCrystal* lcd,byte col,byte row, byte pinLight,
		   byte pinKeysAnalog)
: _lcd(lcd), _pinKeysAnalog(pinKeysAnalog),
  _cursorCol(0), _cursorRow(0), _waitNone(false)
{
	pinMode(pinLight,OUTPUT);
	digitalWrite(pinLight,HIGH);
	this->_lcd->begin(col,row);
	this->_lcd->clear();
}

/*
============================================================
TITLE : showTempScreen
INPUT : 
OUTPUT : void
DESC : 
============================================================
*/
void UIRims::showTempScreen()
{
	this->_lcd->clear();
	this->_printStrLCD(
		String("SP:00.0") + (char)223 +
		String("C(000") + (char)223 + String("F)"),0,0);
	this->_printStrLCD(
	    String("PV:00.0") + (char)223 +
	    String("C(000") + (char)223 + String("F)"),0,1);
}

/*
============================================================
TITLE : showTimeFlowScreen
INPUT : 
OUTPUT : void
DESC : 
============================================================
*/
void UIRims::showTimeFlowScreen()
{
	this->_lcd->clear();
	this->_printStrLCD("time:00:00:00",0,0);
	this->_printStrLCD("flow:00.0 l/min",0,1);
}

/*
============================================================
TITLE : _readKeysADC
INPUT : none
OUTPUT : byte (bouton appuyé)
DESC : Récupère le bouton présentement appuyé sans debounce
============================================================
*/
byte UIRims::_readKeysADC()
{
	byte res;
	int adcKeyVal = analogRead(0);  
	if (adcKeyVal > 1000) res = KEYNONE;
	else if (adcKeyVal < 50) res = KEYRIGHT;
	else if (adcKeyVal < 195) res = KEYUP;
	else if (adcKeyVal < 380) res = KEYDOWN;
	else if (adcKeyVal < 555) res = KEYLEFT;
	else if (adcKeyVal < 790) res = KEYSELECT;
	//Serial.println(res);
	return res;
}

/*
============================================================
TITLE : readKeys
INPUT : none
OUTPUT : byte (bouton appuyé)
DESC : Récupère le bouton présentement appuyé avec debounce
============================================================
*/
byte UIRims::readKeys()
{
	boolean keyConfirmed = false, keyDetected = false;
	byte lastKey = this->_readKeysADC(), currentKey;
	unsigned long refTime = millis(), currentTime;
	while(not keyConfirmed)
	{
		currentTime = millis();
		currentKey = this->_readKeysADC();
		if(keyDetected)
		{
			if(currentTime - refTime >= 10)
			{
				if(currentKey == lastKey)
				{
					keyConfirmed = true;
				}
				else
				{
					keyDetected = false;
					refTime = currentTime;
					lastKey = currentKey;
				}
			}
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
INPUT : LiquidCrystal* lcd, String mess
OUTPUT : void
DESC : Affiche mess sur le lcd
============================================================
*/
void UIRims::_printStrLCD(String mess, byte col, byte row)
{
	this->_lcd->setCursor(col,row);
	this->_lcd->print(mess);
	this->_lcd->setCursor(this->_cursorCol,this->_cursorRow);
}


/*
============================================================
TITLE : _printFloatLCD
INPUT : float val, int width, int prec,
		byte col, byte row
OUTPUT : void
DESC : 
============================================================
*/
void UIRims::_printFloatLCD(float val, int width, int prec,
							byte col, byte row)
{
	char myFloatStr[17];
	String res(dtostrf(val,width,prec,myFloatStr));
	res.replace(String(' '),String('0'));
	this->_lcd->setCursor(col,row);
	this->_lcd->print(res);
	this->_lcd->setCursor(this->_cursorCol,this->_cursorRow);
}

/*
============================================================
TITLE : _setCursorPosition
INPUT : -
OUTPUT : void
DESC : Affiche un float de 3 digits sur le lcd à la 
       position donnée
============================================================
*/
void UIRims::_setCursorPosition(byte col, byte row)
{
	this->_cursorCol = col;
	this->_cursorRow = row;
	this->_lcd->setCursor(col,row);
}

/*
============================================================
TITLE : _celciusToFahrenheit
INPUT : float celcius
OUTPUT : float fahrenheit
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
INPUT : float tempCelcius
OUTPUT : void
DESC : 
============================================================
*/
void UIRims::setTempSP(float tempCelcius)
{
	float tempFahren = this->_celciusToFahrenheit(tempCelcius);
	this->_printFloatLCD(tempCelcius,4,1,3,0);
	this->_printFloatLCD(tempFahren,3,0,10,0);
}

/*
============================================================
TITLE : setTempPV
INPUT : float tempCelcius
OUTPUT : void
DESC : 
============================================================
*/
void UIRims::setTempPV(float tempCelcius)
{
	float tempFahren = this->_celciusToFahrenheit(tempCelcius);
	this->_printFloatLCD(tempCelcius,4,1,3,1);
	this->_printFloatLCD(tempFahren,3,0,10,1);
}


/*
============================================================
TITLE : setTime
INPUT : float tempCelcius
OUTPUT : void
DESC : 
============================================================
*/
void UIRims::setTime(int timeSec)
{
	int hours   = timeSec / 3600;
	int minutes = (timeSec % 3600)/60;
	int seconds = (timeSec % 3600)%60;
	this->_printFloatLCD(hours,2,0,5,0);
	this->_printFloatLCD(minutes,2,0,8,0);
	this->_printFloatLCD(seconds,2,0,11,0);
}


/*
============================================================
TITLE : _incDecValue
INPUT : float value, int digitPosition, 
		boolean increase,
		float lowerBound,float upperBound
OUTPUT : float
DESC : 
============================================================
*/
float UIRims::_incDecValue(float value, int digitPosition, 
					       boolean increase,
						   float lowerBound,float upperBound)
{
	float res,constrainedRes;
	if(increase)
	{
		res = value + pow(10,digitPosition);
	}
	else
	{
		res = value - pow(10,digitPosition);
	}
	Serial.println(res);
	constrainedRes = constrain(res,lowerBound,upperBound);
	return (constrainedRes != res) ? value : res;
}

/*
============================================================
TITLE : _applyValueModifs
INPUT : byte col, byte row, boolean time
OUTPUT : float
DESC : 
============================================================
*/
float UIRims::_applyValueModifs(float value,byte dotPosition, boolean increase,
								float lowerBound, float upperBound,
								boolean timeFormat)
{
	int digitPosition;
	if(not timeFormat)
	{
		if(this->_cursorCol<dotPosition)
		{
			digitPosition = (dotPosition - this->_cursorCol) - 1;
		}
		else
		{
			digitPosition = (dotPosition - this->_cursorCol);
		}
		value = this->_incDecValue(value,digitPosition,increase,
								   lowerBound,upperBound);
		this->setTempSP(value);
	}
	else
	{
		this->setTime(value);
	}
	return value;
}


/*
============================================================
TITLE : _askValue
INPUT : -
OUTPUT : float value
DESC : Demande le setPoint à l'utilisateur.
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
	timeFormat ? this->setTime(defaultVal):this->setTempSP(defaultVal);
	this->_setCursorPosition(dotPosition-1,row);
	this->_lcd->blink();
	while(not valSelected)
	{
		switch(this->readKeys())
		{
			case KEYNONE :
				if(this->_waitNone) this->_waitNone=false;
				break;
			case KEYUP :
				if(not this->_waitNone)
				{
					value = this->_applyValueModifs(value,dotPosition,true,
										lowerBound,upperBound,timeFormat);
					this->_waitNone = true;
				}
				break;
			case KEYDOWN :
				if(not this->_waitNone)
				{
					value = this->_applyValueModifs(value,dotPosition,false,
										lowerBound,upperBound,timeFormat);
					this->_waitNone = true;
				}
				break;
			case KEYLEFT :
				if(not this->_waitNone)
					if(this->_cursorCol > begin)
					{
						if (this->_cursorCol == dotPosition + 1)
						{
							this->_setCursorPosition(this->_cursorCol-2,
													 row);
						}
						else
						{
							this->_setCursorPosition(this->_cursorCol-1,
													 row);
						}
					}
					this->_waitNone = true;
				break;
			case KEYRIGHT :
				if(not this->_waitNone)
					if(this->_cursorCol < end)
					{
						if (this->_cursorCol == dotPosition-1)
						{
							this->_setCursorPosition(this->_cursorCol+2,
													 row);
						}
						else
						{
							this->_setCursorPosition(this->_cursorCol+1,
													 row);
						}
					}
					this->_waitNone = true;
				break;
			case KEYSELECT :
				if(not this->_waitNone)
				{
					valSelected = true;
					this->_lcd->noBlink();
					this->_setCursorPosition(0,0);
					this->_waitNone = true;
				}
				break;
		}
	}
	return value;
}

/*setPoint
============================================================
TITLE : askSetPoint
INPUT : -
OUTPUT : float setPoint
DESC : Demande le setPoint à l'utilisateur.setPoint
============================================================
*/
float UIRims::askSetPoint()
{
	float res;
	this->showTempScreen();
	res = this->_askValue(3,6,5,0,DEFAULTSP,0.0,99.9,false);
	return res;
}

/*
============================================================
TITLE : askTime
INPUT : -
OUTPUT : float time (seconds)
DESC : Demande le temps du pallier à l'utilisateur
============================================================
*/
int UIRims::askTime()
{
	int res;
	this->showTimeFlowScreen();
	res = this->_askValue(5,12,7,0,DEFAULTTIME,0,362439,true);
	return res;
}