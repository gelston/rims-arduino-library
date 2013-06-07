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
		this->_PIDFilterCst = exp((-1.0)*PIDSAMPLETIME/(tauFilter*1000.0));
	}
	else this->_PIDFilterCst = 0;	
	this->_lastPIDFilterOutput = 0;
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
		this->_setPointFilterCst = exp((-1.0)*PIDSAMPLETIME/(tauFilter*1000.0));
	}
	else this->_setPointFilterCst = 0;
	this->_lastSetPointFilterOutput = 0;
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
	boolean timerElapsed = false, pidJustCalculated = true;
	unsigned long lastScreenSwitchTime;
	// === ASK SETPOINT ===
	*(this->_setPointPtr) = this->_ui->askSetPoint(DEFAULTSP);
	Serial.println(*_setPointPtr);
	// === ASK TIMER ===
	this->_settedTime = (unsigned long)this->_ui->askTime(DEFAULTTIME)*1000;
	// === PUMP SWITCHING ===
	this->_ui->showPumpWarning();
	while(this->_ui->readKeysADC()==KEYNONE)
	{
		this->_ui->setFlow(this->getFlow());
	}
	// === HEATER SWITCHING ===
	this->_ui->showHeaterWarning();
	while(this->_ui->readKeysADC()==KEYNONE) continue;
	
	this->_ui->showTempScreen();
	this->_ui->setTempSP(*(this->_setPointPtr));
	*(this->_processValPtr) = this->getTempPV();
	this->_ui->setTempPV(*(this->_processValPtr));

	this->_sumStoppedTime = true;
	this->_runningTime = this->_totalStoppedTime = this->_timerStopTime = 0;
	this->_myPID.SetMode(AUTOMATIC);
	this->_windowStartTime = this->_timerStartTime \
						   = lastScreenSwitchTime = millis();
	while(not timerElapsed)
	{	
		// === READ TEMPERATURE/FLOW ===
		*(_processValPtr) = getTempPV();
		// === FILTER SETPOINT ===
		// TODO :
		// le calcul est décalé d'une loop, i.e. le calcul du
		// PID se fait avec l'ancienne valeur de la sortie du filtre...
		if(_setPointFilterCst !=0 and pidJustCalculated)
		{
			*(_setPointPtr) = (1-_setPointFilterCst) * (*_setPointPtr) + \
							   _setPointFilterCst * _lastSetPointFilterOutput;
			_lastSetPointFilterOutput = *(_setPointPtr);
		}
		Serial.println(*_setPointPtr);
		// === PID COMPUTE ===
		pidJustCalculated = _myPID.Compute();
		// === PID FILTERING ===
		if(_PIDFilterCst != 0 and pidJustCalculated)
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
	this->_myPID.SetMode(MANUAL);
	*(this->_controlValPtr) = 0;
	this->_refreshSSR();
	this->_ui->showEnd();
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
	if(analogRead(this->_analogPinPV) >= 1023) this->_ui->showErrorPV("NC");
	else this->_ui->setTempPV(*(this->_processValPtr));
	this->_ui->setTime((this->_settedTime-this->_runningTime)/1000);
	this->_ui->setFlow(this->_flow);
		
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
TITLE : getTempPV
DESC : Get process value temperature from _analogPinPV.
       It uses Steinhart-hart thermistor equation with a voltage
       divider with RES1
============================================================
*/
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
	this->_ui->showPumpWarning();
	while(this->_ui->readKeysADC()==KEYNONE)
	{
		this->_ui->setFlow(this->getFlow());
	}
	// === HEATER SWITCHING ===
	this->_ui->showHeaterWarning();
	while(this->_ui->readKeysADC()==KEYNONE) continue;
	// === IDENTIFICATION TESTS ===
	this->_ui->showIdentScreen();
	Serial.begin(9600);
	this->_settedTime = 600000;
	this->_totalStoppedTime = this->_windowStartTime = millis();
	this->_runningTime = 0;
	Serial.println("time,cv,pv");
	while(this->_runningTime <= this->_settedTime) // 15 minutes
	{
		*(this->_processValPtr) = this->getTempPV();
		this->_ui->setTempPV(*(this->_processValPtr));
		this->_runningTime = millis() - this->_totalStoppedTime;
		this->_ui->setTime((this->_settedTime-this->_runningTime)/1000);
		if(this->_runningTime >= 240000)
		{
			*(this->_controlValPtr) = 0;
			this->_ui->setIdentCV(0,SSRWINDOWSIZE);
		}
		else if(this->_runningTime >= 120000)
		{
			*(this->_controlValPtr) = SSRWINDOWSIZE;
			this->_ui->setIdentCV(SSRWINDOWSIZE,SSRWINDOWSIZE);
		}
		else
		{
			*(this->_controlValPtr) = 0.5*SSRWINDOWSIZE;
			this->_ui->setIdentCV(0.5 * SSRWINDOWSIZE, SSRWINDOWSIZE);
		}
		this->_refreshSSR();
		Serial.print((double)this->_runningTime/1000.0,3);
		Serial.print(",");
		Serial.print(*(this->_controlValPtr),0);
		Serial.print(",");
		Serial.println(*(this->_processValPtr),15);
	}
	this->_ui->showEnd();
}