/*!
 * \file RimsIdent.h
 * \brief RimsIdent class declaration
 */


#ifndef RimsIdent_h
#define RimsIdent_h

///\brief Sample time for identification [mSec]
#define IDENTSAMPLETIME 1000

///\brief Total length of the identification procedure [mSec]
#define IDENTLENGTH 1800000

#define STEP1TIME 0
#define STEP1VALUE 0.5*SSRWINDOWSIZE

#define STEP2TIME 600000
#define STEP2VALUE SSRWINDOWSIZE

#define STEP3TIME 1200000
#define STEP3VALUE 0

#include "Arduino.h"
#include "Rims.h"
#include "utility/UIRimsIdent.h"


/*! 
 * \brief Toolkits for process identification  to facilitate PID tunning.
 *
 * It sends differents values to the SSR (0%->50%->100%->0%) and monitors
 * the resulting temperature on the
 * <a href=http://arduino.cc/en/reference/serial>serial monitor</a>.
 * Open the monitor before stating identification. It last 30 min.
 *
 * \author Francis Gagnon
 */
class RimsIdent : public Rims
{
	
public:
	RimsIdent(UIRimsIdent* uiRimsIdent, byte analogPinTherm, byte ssrPin, 
		 double* currentTemp, double* ssrControl, double* settedTemp);
	
	void run();
	
	void setInterruptFlow(byte interruptFlow, float flowFactor, 
					      boolean stopOnCriticalFlow = false);
	
protected : 

	void _initialize();
	void _iterate();
	
private :
	
	UIRimsIdent* _ui;
	
	unsigned long _lastTimeSerial;
	
};

#endif