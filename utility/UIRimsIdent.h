#ifndef UIRimsIdent_h
#define UIRimsIdent_h


#include "Arduino.h"
#include "UIRims.h"

/*!
 * \brief UIRims specialized for identification toolkit
 * \author Francis Gagnon
 */
class UIRimsIdent : public UIRims
{
	
public:
	
	UIRimsIdent(LiquidCrystal* lcd, byte pinKeysAnalog,
			    byte pinLight=13,int pinSpeaker=-1);

	void showIdentScreen();
	
	void setIdentCV(unsigned long controlValue, unsigned long ssrWindow);
	void setTime(unsigned int timeSec, boolean waitRefresh = true);
};

#endif