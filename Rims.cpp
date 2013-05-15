/*
  Rims.cpp - Définition de Rims.h
*/

#include "Arduino.h"
#include "math.h"
#include "Rims.h"

/*
============================================================
TITLE : Rims (constructeur)
INPUT : int pin (entrée 0-3.3V)
OUTPUT : void
DESC : Constructeur d'un objet Rims
============================================================
*/
Rims::Rims(UIRims uiRims)
: _uiRims(uiRims)
{

}

/*
============================================================
TITLE : start
INPUT : -
OUTPUT : void
DESC : Routine principale
============================================================
*/
void Rims::start()
{
}

/*
============================================================
TITLE : analogInToCelcius
INPUT : int analogIn
OUTPUT : float
DESC : Steinhart-hart thermistor equation with a voltage
       divider with RES1
============================================================
*/
float Rims::analogInToCelcius(int analogIn)
{
	float vin = ((float)analogIn*VALIM)/(float)1024;
	float resTherm = ((float)RES1*vin)/(VALIM-vin);
	float logResTherm = log(resTherm);
	float invKelvin = (float)STEINHART0+\
	                  (float)STEINHART1*logResTherm+\
	                  (float)STEINHART2*pow(logResTherm,2)+\
	                  (float)STEINHART3*pow(logResTherm,3);
	return (1/invKelvin)-273.15;
}