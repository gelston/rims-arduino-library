/*
  Rims.h
  
  Librairie pour un système RIMS 
  (recirculation infusion mash system)
  Gestion du PID en température
  
  Francis Gagnon
*/


#ifndef Rims_h
#define Rims_h

#define VALIM 5 // volts
#define RES1 10010.0 // ohms
#define STEINHART0 0.000480541720259488
#define STEINHART1 0.000287458436095242
#define STEINHART2 -3.07840710605727e-06
#define STEINHART3 8.65973846884587e-08
#define FINETUNETEMP -0.3 // celsius

#define PIDSAMPLETIME 5000 // mSec 

#define DEFAULTSP 68 // celsius
#define DEFAULTTIME 5400 // seconds

#include "Arduino.h"

#include "utility/UIRims.h"
#include "utility/PID_v1.h"

class Rims
{
	
public:
	Rims(UIRims uiRims, byte analogPinTherm, byte ssrPin, 
		 double* currentTemp, double* ssrControl, double* settedTemp);

	void setTunningPID(double Kp, double Ki, double Kd, double tauFilter);
	void setLedPin(byte ledPin);
	void setInterruptFlow(byte interruptFlow);
	void setPIDFilter(double tauFilter);
	
	void start();
	
	float analogInToCelcius(int analogIn);
	
	float getFlow();
	
protected:
	
private:
	
	
	static void _isrFlowSensor();
	
	static Rims* _rimsPtr;
	
	UIRims _uiRims;
	PID _myPID;
	byte _analogPinPV;
	byte _pinCV;
	byte _ledPin;
	
	double* _setPointPtr;
	double* _processValPtr;
	double* _controlValPtr; // [0,1]
	
	unsigned long _settedTime;				//mSec
	unsigned long _startTime;				//mSec
	
	float _flow;
	
	volatile unsigned long _flowLastTime;	//µSec
	volatile unsigned long _flowCurTime;	//µSec
	
};

#endif