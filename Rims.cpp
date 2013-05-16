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
  _flowLastPulseTime(0), _flow(0)
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
	this->_time = this->_uiRims.askTime(DEFAULTTIME);
	this->_uiRims.showTempScreen();
	boolean timePassed = false, waitNone = true,
	        tempScreenShown = true;
	while(not timePassed)
	{
		int val = analogRead(this->_analogPinPV);
		Serial.println(val);
		if(val >= 1023)
		{
			this->_uiRims.showErrorPV("NC");
		}
		else
		{
			this->_uiRims.setTempPV(this->analogInToCelcius(val));
			Serial.println(this->analogInToCelcius(val));
		}
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
	return this->_flow;
}

/*
============================================================
TITLE : _isrFlowSensor
DESC : 
============================================================
*/
void Rims::_isrFlowSensor()
{
	unsigned long currentTime = millis();
	Rims::_rimsPtr->_flow = (currentTime - Rims::_rimsPtr->_flowLastPulseTime) \
							/ (1000.0*4.8);
	Rims::_rimsPtr->_flowLastPulseTime = currentTime;
}