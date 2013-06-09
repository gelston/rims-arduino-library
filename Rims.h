/*****************************************************************
 * Rims.h
 * 
 * Recirculation infusion mash system (RIMS) library for Arduino
 * 
 * Francis Gagnon * This Library is licensed under a GPLv3 License
 *****************************************************************/


#ifndef Rims_h
#define Rims_h

#define VALIM 5 // volts
#define RES1 10010.0 // ohms
#define STEINHART0 0.000480541720259488
#define STEINHART1 0.000287458436095242
#define STEINHART2 -3.07840710605727e-06
#define STEINHART3 8.65973846884587e-08
#define FINETUNETEMP -0.3 // celsius

#define PIDSAMPLETIME 1000 // mSec 
#define SSRWINDOWSIZE 5000 // mSec

#define DEFAULTSP 68 // celsius
#define DEFAULTTIME 5400 // seconds

#define MAXTEMPVAR 2.0 // celsius

#include "Arduino.h"
#include "utility/UIRims.h"
#include "utility/PID_v1.h"



// === Rims =======================================
// === Main class for Rims library ================
// ================================================
class Rims
{
	friend class RimsIdent;
	
public:
	Rims(UIRims* uiRims, byte analogPinTherm, byte ssrPin, 
		 double* currentTemp, double* ssrControl, double* settedTemp);

	void setTunningPID(double Kp, double Ki, double Kd, double tauFilter);
	void setSetPointFilter(double tauFilter);
	void setPinLED(byte pinLED);
	void setInterruptFlow(byte interruptFlow, float flowFactor);
	
	void start();
	
	double analogInToCelcius(int analogIn);
	
	double getTempPV();
	float getFlow();
	
protected:
	
	void _refreshTimer();
	void _refreshDisplay();
	void _refreshSSR();
	
private:
	
	static Rims* _rimsPtr;
	
	UIRims* _ui;
	PID _myPID;
	byte _analogPinPV;
	byte _pinCV;
	byte _pinLED;
	
	double* _setPointPtr;
	double* _processValPtr;
	double* _controlValPtr; // [0,SSRWINDOWSIZE]
	
	double _PIDFilterCst;
	double _lastPIDFilterOutput;
	
	double _setPointFilterCst;
	double _lastSetPointFilterOutput;
	
	unsigned long _windowStartTime;			//mSec
	
	unsigned long _settedTime;				//mSec
	unsigned long _runningTime;				//mSec
	unsigned long _totalStoppedTime;		//mSec
	unsigned long _timerStopTime;			//mSec
	unsigned long _timerStartTime;			//mSec
	boolean _sumStoppedTime;
	
	static void _isrFlowSensor(); // ISR for hall-effect flow sensor
	float _flowFactor; // freq[Hz] = flowFactor * flow[L/min]
	float _flow;
	
	volatile unsigned long _flowLastTime;	//µSec
	volatile unsigned long _flowCurTime;	//µSec
	
};

#endif