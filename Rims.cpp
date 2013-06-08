/*
  Rims.cpp - Rims.h definition
*/

#include "Arduino.h"
#include "math.h"
#include "utility/PID_v1.h"
#include "Rims.h"

/*************************************************************
 ************************************************************* 
 * Rims
 *     Main library for Rims
 ************************************************************* 
 *************************************************************/


/* 
============================================================
static member _rimsPtr definition
============================================================
*/

Rims* Rims::_rimsPtr = 0;

/*
============================================================
TITLE : Rims (constructor)
DESC : Rims constructor
============================================================
*/
Rims::Rims(UIRims* uiRims, byte analogPinTherm, byte ssrPin, 
	       double* currentTemp, double* ssrControl, double* settedTemp)
: _ui(uiRims), _analogPinPV(analogPinTherm), _pinCV(ssrPin),
  _processValPtr(currentTemp), _controlValPtr(ssrControl), _setPointPtr(settedTemp),
  _myPID(currentTemp, ssrControl, settedTemp, 0, 0, 0, DIRECT),
  _pinLED(13), _PIDFilterCst(0),
  _flowLastTime(0), _flowCurTime(0)
{
	_myPID.SetSampleTime(PIDSAMPLETIME);
	_myPID.SetOutputLimits(0,SSRWINDOWSIZE);
	pinMode(13,OUTPUT);
}

/*
============================================================
TITLE : setTunningPID
DESC : 
============================================================
*/
void Rims::setTunningPID(double Kp, double Ki, double Kd, double tauFilter)
{
	_myPID.SetTunings(Kp,Ki,Kd);
	if(tauFilter>=0)
	{
		_PIDFilterCst = exp((-1.0)*PIDSAMPLETIME/(tauFilter*1000.0));
	}
	else _PIDFilterCst = 0;	
	_lastPIDFilterOutput = 0;
}

/*
============================================================
TITLE : setFilterSetPoint
DESC : 
============================================================
*/
void Rims::setSetPointFilter(double tauFilter)
{
	if(tauFilter>=0)
	{
		_setPointFilterCst = exp((-1.0)*PIDSAMPLETIME/(tauFilter*1000.0));
	}
	else _setPointFilterCst = 0;
	_lastSetPointFilterOutput = 0;
}

/*
============================================================
TITLE : setInterruptFlow
DESC : 
============================================================
*/
void Rims::setInterruptFlow(byte interruptFlow)
{
	Rims::_rimsPtr = this;
	attachInterrupt(interruptFlow,Rims::_isrFlowSensor,RISING);
}

/*
============================================================
TITLE : setInterruptFlow
DESC : 
============================================================
*/
void Rims::setPinLED(byte pinLED)
{
	pinMode(pinLED,OUTPUT);
	_pinLED = pinLED;
}

/*
============================================================
TITLE : start
DESC : Routine principale
============================================================
*/
void Rims::start()
{
	boolean timerElapsed = false, pidJustCalculated = true;
	unsigned long lastScreenSwitchTime;
	// === ASK SETPOINT ===
	*(_setPointPtr) = _ui->askSetPoint(DEFAULTSP);
	// === ASK TIMER ===
	_settedTime = (unsigned long)_ui->askTime(DEFAULTTIME)*1000;
	// === PUMP SWITCHING ===
	_ui->showPumpWarning();
	while(_ui->readKeysADC()==KEYNONE)
	{
		_ui->setFlow(this->getFlow());
	}
	// === HEATER SWITCHING ===
	_ui->showHeaterWarning();
	while(_ui->readKeysADC()==KEYNONE) continue;
	
	_ui->showTempScreen();
	_ui->setTempSP(*(_setPointPtr));
	*(_processValPtr) = this->getTempPV();
	_ui->setTempPV(*(_processValPtr));

	_sumStoppedTime = true;
	_runningTime = _totalStoppedTime = _timerStopTime = 0;
	_myPID.SetMode(AUTOMATIC);
	_windowStartTime = _timerStartTime \
						   = lastScreenSwitchTime = millis();
	while(not timerElapsed)
	{	
		// === READ TEMPERATURE/FLOW ===
		*(_processValPtr) = getTempPV();
		// === FILTER SETPOINT ===
		// TODO :
		// le calcul est décalé d'une loop, i.e. le calcul du
		// PID se fait avec l'ancienne valeur de la sortie du filtre...
// 		if(_setPointFilterCst !=0 and pidJustCalculated)
// 		{
// 			*(_setPointPtr) = (1-_setPointFilterCst) * (*_setPointPtr) + \
// 							   _setPointFilterCst * _lastSetPointFilterOutput;
// 			_lastSetPointFilterOutput = *(_setPointPtr);
// 		}
		if(pidJustCalculated) Serial.println(*_setPointPtr);
		// === PID COMPUTE ===
		pidJustCalculated = _myPID.Compute();
		// === PID FILTERING ===
		if(_PIDFilterCst != 0 and pidJustCalculated)
		{
			*(_controlValPtr) = (1-_PIDFilterCst) * (*_controlValPtr) + \
								_PIDFilterCst * _lastPIDFilterOutput;
			_lastPIDFilterOutput = *(_controlValPtr);
		}
		if(pidJustCalculated) Serial.println(*(_controlValPtr));
		// === SSR CONTROL ===
		_refreshSSR();
		// === TIME REMAINING ===
		_refreshTimer();
		// === REFRESH DISPLAY ===
		_refreshDisplay();
		// === KEY CHECK ===
		if(_ui->readKeysADC() != KEYNONE and \
		   millis() - lastScreenSwitchTime >= 500)
		{
			_ui->switchScreen();
			lastScreenSwitchTime = millis();
		}
		if(_runningTime >= _settedTime) timerElapsed = true;
	}
	_myPID.SetMode(MANUAL);
	*(_controlValPtr) = 0;
	_refreshSSR();
	_ui->showEnd();
}

/*
============================================================
TITLE : _refreshTimer
DESC : 
============================================================
*/
void Rims::_refreshTimer()
{
	unsigned long currentTime = millis();
	if(abs(*(_setPointPtr) - *(_processValPtr)) <= MAXTEMPVAR)
	{
		if(_sumStoppedTime)
		{
			_sumStoppedTime = false;
			_totalStoppedTime += (_timerStartTime - 
										_timerStopTime);
		}
		_runningTime = currentTime - _totalStoppedTime;
		_timerStopTime = currentTime;
	}
	else
	{
		_timerStartTime = currentTime;
		if(not _sumStoppedTime) _sumStoppedTime = true;
	}
}

/*
============================================================
TITLE : _refreshDisplay
DESC : 
============================================================
*/
void Rims::_refreshDisplay()
{
	if(analogRead(_analogPinPV) >= 1023) _ui->showErrorPV("NC");
	else _ui->setTempPV(*(_processValPtr));
	_ui->setTime((_settedTime-_runningTime)/1000);
	_ui->setFlow(_flow);
		
}

/*
============================================================
TITLE : _refreshSSR
DESC : 
============================================================
*/
void Rims::_refreshSSR()
{
	unsigned long currentTime = millis();
	if(currentTime - _windowStartTime > SSRWINDOWSIZE)
	{
		_windowStartTime += SSRWINDOWSIZE;
	}
	if(currentTime - _windowStartTime <= *(_controlValPtr))
	{
		digitalWrite(_pinCV,HIGH);
		digitalWrite(_pinLED,HIGH);
	}
	else
	{
		digitalWrite(_pinCV,LOW);
		digitalWrite(_pinLED,LOW);
	}
}

/*
============================================================
TITLE : getTempPV
DESC : Get process value temperature from _analogPinPV.
       It uses Steinhart-hart thermistor equation with a voltage
       divider with RES1
============================================================
*/
double Rims::getTempPV()
{
	double tempPV = 0;
	int curTempADC = analogRead(_analogPinPV);
	if(curTempADC >= 1023)
	{
		if(_myPID.GetMode()==AUTOMATIC)
		{
			_myPID.SetMode(MANUAL);
			*(_controlValPtr) = 0;
		}
	}
	else
	{
		if(_myPID.GetMode()==MANUAL) _myPID.SetMode(AUTOMATIC);
		double vin = ((double)curTempADC*VALIM)/1024.0;
		double resTherm = (RES1*vin)/(VALIM-vin);
		double logResTherm = log(resTherm);
		double invKelvin = STEINHART0+\
						STEINHART1*logResTherm+\
						STEINHART2*pow(logResTherm,2)+\
						STEINHART3*pow(logResTherm,3);
		tempPV = (1/invKelvin)-273.15+FINETUNETEMP;
	}
	return tempPV;
}

/*
============================================================
TITLE : getFlow
DESC : 
============================================================
*/
float Rims::getFlow()
{
	float flow;
	if(_flowCurTime == 0) flow = 0.0;
	else if(micros() - _flowCurTime >= 5e06) flow = 0.0;
	else
	{
		flow = (1e06 / (4.8* \ 
		(_flowCurTime - _flowLastTime)));
	}
	return flow;
}


/*
============================================================
TITLE : _isrFlowSensor
DESC : 
============================================================
*/
void Rims::_isrFlowSensor()
{
	Rims::_rimsPtr->_flowLastTime = Rims::_rimsPtr->_flowCurTime;
	Rims::_rimsPtr->_flowCurTime = micros();
}



/*************************************************************
 ************************************************************* 
 * RimsIdent
 *     Identification tools for Rims library 
 ************************************************************* 
 *************************************************************/


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