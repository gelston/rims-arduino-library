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
  _cursorCol(0), _cursorRow(0), _waitNone(true)
{
	pinMode(pinLight,OUTPUT);
	digitalWrite(pinLight,HIGH);
	this->_lcd->begin(col,row);
	this->_lcd->clear();
}


/*
============================================================
TITLE : _readKeys
INPUT : none
OUTPUT : int (bouton appuyé)
DESC : Récupère le bouton présentement appuyé
============================================================
*/
byte UIRims::_readKeys()
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
DESC : Affiche un float de 3 digits sur le lcd à la 
       position donnée
============================================================
*/
void UIRims::_printFloatLCD(float val, int width, int prec,
							byte col, byte row)
{
	this->_lcd->noBlink();
	char myFloatStr[5];
	dtostrf(val,width,prec,myFloatStr);
	this->_lcd->setCursor(col,row);
	this->_lcd->print(myFloatStr);
	this->_lcd->setCursor(this->_cursorCol,this->_cursorRow);
	this->_lcd->blink();
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
	//Serial.println(this->_celciusToFahrenheit(tempCelcius));
	char myFloatStr[4];
	dtostrf(this->_celciusToFahrenheit(tempCelcius),
			3,0,myFloatStr);
	this->_printFloatLCD(tempCelcius,4,1,3,0);
	this->_printStrLCD(myFloatStr,10,0);
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
	char myFloatStr[4];
	dtostrf(this->_celciusToFahrenheit(tempCelcius),
			3,0,myFloatStr);
	this->_printFloatLCD(tempCelcius,4,1,3,1);
	this->_printStrLCD(myFloatStr,10,1);
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
	int minutes = timeSec/60;
	int seconds = timeSec%60;
	this->_printFloatLCD(minutes,3,0,5,0);
	this->_printFloatLCD(seconds,2,0,11,0);
}


/*
============================================================
TITLE : _incDecSetPoint
INPUT : float curSP, boolean positive
OUTPUT : float
DESC : 
============================================================
*/
float UIRims::_incDecSetPoint(float curSetPoint, boolean positive)
{
	float res,constrainedRes;
	switch(this->_cursorCol)
	{
		case 3:
			res = (positive) ? (curSetPoint + 10) : (curSetPoint - 10);
			break;
		case 4:
			res = (positive) ? (curSetPoint + 1) : (curSetPoint - 1);
			break;
		case 6:
			res = (positive) ? (curSetPoint + 0.1) : (curSetPoint - 0.1);
			break;
	}
	constrainedRes = constrain(res,0.0,99.9);
	return (constrainedRes != res) ? curSetPoint : res;
}

/*
============================================================
TITLE : askSetPoint
INPUT : -
OUTPUT : float setPoint
DESC : Demande le setPoint à l'utilisateur.
============================================================
*/
float UIRims::askSetPoint()
{
	this->_lcd->clear();
	this->_printStrLCD(
		String("SP:00.0") + (char)223 +
		String("C(000") + (char)223 + String("F)"),0,0);
	this->_printStrLCD(
	    String("PV:20.0") + (char)223 +
	    String("C( 68") + (char)223 + String("F)"),0,1);
	this->setTempSP(DEFAULTSP);
	this->_setCursorPosition(4,0);
	this->_lcd->blink();
	boolean tempSelected = false;
	float setPoint = DEFAULTSP;
	while(not tempSelected)
	{
		switch(this->_readKeys())
		{
			case KEYNONE :
				if(this->_waitNone) this->_waitNone=false;
				break;
			case KEYUP :
				if(not this->_waitNone)
				{
					setPoint = this->_incDecSetPoint(setPoint,true);
					this->setTempSP(setPoint);
					this->_waitNone = true;
				}
				break;
			case KEYDOWN :
				if(not this->_waitNone)
				{
					setPoint = this->_incDecSetPoint(setPoint,false);
					this->setTempSP(setPoint);
					this->_waitNone = true;
				}
				break;
			case KEYLEFT :
				if(not this->_waitNone)
					switch(this->_cursorCol)
					{
						case 3 :
							break;
						case 4 :
							this->_setCursorPosition(3,0);
							break;
						case 6 :
							this->_setCursorPosition(4,0);
							break;
						default :
							this->_setCursorPosition(4,0);
							break;
					}
					this->_waitNone = true;
				break;
			case KEYRIGHT :
				if(not this->_waitNone)
					switch(this->_cursorCol)
					{
						case 3 :
							this->_setCursorPosition(4,0);
							break;
						case 4 :
							this->_setCursorPosition(6,0);
							break;
						case 6 :
							break;
						default :
							this->_setCursorPosition(4,0);
							break;
					}
					this->_waitNone = true;
				break;
			case KEYSELECT :
				if(not this->_waitNone)
				{
					tempSelected = true;
					this->_lcd->noBlink();
					this->_setCursorPosition(0,0);
					this->_waitNone = true;
				}
				break;
		}
	}
	return setPoint;
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
	this->_lcd->clear();
	this->_printStrLCD("time:000min00sec",0,0);
	this->setTime(DEFAULTTIME);
	this->_setCursorPosition(6,0);
	this->_lcd->blink();
	boolean timeSelected = false;
	int timeSec = DEFAULTTIME;
	while(not timeSelected)
	{
		switch(this->_readKeys())
		{
			case KEYNONE :
					if(this->_waitNone) this->_waitNone=false;
					break;
				case KEYUP :
					if(not this->_waitNone)
					{
						this->_waitNone = true;
					}
					break;
				case KEYDOWN :
					if(not this->_waitNone)
					{
						this->_waitNone = true;
					}
					break;
				case KEYLEFT :
					if(not this->_waitNone)
						switch(this->_cursorCol)
						{
							case 5 :
								break;
							case 6 :
								this->_setCursorPosition(5,0);
								break;
							case 7 :
								this->_setCursorPosition(6,0);
								break;
							case 11 :
								this->_setCursorPosition(7,0);
								break;
							case 12 :
								this->_setCursorPosition(11,0);
								break;
							default :
								this->_setCursorPosition(6,0);
								break;
						}
						this->_waitNone = true;
					break;
				case KEYRIGHT :
					if(not this->_waitNone)
						switch(this->_cursorCol)
						{
							case 5 :
								this->_setCursorPosition(6,0);
								break;
							case 6 :
								this->_setCursorPosition(7,0);
								break;
							case 7 :
								this->_setCursorPosition(11,0);
								break;
							case 11 :
								this->_setCursorPosition(12,0);
								break;
							case 12 :
								break;
							default :
								this->_setCursorPosition(6,0);
								break;
						}
						this->_waitNone = true;
					break;
				case KEYSELECT :
					if(not this->_waitNone)
					{
						timeSelected = true;
						this->_lcd->noBlink();
						this->_setCursorPosition(0,0);
						this->_waitNone = true;
					}
					break;
		}
	}
	return 0.0;
}