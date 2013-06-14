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
  _rimsInitialized(false),
  _pinLED(13), _PIDFilterCst(0),
  _flowLastTime(0), _flowCurTime(0)
{
	_steinhartCoefs[0] = 0.001; _steinhartCoefs[1] = 0.0002;
	_steinhartCoefs[2] = -4e-7; _steinhartCoefs[3] = 1e-7;
	_res1 = 10000;
	_fineTuneTemp = 0;
	_myPID.SetSampleTime(PIDSAMPLETIME);
	_myPID.SetOutputLimits(0,SSRWINDOWSIZE);
	pinMode(ssrPin,OUTPUT);
	pinMode(13,OUTPUT);
}

/*
============================================================
TITLE : setThermistor
DESC : 
============================================================
*/
void Rims::setThermistor(float steinhartCoefs[], float res1, float fineTuneTemp)
{
	for(int i=0;i<=4;i++) _steinhartCoefs[i] = steinhartCoefs[i];
	_res1 = res1;
	_fineTuneTemp = fineTuneTemp;
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
void Rims::run()
{
	if(not _rimsInitialized) this->_initialize();
	else
	{
		if(not _timerElapsed) this->_iterate();
		else
		{
			_myPID.SetMode(MANUAL);
			*(_controlValPtr) = 0;
			_refreshSSR();
			_rimsInitialized = false;
			_ui->showEnd();
		}
	}
}

/*
============================================================
TITLE : _initialize
DESC : 
============================================================
*/
void Rims::_initialize()
{
	_timerElapsed = false, _pidJustCalculated = true;
	// === ASK SETPOINT ===
	_rawSetPoint = _ui->askSetPoint(DEFAULTSP);
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
	_ui->setTempSP(_rawSetPoint);
	_ui->setTempPV(*(_processValPtr));
	_sumStoppedTime = true;
	_runningTime = _totalStoppedTime = _timerStopTime = 0;
	_myPID.SetMode(AUTOMATIC);
	_rimsInitialized = true;
	_windowStartTime = _timerStartTime \
					 = _lastScreenSwitchTime = millis();
	_lastTimePID = _timerStartTime - PIDSAMPLETIME;
}

/*
============================================================
TITLE : _iterate
DESC : 
============================================================
*/
void Rims::_iterate()
{
	unsigned long currentTime = millis();
	// === READ TEMPERATURE/FLOW ===
	*(_processValPtr) = getTempPV();
	// === SETPOINT FILTERING ===
	currentTime = millis();
	if(currentTime-_lastTimePID>=PIDSAMPLETIME)
	{
		*(_setPointPtr) = (1-_setPointFilterCst) * _rawSetPoint + \
						_setPointFilterCst * _lastSetPointFilterOutput;
		_lastSetPointFilterOutput = *(_setPointPtr);
		_lastTimePID = currentTime;
	}
	// === PID COMPUTE ===
	_pidJustCalculated = _myPID.Compute();
	// === PID FILTERING ===
	if(_pidJustCalculated)
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
	millis() - _lastScreenSwitchTime >= 500)
	{
		_ui->switchScreen();
		_lastScreenSwitchTime = millis();
	}
	if(_runningTime >= _settedTime) _timerElapsed = true;
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
       divider with _res1
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
		double resTherm = (_res1*vin)/(VALIM-vin);
		double logResTherm = log(resTherm);
		double invKelvin = _steinhartCoefs[0]+\
						_steinhartCoefs[1]*logResTherm+\
						_steinhartCoefs[2]*pow(logResTherm,2)+\
						_steinhartCoefs[3]*pow(logResTherm,3);
		tempPV = (1/invKelvin)-273.15+_fineTuneTemp;
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