/*
  Rims.h
  
  Librairie pour un système RIMS 
  (recirculation infusion mash system)
  Gestion du PID en température
  
  Francis Gagnon
*/


#ifndef Rims_h
#define Rims_h

#include "Arduino.h"

#include "LiquidCrystal.h"
#include "utility/UIRims.h"

class Rims
{
	
public:
	Rims(UIRims uiRims);

	void start();
	
private:
	UIRims _uiRims;
};

#endif