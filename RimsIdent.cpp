/*!
 * \file RimsIdent.cpp
 * \brief RimsIdent class definition
 */

#include "Arduino.h"
#include "RimsIdent.h"

/*!
 * \brief Consructor
 * \param uiRimsIdent : unsigned long. current CV between 0 to ssrWindow
 * \param analogPinTherm : unsigned long. ssrWindow size in mSec.
 * \param ssrPin : byte. Pin to control heater's solid state relay.
 * \param currentTemp : double*. Pointer to a double that 
 *                      will be used for current temparature
 * \param ssrControl : double*. Pointer to a double that 
 *                     will use to control SSR
 * \param settedTemp : double*. Pointer to a double that 
 *                     will be use to store setted temperature
 */
RimsIdent::RimsIdent(UIRimsIdent* uiRimsIdent, byte analogPinTherm, 
					 byte ssrPin, double* currentTemp, double* ssrControl, 
					 double* settedTemp)
: Rims(uiRimsIdent, analogPinTherm, ssrPin, 
	   currentTemp, ssrControl, settedTemp),
 _ui(uiRimsIdent)
{
}

/*!
 * \brief Start identification procedure
 *
 * It will last 30 minutes. Step order is :
 * -# 0-50% at t = 0 sec
 * -# 50%-100% at t = 10 min
 * -# 100%-0% at t = 20 min
 * -# end at t = 30 min
 *
 * All information for process identification 
 * is printed in Serial monitor on Arduino IDE.
 *
 * I won't explain here how to identify process and how to tune PID with
 * that information but there's a lot on information here :
 * http://www.controlguru.com/wp/p87.html
 *
 */
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
	_settedTime = 1800000;
	_totalStoppedTime = _windowStartTime = millis();
	_runningTime = 0;
	Serial.println("time,cv,pv");
	while(_runningTime <= _settedTime) // 15 minutes
	{
		*(_processValPtr) = this->getTempPV();
		_ui->setTempPV(*(_processValPtr));
		_runningTime = millis() - _totalStoppedTime;
		_ui->setTime((_settedTime-_runningTime)/1000);
		if(_runningTime >= 1200000)
		{
			*(_controlValPtr) = 0;
			_ui->setIdentCV(0,SSRWINDOWSIZE);
		}
		else if(_runningTime >= 600000)
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