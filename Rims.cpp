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
Rims::Rims(UIRims uiRims, byte analogPinTherm, byte ssrPin, 
	       double* currentTemp, double* ssrControl, double* settedTemp)
: _uiRims(uiRims), _analogPinPV(analogPinTherm), _pinCV(ssrPin),
  _processValPtr(currentTemp), _controlValPtr(ssrControl), _setPointPtr(settedTemp),
  _myPID(currentTemp, ssrControl, settedTemp, 0, 0, 0, DIRECT),
  _ledPin(13), _filterCst(0),
  _flowLastTime(0), _flowCurTime(0)
{
	_myPID.SetSampleTime(PIDSAMPLETIME);
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
	this->_myPID.SetTunings(Kp,Ki,Kd);
	if(tauFilter>=0)
	{
		_filterCst = exp((-1.0)*PIDSAMPLETIME/(tauFilter*1000.0));
		_lastFilterOutput = 0;
	}
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
void Rims::setLedPin(byte ledPin)
{
	pinMode(ledPin,OUTPUT);
	this->_ledPin = ledPin;
}

/*
============================================================
TITLE : start
DESC : Routine principale
============================================================
*/
void Rims::start()
{
	*(this->_setPointPtr) = this->_uiRims.askSetPoint(DEFAULTSP);
	this->_settedTime = (unsigned long)this->_uiRims.askTime(DEFAULTTIME)*1000;
	this->_uiRims.showTempScreen();
	this->_uiRims.setTempSP(*(this->_setPointPtr));
	int curTempADC = analogRead(this->_analogPinPV);
	*(this->_processValPtr) = this->analogInToCelcius(curTempADC);
	this->_uiRims.setTempPV(*(this->_processValPtr));
	boolean timePassed = false, waitNone = true;
	unsigned long currentTime, runningTime, remainingTime;
	this->_startTime = millis();
	while(not timePassed)
	{	
		// === READ TEMPERATURE/FLOW ===
		curTempADC = analogRead(this->_analogPinPV);
		*(this->_processValPtr) = this->analogInToCelcius(curTempADC);
		this->_flow = this->getFlow();
		// === PID COMPUTE ===
		this->_myPID.Compute();
		// === PID FILTERING ===
		if(_filterCst != 0)
		{
			*(_controlValPtr) = (1-_filterCst)*(*_controlValPtr) + \
							_filterCst * _lastFilterOutput;
			_lastFilterOutput = *(_controlValPtr);
		}
		// === TIME REMAINING ===
		currentTime = millis();
		runningTime = currentTime - this->_startTime;
		remainingTime = this->_settedTime-runningTime;
		// === REFRESH DISPLAY ===
		if(curTempADC >= 1023)
		{
			this->_uiRims.showErrorPV("NC");
		}
		this->_uiRims.setTempPV(*(this->_processValPtr));
		this->_uiRims.setTime(remainingTime/1000);
		this->_uiRims.setFlow(this->_flow);
		// === KEY CHECK ===
		if(waitNone)
		{
			if(this->_uiRims.readKeysADC() == KEYNONE) waitNone = false;
		}
		else
		{
			if(this->_uiRims.readKeysADC() != KEYNONE)
			{
				this->_uiRims.switchScreen();
				waitNone = true;
			}
		}
		if(runningTime >= this->_settedTime) timePassed = true;
	}
	this->_uiRims.showEnd();
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
	if(this->_flowCurTime == 0)
	{
		flow = 0.0;
	}
	else if(micros() - this->_flowCurTime >= 5e06)
	{
		flow = 0.0;
	}
	else
	{
		flow = (1e06 / (4.8* \ 
		(this->_flowCurTime - this->_flowLastTime)));
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