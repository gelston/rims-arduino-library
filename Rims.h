/*
  Rims.h
  
  Librairie pour un système RIMS 
  (recirculation infusion mash system)
  Gestion du PID en température
  
  Francis Gagnon
*/


#ifndef Rims_h
#define Rims_h

#define VALIM 4.97
#define RES1 9900.0

#define STEINHART0 0.000480541720259488
#define STEINHART1 0.000287458436095242
#define STEINHART2 -3.07840710605727e-06
#define STEINHART3 8.65973846884587e-08

#include "Arduino.h"

#include "LiquidCrystal.h"
#include "utility/UIRims.h"

class Rims
{
	
public:
	Rims(UIRims uiRims);

	void start();
	float analogInToCelcius(int analogIn);
	
protected:
	
private:
	UIRims _uiRims;
};

#endif