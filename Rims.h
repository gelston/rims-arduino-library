/*! 
 * \mainpage Recirculation infusion mash system library for arduino
 * 
 * - Rims main class
 *
 * Francis Gagnon
 * This Library is licensed under a GPLv3 License
 *
 */
 */

Rims* Rims::_rimsPtr = 0;

#ifndef Rims_h
#define Rims_h

#define VALIM 5 /// ADC max value [volts]

#define PIDSAMPLETIME 1000 /// mSec
#define SSRWINDOWSIZE 5000 /// mSec

#define DEFAULTSP 68 /// celsius
#define DEFAULTTIME 5400 /// seconds

#define DEFAULTSTEINHART0 0.001
#define DEFAULTSTEINHART1 0.0002
#define DEFAULTSTEINHART2 -4e-7
#define DEFAULTSTEINHART3 1e-7
#define DEFAULTRES1 10000

#define MAXTEMPVAR 2.0 /// celsius
#define SCREENSWITCHTIME 10000 /// mSec

#include "Arduino.h"
#include "utility/UIRims.h"
#include "utility/PID_v1.h"



/*!
 * \brief Recirculation infusion mash system (RIMS) library for Arduino
 * \author Francis Gagnon 
 *
 * \bug Only one Rims (or child) instance is allowed with flow sensor
        because of static method _isrFlowSensor()
 */

class Rims
{
	friend class RimsIdent;
	
public:
	Rims(UIRims* uiRims, byte analogPinTherm, byte ssrPin, 
		 double* currentTemp, double* ssrControl, double* settedTemp);

	void setThermistor(float steinhartCoefs[],float res1, float fineTune = 0);
	void setTunningPID(double Kp, double Ki, double Kd, double tauFilter);
	void setSetPointFilter(double tauFilter);
	void setPinLED(byte pinLED);
	void setInterruptFlow(byte interruptFlow, float flowFactor);
	
	void run();
	
	double getTempPV();
	float getFlow();
	
protected:
	
	void _initialize();
	void _iterate();
	
	void _refreshTimer();
	void _refreshDisplay();
	void _refreshSSR();
	
private:
	
	static Rims* _rimsPtr;
	
	UIRims* _ui;
	PID _myPID;
	byte _analogPinPV;
	byte _pinCV;
	byte _pinLED;
	
	boolean _rimsInitialized;
	boolean _pidJustCalculated;
	
	double _rawSetPoint;
	
	double* _setPointPtr;
	double* _processValPtr;
	double* _controlValPtr; /// [0,SSRWINDOWSIZE]
	
	float _steinhartCoefs[4];
	float _res1;
	float _fineTuneTemp;
	
	double _PIDFilterCst;
	double _lastPIDFilterOutput;
	
	double _setPointFilterCst;
	double _lastSetPointFilterOutput;
	
	unsigned long _windowStartTime;			///mSec
	unsigned long _settedTime;				///mSec
	unsigned long _runningTime;				///mSec
	unsigned long _totalStoppedTime;		///mSec
	unsigned long _timerStopTime;			///mSec
	unsigned long _timerStartTime;			///mSec
	unsigned long _lastScreenSwitchTime;
	unsigned long _lastTimePID;
	boolean _sumStoppedTime;
	boolean _timerElapsed;
	
	static void _isrFlowSensor(); ///mSec ISR for hall-effect flow sensor
	float _flowFactor; /// freq[Hz] = flowFactor * flow[L/min]
	float _flow;
	
	volatile unsigned long _flowLastTime;	///µSec
	volatile unsigned long _flowCurTime;	///µSec
	
};

#endif