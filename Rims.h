/*
  Rims.h
  
  Librairie pour un système RIMS 
  (recirculation infusion mash system)
  Gestion du LCD ainsi que du PID en température
  
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
	Rims(LiquidCrystal* lcd, byte col, byte row, byte pinLight,
	     byte pinKeysAnalog);

	void start();
};

#endif