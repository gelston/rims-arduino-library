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
#define RES1 9920.0

#define STEINHART0 0.000480541720259488
#define STEINHART1 0.000287458436095242
#define STEINHART2 -3.07840710605727e-06
#define STEINHART3 8.65973846884587e-08

#define DEFAULTSP 68 // Celsius
#define DEFAULTTIME 5400 // seconds

#include "Arduino.h"

#include "LiquidCrystal.h"
#include "utility/UIRims.h"

class Rims
{
	
public:
	Rims(UIRims uiRims, byte analogPinPV, byte interruptFlow);

	void start();
	
	float analogInToCelcius(int analogIn);
	
	float getFlow();
	
protected:
	
private:
	
	
	static void _isrFlowSensor();
	
	static Rims* _rimsPtr;
	
	UIRims _uiRims;
	byte _analogPinPV;
	
	float _tempSP;
	float _tempPV;
	
	unsigned long _settedTime;				//mSec
	unsigned long _startTime;				//mSec
	
	float _flow;
	
	volatile unsigned long _flowLastTime;	//µSec
	volatile unsigned long _flowCurTime;	//µSec
};

#endif