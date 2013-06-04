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
Rims::Rims(UIRims uiRims, byte analogPinTherm, byte ssrPin, 
	       double* currentTemp, double* ssrControl, double* settedTemp)
: _uiRims(uiRims), _analogPinPV(analogPinTherm), _pinCV(ssrPin),
  _processValPtr(currentTemp), _controlValPtr(ssrControl), _setPointPtr(settedTemp),
  _myPID(currentTemp, ssrControl, settedTemp, 0, 0, 0, DIRECT),
  _pinLED(13), _filterCst(0),
  _flowLastTime(0), _flowCurTime(0)
{
	this->_myPID.SetSampleTime(PIDSAMPLETIME);
	this->_myPID.SetOutputLimits(0,SSRWINDOWSIZE);
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
void Rims::setPinLED(byte pinLED)
{
	pinMode(pinLED,OUTPUT);
	this->_pinLED = pinLED;
}

/*
============================================================
TITLE : start
DESC : Routine principale
============================================================
*/
void Rims::start()
{
	boolean timerElapsed = false;
	unsigned long windowStartTime, lastScreenSwitchTime;
	// === ASK SETPOINT ===
	*(this->_setPointPtr) = this->_uiRims.askSetPoint(DEFAULTSP);
	// === ASK TIMER ===
	this->_settedTime = (unsigned long)this->_uiRims.askTime(DEFAULTTIME)*1000;
	// === PUMP SWITCHING ===
	this->_uiRims.showPumpWarning();
	while(this->_uiRims.readKeysADC()==KEYNONE)
	{
		this->_uiRims.setFlow(this->getFlow());
	}
	// === HEATER SWITCHING ===
	this->_uiRims.showHeaterWarning();
	while(this->_uiRims.readKeysADC()==KEYNONE) continue;
	
	this->_uiRims.showTempScreen();
	this->_uiRims.setTempSP(*(this->_setPointPtr));
	*(this->_processValPtr) = this->getTempPV();
	this->_uiRims.setTempPV(*(this->_processValPtr));

	this->_sumStoppedTime = true;
	this->_runningTime = this->_totalStoppedTime = this->_timerStopTime = 0;
	this->_myPID.SetMode(AUTOMATIC);
	this->_windowStartTime = this->_timerStartTime = millis();
	while(not timerElapsed)
	{	
		// === READ TEMPERATURE/FLOW ===
		*(this->_processValPtr) = this->getTempPV();
		// === PID COMPUTE ===
		this->_myPID.Compute();
		// === PID FILTERING ===
		if(_filterCst != 0)
		{
			*(_controlValPtr) = (1-_filterCst)*(*_controlValPtr) + \
							_filterCst * _lastFilterOutput;
			_lastFilterOutput = *(_controlValPtr);
		}
		// === SSR CONTROL ===
		this->_refreshSSR();
		// === TIME REMAINING ===
		this->_refreshTimer();
		// === REFRESH DISPLAY ===
		this->_refreshDisplay();
		// === KEY CHECK ===
		if(this->_uiRims.readKeysADC() != KEYNONE and \
		   millis() - lastScreenSwitchTime >= 500)
		{
			this->_uiRims.switchScreen();
			lastScreenSwitchTime = millis();
		}
		if(this->_runningTime >= this->_settedTime) timerElapsed = true;
	}
	this->_myPID.SetMode(MANUAL);
	*(this->_controlValPtr) = 0;
	this->_refreshSSR();
	this->_uiRims.showEnd();
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
	if(abs(*(this->_setPointPtr) - *(this->_processValPtr)) <= DELTATEMPOK)
	{
		if(this->_sumStoppedTime)
		{
			this->_sumStoppedTime = false;
			this->_totalStoppedTime += (this->_timerStartTime - 
										this->_timerStopTime);
		}
		this->_runningTime = currentTime - this->_totalStoppedTime;
		this->_timerStopTime = currentTime;
	}
	else
	{
		this->_timerStartTime = currentTime;
		if(not this->_sumStoppedTime) this->_sumStoppedTime = true;
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
	if(analogRead(this->_analogPinPV) >= 1023) this->_uiRims.showErrorPV("NC");
	else this->_uiRims.setTempPV(*(this->_processValPtr));
	this->_uiRims.setTime((this->_settedTime-this->_runningTime)/1000);
	this->_uiRims.setFlow(this->_flow);
		
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
	if(currentTime - this->_windowStartTime > SSRWINDOWSIZE)
	{
		this->_windowStartTime += SSRWINDOWSIZE;
	}
	if(currentTime - this->_windowStartTime <= *(this->_controlValPtr))
	{
		digitalWrite(this->_pinCV,HIGH);
		digitalWrite(this->_pinLED,HIGH);
	}
	else
	{
		digitalWrite(this->_pinCV,LOW);
		digitalWrite(this->_pinLED,LOW);
	}
}

/*
============================================================
TITLE : analogInToCelcius
DESC : Steinhart-hart thermistor equation with a voltage
       divider with RES1
============================================================
*/
double Rims::analogInToCelcius(int analogIn)
{

}

double Rims::getTempPV()
{
	double tempPV = 0;
	int curTempADC = analogRead(this->_analogPinPV);
	if(curTempADC >= 1023)
	{
		if(this->_myPID.GetMode()==AUTOMATIC)
		{
			this->_myPID.SetMode(MANUAL);
			*(this->_controlValPtr) = 0;
		}
	}
	else
	{
		if(this->_myPID.GetMode()==MANUAL) this->_myPID.SetMode(AUTOMATIC);
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
	if(this->_flowCurTime == 0) flow = 0.0;
	else if(micros() - this->_flowCurTime >= 5e06) flow = 0.0;
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



/*************************************************************
 ************************************************************* 
 * IdentRims
 *     Identification tools for Rims library 
 ************************************************************* 
 *************************************************************/


IdentRims::IdentRims(UIRims uiRims, byte analogPinTherm, byte ssrPin, 
			double* currentTemp, double* ssrControl, double* settedTemp)
: Rims(uiRims, analogPinTherm, ssrPin, currentTemp, ssrControl, settedTemp)
{
}

void IdentRims::startIdent()
{
	unsigned long startTime, currentTime, relativeTime;
	Serial.begin(9600);
	startTime = currentTime = this->_windowStartTime = millis();
	while(millis() - startTime <= 900000) // 15 minutes
	{
		*(this->_processValPtr) = this->analogInToCelcius(this->_analogPinPV);
		currentTime = millis();
		relativeTime = currentTime - startTime;
		if(relativeTime >= 600000)
		{
			*(this->_controlValPtr) = 0.75 * SSRWINDOWSIZE;
		}
		else if(relativeTime >= 300000)
		{
			*(this->_controlValPtr) = SSRWINDOWSIZE;
		}
		else
		{
			*(this->_controlValPtr) = 0.5*SSRWINDOWSIZE;
		}
		this->_refreshSSR();
		Serial.print((double)relativeTime/1000.0,3);
		Serial.print(",");
		Serial.print(*(this->_controlValPtr),0);
		Serial.print(",");
		Serial.println(*(this->_processValPtr),15);
	}
}