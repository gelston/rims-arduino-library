/*
  UIRims.cpp - Définition de UIRims.h
*/

#include "Arduino.h"
#include "UIRims.h"

/*
============================================================
TITLE : UIRims (constructeur)
DESC : Constructeur d'un objet UIRims
============================================================
*/
UIRims::UIRims(LiquidCrystal* lcd,byte col,byte row, byte pinLight,
		   byte pinKeysAnalog)
: _lcd(lcd), _pinKeysAnalog(pinKeysAnalog),
  _cursorCol(0), _cursorRow(0), _pinLight(pinLight)
{
	pinMode(pinLight,OUTPUT);
	digitalWrite(pinLight,HIGH);
	this->_lcd->begin(col,row);
	this->_lcd->clear();
}

/*
============================================================
TITLE : showTempScreen
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
	this->_tempScreenShown = true;
}

/*
============================================================
TITLE : showTimeFlowScreen
DESC : 
============================================================
*/
void UIRims::showTimeFlowScreen()
{
	this->_lcd->clear();
	this->_printStrLCD("time:000m00s",0,0);
	this->_printStrLCD("flow:00.0l/min",0,1);
	this->_tempScreenShown = false;
}

/*
============================================================
TITLE : switchScreen
DESC : 
============================================================
*/
void UIRims::switchScreen()
{
	if(this->_tempScreenShown)
	{
		this->showTimeFlowScreen();
		this->_tempScreenShown = false;
	}
	else
	{
		this->showTempScreen();
		this->_tempScreenShown = true;
	}
}

/*
============================================================
TITLE : getTempScreenShown
DESC : 
============================================================
*/
boolean UIRims::getTempScreenShown()
{
	return this->_tempScreenShown;
}

/*
============================================================
TITLE : readKeysADC
DESC : Récupère le bouton présentement appuyé sans debounce
============================================================
*/
byte UIRims::readKeysADC()
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
TITLE : _waitForKeyChange
DESC : Récupère le bouton présentement appuyé avec debounce
============================================================
*/
byte UIRims::_waitForKeyChange()
{
	boolean keyConfirmed = false, keyDetected = false;
	byte lastKey = this->readKeysADC(), currentKey;
	unsigned long refTime = millis(), currentTime;
	while(not keyConfirmed)
	{
		currentTime = millis();
		currentKey = this->readKeysADC();
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
DESC : 
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
	this->_lcd->setCursor(col,row);
	this->_lcd->print(res);
	this->_lcd->setCursor(this->_cursorCol,this->_cursorRow);
}

/*
============================================================
TITLE : _setCursorPosition
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
DESC : 
============================================================
*/
void UIRims::setTime(unsigned int timeSec)
{
	int minutes = timeSec / 60;
	int seconds = timeSec % 60;
	this->_printFloatLCD(minutes,3,0,5,0);
	this->_printFloatLCD(seconds,2,0,9,0);
}

/*
============================================================
TITLE : setFlow
DESC : 
============================================================
*/
void UIRims::setFlow(int flow)
{
	this->_printFloatLCD(flow,3,1,5,1);
}
/*
============================================================
TITLE : _incDecValue
DESC : 
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
	if(not timeFormat) this->setTempSP(constrainedRes);
	else this->setTime(constrainedRes);
	return constrainedRes;
}

/*
============================================================
TITLE : _moveCursorLR
DESC : 
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
				this->_lcd->noBlink();
				this->_setCursorPosition(0,0);
				break;
		}
	}
	return value;
}

/*
============================================================
TITLE : askSetPoint
DESC : Demande le setPoint à l'utilisateur.setPoint
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
DESC : Demande le temps du pallier à l'utilisateur
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
DESC :
============================================================
*/
void UIRims::showEnd()
{
	unsigned long refTime, currentTime;
	this->_lcd->clear();
	this->_printStrLCD("finished!",0,0);
	refTime = currentTime = millis();
	boolean lightState = true;
	while(true)
	{
		currentTime = millis();
		if(currentTime-refTime>=500)
		{
			refTime = millis();
			lightState = not lightState;
			digitalWrite(this->_pinLight,lightState);
		}
	}
}

/*
============================================================
TITLE : show
DESC :
============================================================
*/
void UIRims::showErrorPV(String mess)
{
	if(mess.length() > 2)
	{
		mess = String(mess).substring(0,2);
	}
	this->_printStrLCD(String(" #")+mess,3,1);
	this->_printStrLCD(String("#")+mess,10,1);
}