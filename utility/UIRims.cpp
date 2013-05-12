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
  _cursorCol(0), _cursorRow(0)
{
	pinMode(pinLight,OUTPUT);
	digitalWrite(pinLight,HIGH);
	this->_lcd->begin(col,row);
	this->_lcd->clear();
	this->_printStrLCD(
		String("SP:00.0")+(char)223+
		String("C(000")+(char)223+String("F)"),0,0);
	this->_printStrLCD(
	    String("PV:20.0")+(char)223+
	    String("C( 68")+(char)223+String("F)"),0,1);
	this->setTempSP(DEFAULTSP);
	this->_setCursorPosition(4,0);
	this->_lcd->blink();
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
INPUT : -
OUTPUT : void
DESC : Affiche un float de 3 digits sur le lcd à la 
       position donnée
============================================================
*/
void UIRims::_printFloatLCD(float val, byte col, byte row)
{
	this->_lcd->noBlink();
	char myFloatStr[5];
	dtostrf(val,4,1,myFloatStr);
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
	//TODO
	//TODO
	Serial.println(this->_celciusToFahrenheit(tempCelcius));
	char myFloatStr[6];
	dtostrf(this->_celciusToFahrenheit(tempCelcius),
			3,0,myFloatStr);
	this->_printFloatLCD(tempCelcius,3,0);
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
	//TODO
	//TODO
	this->_printFloatLCD(tempCelcius,3,1);
	this->_printFloatLCD(this->_celciusToFahrenheit(tempCelcius),10,1);
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
TITLE : initialize
INPUT : -
OUTPUT : void
DESC : Routine principale
============================================================
*/
float UIRims::getSetPoint()
{
	boolean tempSelected = false, waitNone = false;
	float setPoint = DEFAULTSP;
	while(not tempSelected)
	{
		switch(this->_readKeys())
		{
			case KEYNONE :
				if(waitNone) waitNone=false;
				break;
			case KEYUP :
				if(not waitNone)
				{
					setPoint = this->_incDecSetPoint(setPoint,true);
					this->setTempSP(setPoint);
					waitNone = true;
				}
				break;
			case KEYDOWN :
				if(not waitNone)
				{
					setPoint = this->_incDecSetPoint(setPoint,false);
					this->setTempSP(setPoint);
					waitNone = true;
				}
				break;
			case KEYLEFT :
				if(not waitNone)
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
					waitNone = true;
				break;
			case KEYRIGHT :
				if(not waitNone)
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
					waitNone = true;
				break;
			case KEYSELECT :
				tempSelected = true;
				this->_lcd->noBlink();
				break;
		}
	}
	return setPoint;
}