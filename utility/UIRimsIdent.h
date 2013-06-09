// === UIRimsIdent ================================
// === UI for identification toolkit      =========
// === IdentRims class                    =========
// ================================================


#ifndef UIRimsIdent_h
#define UIRimsIdent_h


#include "Arduino.h"
#include "UIRims.h"


class UIRimsIdent : public UIRims
{
	
public:
	
	UIRimsIdent(LiquidCrystal lcd, byte col, byte row, byte pinLight,
				byte pinKeysAnalog);

	void showIdentScreen();
	
	void setIdentCV(unsigned long controlValue, unsigned long ssrWindow);
	void setTime(unsigned int timeSec, boolean waitRefresh = true);
};

#endif