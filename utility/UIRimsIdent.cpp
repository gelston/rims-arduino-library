#include "Arduino.h"
#include "UIRimsIdent.h"


/*!
 * \brief Constructor
 * \param lcd : LiquidCrystal* (16x2 characters) that will be used during
                identification.
 * \param pinKeysAnalog : byte analog pin for thermistor reading
 * \param pinLight : byte pin used for heater LED indicator
 * \param pinSpeaker : byte pin used for buzzer alarm
 */
UIRimsIdent::UIRimsIdent(LiquidCrystal* lcd, 
						 byte pinKeysAnalog,byte pinLight,int pinSpeaker)
: UIRims(lcd,pinKeysAnalog,pinLight,pinSpeaker)
{
}
/*
============================================================
TITLE : showIdentScreen
DESC : Show system identification screen on _lcd
============================================================
*/
void UIRimsIdent::showIdentScreen()
{
	_printStrLCD("000% 000m00s    ",0,0);
	_printStrLCD("PV:00.0\xdf""C(000\xdf""F)",0,1);
	_tempScreenShown = true;
}

/*
============================================================
TITLE : setIdentCV
DESC : Set ident CV on identScreen
============================================================
*/
void UIRimsIdent::setIdentCV(unsigned long controlValue, 
							 unsigned long ssrWindow)
{
	_printFloatLCD(controlValue*100/ssrWindow,3,0,0,0);
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
void UIRimsIdent::setTime(unsigned int timeSec, boolean waitRefresh)
{
	_tempScreenShown = false;
	UIRims::setTime(timeSec,waitRefresh);
	_tempScreenShown = true;
}