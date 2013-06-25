/*****************************************************************
 * Rims.h
 * 
 * Recirculation infusion mash system (RIMS) library for Arduino
 * 
 * Francis Gagnon * This Library is licensed under a GPLv3 License
 *****************************************************************/


#ifndef RimsIdent_h
#define RimsIdent_h

#include "Arduino.h"
#include "Rims.h"
#include "utility/UIRimsIdent.h"


/*! 
 * \brief Toolkits for process identification  to facilitate PID tunning.
 * \author Francis Gagnon
 */
class RimsIdent : public Rims
{
	
public:
	RimsIdent(UIRimsIdent* uiRimsIdent, byte analogPinTherm, byte ssrPin, 
		 double* currentTemp, double* ssrControl, double* settedTemp);
	
	void startIdent();
	
private :
	
	UIRimsIdent* _ui;
	
};

#endif