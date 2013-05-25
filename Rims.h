/*
  Rims.h
  
  Librairie pour un système RIMS 
  (recirculation infusion mash system)
  Gestion du PID en température
  
  Francis Gagnon
*/


#ifndef Rims_h
#define Rims_h

#define VALIM 5
#define RES1 10010.0

#define STEINHART0 0.000480541720259488
#define STEINHART1 0.000287458436095242
#define STEINHART2 -3.07840710605727e-06
#define STEINHART3 8.65973846884587e-08

#define FINETUNETEMP -0.3

#define PIDSAMPLETIME 5000 // mSec 

#define DEFAULTSP 68 // Celsius
#define DEFAULTTIME 5400 // seconds

#include "Arduino.h"

#include "utility/UIRims.h"
#include "utility/PID_v1.h"

class Rims
{
	
public:
	Rims(UIRims uiRims, byte analogPinPV, byte interruptFlow,
		 byte ssrPin, byte ledPin,
		 PID myPID);

	void start();
	
	float analogInToCelcius(int analogIn);
	
	float getFlow();
	PID getPID();
	
protected:
	
private:
	
	
	static void _isrFlowSensor();
	
	static Rims* _rimsPtr;
	
	UIRims _uiRims;
	PID _myPID;
	byte _analogPinPV;
	byte _ssrPin;
	byte _ledPin;
	
	double* _tempSP;
	double* _tempPV;
	double* _controlValue; // [0,1]
	
	unsigned long _settedTime;				//mSec
	unsigned long _startTime;				//mSec
	
	float _flow;
	
	volatile unsigned long _flowLastTime;	//µSec
	volatile unsigned long _flowCurTime;	//µSec
	
};

#endif