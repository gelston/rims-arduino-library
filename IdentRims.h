/*
  IdentRims.h
  
  System identification library for Rims.
  Usefull to calculate PID tunning parameters.
  
  Francis Gagnon
*/


#ifndef IdentRims_h
#define IdentRims_h

#include "Arduino.h"
#include "Rims.h"

class IdentRims
{
	
public:
	IdentRims(Rims myRims);

private:
	Rims _myRims;
	
};

#endif