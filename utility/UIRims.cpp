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
: _lcd(lcd), _pinKeysAnalog(pinKeysAnalog)
{
	pinMode(pinLight,OUTPUT);
	digitalWrite(pinLight,HIGH);
	this->_lcd->begin(col,row);
	this->printStrLCD(String("SP:20") + (char)223 + String("C\n") +
	                  String("PV:20.0") + (char)223 + String("C"));
	this->_lcd->setCursor(4,0);
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
TITLE : printStrLCD
INPUT : LiquidCrystal* lcd, String mess
OUTPUT : void
DESC : Affiche mess sur le lcd
============================================================
*/
void UIRims::printStrLCD(String mess)
{
	this->_lcd->blink();
	this->_lcd->clear();
	int newLineIndex = mess.indexOf('\n');
	if(newLineIndex != -1)
	{
		this->_lcd->print(mess.substring(0,newLineIndex));
		this->_lcd->setCursor(0,1);
		this->_lcd->print(mess.substring(newLineIndex+1));
	}
	else
	{
		this->_lcd->print(mess);
	}
}


/*
============================================================
TITLE : _printIntLCD
INPUT : -
OUTPUT : void
DESC : Affiche un entier de 2 digits à la position donnée 
       sur le LCD aligné à la droite.
============================================================
*/
void UIRims::_printIntLCD(int val, byte col, byte row,
						byte curCol, byte curRow)
{
	this->_lcd->noBlink();
	byte valLen = String(val).length();
	this->_lcd->setCursor(col,row);
	if(val<10) this->_lcd->print(String("0")+String(val));
    else this->_lcd->print(val);
	this->_lcd->setCursor(curCol,curRow);
	this->_lcd->blink();
}

/*
============================================================
TITLE : start
INPUT : -
OUTPUT : void
DESC : Routine principale
============================================================
*/
void UIRims::start()
{
	boolean tempSelected = false, waitNone = false;
	byte setPoint = 20, digitPosition = 0;
	while(not tempSelected)
	{
		Serial.println(this->_readKeys());
		switch(this->_readKeys())
		{
			case KEYNONE :
				if(waitNone) waitNone=false;
				break;
			case KEYUP :
				if(not waitNone)
				{
					if((setPoint<99 and digitPosition==0) or 
					   (setPoint<90 and digitPosition==1))
					{
						(digitPosition==0) ? (setPoint++) : (setPoint+=10);
						this->_printIntLCD(setPoint,3,0,4-digitPosition,0);
						waitNone = true;
					}	
				}
				break;
			case KEYDOWN :
				if(not waitNone)
				{
					if((setPoint>0 and digitPosition==0) or 
					   (setPoint>9 and digitPosition==1))
					{
						(digitPosition==0) ? (setPoint--) : (setPoint-=10);
						this->_printIntLCD(setPoint,3,0,4-digitPosition,0);
						waitNone = true;
					}
				}
				break;
			case KEYLEFT :
				if(not waitNone)
					digitPosition = 1;
					this->_lcd->setCursor(3,0);
					waitNone = true;
				break;
			case KEYRIGHT :
				if(not waitNone)
					digitPosition = 0;
					this->_lcd->setCursor(4,0);
					waitNone = true;
				break;
			case KEYSELECT :
				tempSelected = true;
				this->_lcd->noBlink();
				break;
		}
	}
}