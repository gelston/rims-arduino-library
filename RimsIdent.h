/*!
 * \file RimsIdent.h
 * \brief RimsIdent class declaration
 */


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