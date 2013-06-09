/*************************************************************
 ************************************************************* 
 * RimsIdent
 *     Identification tools for Rims library 
 ************************************************************* 
 *************************************************************/

#include "RimsIdent.h"

RimsIdent::RimsIdent(UIRimsIdent* uiRimsIdent, byte analogPinTherm, 
					 byte ssrPin, double* currentTemp, double* ssrControl, 
					 double* settedTemp)
: Rims(uiRimsIdent, analogPinTherm, ssrPin, 
	   currentTemp, ssrControl, settedTemp),
 _ui(uiRimsIdent)
{
}

void RimsIdent::startIdent()
{
	// === PUMP SWITCHING ===
	_ui->showPumpWarning();
	while(_ui->readKeysADC()==KEYNONE)
	{
		_ui->setFlow(this->getFlow());
	}
	// === HEATER SWITCHING ===
	_ui->showHeaterWarning();
	while(_ui->readKeysADC()==KEYNONE) continue;
	// === IDENTIFICATION TESTS ===
	_ui->showIdentScreen();
	Serial.begin(9600);
	_settedTime = 600000;
	_totalStoppedTime = _windowStartTime = millis();
	_runningTime = 0;
	Serial.println("time,cv,pv");
	while(_runningTime <= _settedTime) // 15 minutes
	{
		*(_processValPtr) = this->getTempPV();
		_ui->setTempPV(*(_processValPtr));
		_runningTime = millis() - _totalStoppedTime;
		_ui->setTime((_settedTime-_runningTime)/1000);
		if(_runningTime >= 240000)
		{
			*(_controlValPtr) = 0;
			_ui->setIdentCV(0,SSRWINDOWSIZE);
		}
		else if(_runningTime >= 120000)
		{
			*(_controlValPtr) = SSRWINDOWSIZE;
			_ui->setIdentCV(SSRWINDOWSIZE,SSRWINDOWSIZE);
		}
		else
		{
			*(_controlValPtr) = 0.5*SSRWINDOWSIZE;
			_ui->setIdentCV(0.5 * SSRWINDOWSIZE, SSRWINDOWSIZE);
		}
		_refreshSSR();
		Serial.print((double)_runningTime/1000.0,3);
		Serial.print(",");
		Serial.print(*(_controlValPtr),0);
		Serial.print(",");
		Serial.println(*(_processValPtr),15);
	}
	_ui->showEnd();
}