/*************************************************************
 ************************************************************* 
 * Rims
 *     Main library for Rims
 ************************************************************* 
 *************************************************************/


#include "Arduino.h"
#include "math.h"
#include "utility/PID_v1.h"
#include "Rims.h"


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
	if(tauFilter>0)
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
	if(tauFilter>0)
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
void Rims::setInterruptFlow(byte interruptFlow, float flowFactor)
{
	Rims::_rimsPtr = this;
	attachInterrupt(interruptFlow,Rims::_isrFlowSensor,RISING);
	_flowFactor = flowFactor;
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
	unsigned long currentTime ,lastScreenSwitchTime, lastTimePID;
	// === ASK SETPOINT ===
	double rawSetPoint = _ui->askSetPoint(DEFAULTSP);
	*(_setPointPtr) = 0;
	// === ASK TIMER ===
	_settedTime = (unsigned long)_ui->askTime(DEFAULTTIME)*1000;
	// === PUMP SWITCHING WARN ===
	_ui->showPumpWarning();
	while(_ui->readKeysADC()==KEYNONE)
	{
		_ui->setFlow(this->getFlow());
	}
	// === HEATER SWITCHING WARN ===
	_ui->showHeaterWarning();
	while(_ui->readKeysADC()==KEYNONE) continue;
	_ui->showTempScreen();
	*(_processValPtr) = this->getTempPV();
	_ui->setTempSP(rawSetPoint);
	_ui->setTempPV(*(_processValPtr));
	_sumStoppedTime = true;
	_runningTime = _totalStoppedTime = _timerStopTime = 0;
	_myPID.SetMode(AUTOMATIC);
	currentTime =_windowStartTime = _timerStartTime \
				= lastScreenSwitchTime = millis();
	lastTimePID = currentTime - PIDSAMPLETIME;
	while(not timerElapsed)
	{	
		// === READ TEMPERATURE/FLOW ===
		*(_processValPtr) = getTempPV();
		// === SETPOINT FILTERING ===
		currentTime = millis();
		if(currentTime-lastTimePID>=PIDSAMPLETIME)
		{
			*(_setPointPtr) = (1-_setPointFilterCst) * rawSetPoint + \
							   _setPointFilterCst * _lastSetPointFilterOutput;
			_lastSetPointFilterOutput = *(_setPointPtr);
			lastTimePID = currentTime;
		}
		// === PID COMPUTE ===
		pidJustCalculated = _myPID.Compute();
		// === PID FILTERING ===
		if(pidJustCalculated)
		{
			*(_controlValPtr) = (1-_PIDFilterCst) * (*_controlValPtr) + \
								_PIDFilterCst * _lastPIDFilterOutput;
			_lastPIDFilterOutput = *(_controlValPtr);
		}
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
	else flow = (1e06 / (_flowFactor* (_flowCurTime - _flowLastTime)));
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