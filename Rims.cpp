/*
  Rims.cpp - Définition de Rims.h
*/

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
TITLE : Rims (constructeur)
DESC : Constructeur d'un objet Rims
============================================================
*/
Rims::Rims(UIRims uiRims, byte analogPinPV, byte interruptFlow,
	       byte ssrPin, byte ledPin, 
		   PID myPID)
: _myPID(myPID),
  _uiRims(uiRims), _analogPinPV(analogPinPV),
  _ssrPin(ssrPin), _ledPin(ledPin),
  _flowLastTime(0), _flowCurTime(0)
{
	Rims::_rimsPtr = this;
	_myPID.SetSampleTime(PIDSAMPLETIME);
	attachInterrupt(interruptFlow,Rims::_isrFlowSensor,RISING);
}

/*
============================================================
TITLE : start
DESC : Routine principale
============================================================
*/
void Rims::start()
{
	*(_tempSP) = _uiRims.askSetPoint(DEFAULTSP);
	_settedTime = (unsigned long)_uiRims.askTime(DEFAULTTIME)*1000;
	_uiRims.showTempScreen();
	_uiRims.setTempSP(*_tempSP);
	int curTempADC = analogRead(_analogPinPV);
	*(_tempPV) = this->analogInToCelcius(curTempADC);
	_uiRims.setTempPV(*_tempPV);
	boolean timePassed = false, waitNone = true;
	unsigned long currentTime, runningTime, remainingTime;
	_startTime = millis();
	while(not timePassed)
	{	
		// === READ TEMPERATURE/FLOW ===
		curTempADC = analogRead(_analogPinPV);
		*(_tempPV) = this->analogInToCelcius(curTempADC);
		_flow = this->getFlow();
		// === PID COMPUTE ===
		// === PID FILTERING ===
		if(_filterCst != 0)
		{
			*_controlValue = (1-_filterCst)*(*_controlValue) + \
							_filterCst * _lastFilterOutput;
			_lastFilterOutput = *_controlValue;
		}
		// === TIME REMAINING ===
		currentTime = millis();
		runningTime = currentTime - _startTime;
		remainingTime = _settedTime-runningTime;
		if(curTempADC >= 1023)
		{
			_uiRims.showErrorPV("NC");
		}
		// === REFRESH DISPLAY ===
		_uiRims.setTempPV(*_tempPV);
		_uiRims.setTime(remainingTime/1000);
		_uiRims.setFlow(_flow);
		// === KEY CHECK ===
		if(waitNone)
		{
			if(_uiRims.readKeysADC() == KEYNONE) waitNone = false;
		}
		else
		{
			if(_uiRims.readKeysADC() != KEYNONE)
			{
				_uiRims.switchScreen();
				waitNone = true;
			}
		}
		if(runningTime >= _settedTime) timePassed = true;
	}
	_uiRims.showEnd();
}

/*
============================================================
TITLE : analogInToCelcius
DESC : Steinhart-hart thermistor equation with a voltage
       divider with RES1
============================================================
*/
float Rims::analogInToCelcius(int analogIn)
{
	float vin = ((float)analogIn*VALIM)/(float)1024;
	float resTherm = (RES1*vin)/(VALIM-vin);
	float logResTherm = log(resTherm);
	float invKelvin = STEINHART0+\
	                  STEINHART1*logResTherm+\
	                  STEINHART2*pow(logResTherm,2)+\
	                  STEINHART3*pow(logResTherm,3);
	return (1/invKelvin)-273.15+FINETUNETEMP;
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
	if(_flowCurTime == 0)
	{
		flow = 0.0;
	}
	else if(micros() - _flowCurTime >= 5e06)
	{
		flow = 0.0;
	}
	else
	{
		flow = (1e06 / (4.8* \ 
		(_flowCurTime - _flowLastTime)));
	}
	return flow;
}

/*
============================================================
TITLE : getPID
DESC : 
============================================================
*/
PID Rims::getPID()
{
	return _myPID;
}

/*
============================================================
TITLE : setPIDFilter
DESC : 
============================================================
*/
void Rims::setPIDFilter(double tauFilter)
{
	if(tauFilter>=0)
	{
		_filterCst = exp((-1.0)*PIDSAMPLETIME/(tauFilter*1000.0));
		_lastFilterOutput = 0;
	}
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