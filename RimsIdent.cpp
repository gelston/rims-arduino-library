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
 * -# 0-STEP1VALUE% at t = STEP1TIME sec
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
void RimsIdent::run()
{
	Rims::run();
}

/*!
 * \brief Initialize RimsIdent before iteration
 */
void RimsIdent::_initialize()
{
	unsigned long currentTime;
	// === PUMP SWITCHING ===
	_ui->showPumpWarning();
	while(_ui->readKeysADC()==KEYNONE)
	{
		_ui->setFlow(this->getFlow());
	}
	// === HEATER SWITCHING ===
	_ui->showHeaterWarning();
	while(_ui->readKeysADC()==KEYNONE) continue;
	_rimsInitialized = true;
	// === IDENTIFICATION TESTS ===
	Serial.begin(9600);
	Serial.println("time,cv,pv,flow");
	_ui->showIdentScreen();
	_settedTime = IDENTLENGTH; // 15 minutes
	_sumStoppedTime = false;
	currentTime = millis();
	_totalStoppedTime = _windowStartTime = currentTime;
	_runningTime = 0;
	_lastTimeSerial = currentTime - IDENTSAMPLETIME;
}

/*!
 * \brief Iteration for RimsIdent instance
 */
void RimsIdent::_iterate()
{
	_refreshTimer(false);
	if(_runningTime >= STEP3TIME) *(_controlValPtr) = STEP3VALUE;
	else if(_runningTime >= STEP2TIME) *(_controlValPtr) = STEP2VALUE;
	else if(_runningTime >= STEP1TIME) *(_controlValPtr) = STEP1VALUE;
	_refreshSSR();
	if(_currentTime - _lastTimeSerial >= IDENTSAMPLETIME)
	{
		///  \todo : better handling of unconnected thermistor
		///  \todo : use std screen of UIRims
		*(_processValPtr) = this->getTempPV();
		_flow = this->getFlow();
		Serial.print((double)_runningTime/1000.0,3);
		Serial.print(",");
		Serial.print(*(_controlValPtr),0);
		Serial.print(",");
		Serial.print(*(_processValPtr),15);
		Serial.print(",");
		Serial.println(_flow,2);
		_ui->setIdentCV(*(_controlValPtr),SSRWINDOWSIZE);
		_refreshDisplay();
		_lastTimeSerial = _currentTime;
	}
	if(_runningTime >= _settedTime) _timerElapsed = true;
}