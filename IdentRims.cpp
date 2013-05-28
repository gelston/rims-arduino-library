/*
  IdentRims.cpp - IdentRims.h definition
*/

#include "Arduino.h"

#include "IdentRims.h"

IdentRims::IdentRims(Rims myRims)
: _myRims(myRims)
{
	Serial.println(_myRims._pinLED);
}