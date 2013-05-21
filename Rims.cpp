/*
  Rims.cpp - Définition de Rims.h
*/

#include "Arduino.h"
#include "math.h"
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
Rims::Rims(UIRims uiRims, byte analogPinPV, byte interruptFlow)
: _uiRims(uiRims), _analogPinPV(analogPinPV),
  _flowLastTime(0), _flowCurTime(0)
{
	Rims::_rimsPtr = this;
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
	this->_tempSP = this->_uiRims.askSetPoint(DEFAULTSP);
	this->_settedTime = (unsigned long)this->_uiRims.askTime(DEFAULTTIME)*1000;
	this->_uiRims.showTempScreen();
	this->_uiRims.setTempSP(this->_tempSP);
	boolean timePassed = false, waitNone = true;
	unsigned long currentTime, runningTime, remainingTime;
	int curTempADC = 1023;
	this->_startTime = millis();
	while(not timePassed)
	{	
		// === READ TEMPERATURE/FLOW ===
		curTempADC = analogRead(this->_analogPinPV);
		this->_tempPV = this->analogInToCelcius(curTempADC);
		this->_flow = this->getFlow();
		// === TIME REMAINING ===
		currentTime = millis();
		runningTime = currentTime - this->_startTime;
		remainingTime = this->_settedTime-runningTime;
		if(curTempADC >= 1023)
		{
			this->_uiRims.showErrorPV("NC");
		}
		// === REFRESH DISPLAY ===
		this->_uiRims.setTempPV(this->_tempPV);
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
	return (1/invKelvin)-273.15;
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