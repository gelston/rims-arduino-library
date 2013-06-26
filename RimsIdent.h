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
 *
 * It sends differents values to the SSR (0%->50%->100%->0%) and monitors
 * the resulting temperature on the
 * <a href=http://arduino.cc/en/reference/serial>serial monitor</a>.
 * Open the monitor before stating identification. It last 15 min.
 *
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