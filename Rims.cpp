/*!
 * \file Rims.cpp
 * \brief Rims class definition
 */
 
#include "Arduino.h"
#include "math.h"
#include "utility/PID_v1.h"
#include "Rims.h"


/*!
 * \brief Constructor
 * \param uiRims : UIRims*. Pointer to UIRims instance
 * \param analogPinTherm : byte. Analog pin to thermistor
 * \param ssrPin : byte. Pin to control heater's solid state relay.
 * \param currentTemp : double*. Pointer to a double that 
 *                      will be used for current temparature
 * \param ssrControl : double*. Pointer to a double that 
 *                     will use to control SSR
 * \param settedTemp : double*. Pointer to a double that 
 *                     will be use to store setted temperature
 */
Rims::Rims(UIRims* uiRims, byte analogPinTherm, byte ssrPin, 
	       double* currentTemp, double* ssrControl, double* settedTemp)
: _ui(uiRims), _analogPinPV(analogPinTherm), _pinCV(ssrPin),
  _processValPtr(currentTemp), _controlValPtr(ssrControl), _setPointPtr(settedTemp),
  _myPID(currentTemp, ssrControl, settedTemp, 0, 0, 0, DIRECT),
  _secondPIDSetted(false), _rimsInitialized(false),
  _pinLED(13),
  _flowLastTime(0), _flowCurTime(0)
{
	_steinhartCoefs[0] = DEFAULTSTEINHART0;
	_steinhartCoefs[1] = DEFAULTSTEINHART1;
	_steinhartCoefs[2] = DEFAULTSTEINHART2; 
	_steinhartCoefs[3] = DEFAULTSTEINHART3;
	_res1 = DEFAULTRES1;
	_fineTuneTemp = 0;
	for(int i=0;i<=1;i++)
	{
		_PIDFilterCsts[i] = 0; _setPointFilterCsts[i] = 0;
		_kps[i] = 0; _kis[i] = 0; _kds[i] = 0;
	}
	_myPID.SetSampleTime(PIDSAMPLETIME);
	_myPID.SetOutputLimits(0,SSRWINDOWSIZE);
	pinMode(ssrPin,OUTPUT);
	pinMode(13,OUTPUT);
}

/*!
 * \brief Set thermistor parameters.
 *
 * @image html thermistor_circuit.png "Thermistor voltage divider circuit" 
 * \param steinhartCoefs : float[4]. Steinhart-hart equation coefficients in order
 *        of increasing power, i.e :
 *    	  \f[
 *	      \frac{1}{T[kelvin]}=C_{0}+C_{1}\ln(R)+C_{2}\ln(R)^2+C_{3}\ln(R)^3
 *   	  \f]
 *        for more information : http://en.wikipedia.org/wiki/Steinhart%E2%80%93Hart_equation
 *
 * \param res1 : float. In ohm.
 * \param fineTuneTemp : float. optional (default=0). if you want to add a fine tune factor
          after the steinhart-hart temperature calculation.
 */
void Rims::setThermistor(float steinhartCoefs[], float res1, float fineTuneTemp)
{
	for(int i=0;i<=4;i++) _steinhartCoefs[i] = steinhartCoefs[i];
	_res1 = res1;
	_fineTuneTemp = fineTuneTemp;
}

/*!
 * \brief Set tuning for PID object.
 *
 * Algorithm is in parallel form i.e. :
 * \f[ 
 * G_{c}(s) = [K_{p}+\frac{K_{i}}{s}+K_{d}s]\times\frac{1}{Ts+1}
 * \f]
 * Where \f$T=tauFilter[sec]\f$
 * For more information : 
 * http://playground.arduino.cc/Code/PIDLibrary
 *
 * \param Kp : float. Propotionnal gain
 * \param Ki : float. Integral gain.
 * \param Kd : float. Derivative gain.
 * \param tauFilter : float. PID output filter time constant in sec.
 * \param batchSize : byte (default=0). 
 *                    Specify which of the 2 regulators to setup, if needed.
 */
void Rims::setTuningPID(double Kp, double Ki, double Kd, double tauFilter,
                         byte batchSize)
{
	_kps[batchSize] = Kp; _kis[batchSize] = Ki; _kds[batchSize] = Kd;
	if(tauFilter>0)
	{
		_PIDFilterCsts[batchSize] = exp((-1.0)*PIDSAMPLETIME/(tauFilter*1000.0));
	}
	else _PIDFilterCsts[batchSize] = 0;
	_lastPIDFilterOutput = 0;
	_secondPIDSetted = (batchSize!=0);
}

/*!
 * \brief Set Set Point filter. 
 *
 * A set point filter allow you to cancel overshoot (for integretor plant).
 * Normally : 
 * \f[
 * tauFilter=\frac{K_{c}}{K_{i}}
 * \f]
 *
 * With this value, overshoot will be totally canceled. If you found that
 * that with this time constant, your RIMS is too slow, my advice would be to try
 * from 0.1 to 1 times this value. You will be able to find a good comprimise
 * between overshoot and rapidity.
 *
 * \param tauFilter : float. set point filter time constant in sec.
 * \param batchSize : byte. (default=0).
 *                    Specify which of the 2 regulators to setup, if needed.
 */
void Rims::setSetPointFilter(double tauFilter,byte batchSize)
{
	if(tauFilter>0)
	{
		_setPointFilterCsts[batchSize] = \
					exp((-1.0)*PIDSAMPLETIME/(tauFilter*1000.0));
	}
	else _setPointFilterCsts[batchSize] = 0;
	_lastSetPointFilterOutput = 0;
}

/*!
 * \brief Set interrupt function for flow sensor.
 * \param interruptFlow : byte. Interrupt pin number connected to the
 *                        flow sensor. For more info :
 *                        http://arduino.cc/en/Reference/attachInterrupt
 * \param flowFactor : float. Factor used to calculate flow from the
                       input frequency, i.e. :
					   \f[
					   freq[Hz] = flowFactor * flow[L/min]
					   \f]
 */
void Rims::setInterruptFlow(byte interruptFlow, float flowFactor)
{
	Rims::_rimsPtr = this;
	attachInterrupt(interruptFlow,Rims::_isrFlowSensor,RISING);
	_flowFactor = flowFactor;
}

/*!
 * \brief Set pin for heater LED indicator
 * \param pinLED : byte
 */
void Rims::setPinLED(byte pinLED)
{
	pinMode(pinLED,OUTPUT);
	_pinLED = pinLED;
}

/*!
 * \brief Start and run Rims instance. 
 *
 * Should be called in the loop() function of your sketchbook
 * First time : _initialize() is called
 * Remaining time : _iterate() is called
 */
void Rims::run()
{
	if(not _rimsInitialized) this->_initialize();
	else
	{
		if(not _timerElapsed) this->_iterate();
		else
		{
			_myPID.SetMode(MANUAL);
			*(_controlValPtr) = 0;
			_refreshSSR();
			_rimsInitialized = false;
			_ui->showEnd();
		}
	}
}

/*!
 * \brief Initialize a Rims instance before starting temperature regulation.
 *
 * Initialization procedure :
 * -# Ask Temperature set point
 * -# Ask Timer time
 * -# Ask Batch size (if setted)
 * -# Show pump switching warning
 * -# Show heater switching warning
 */
void Rims::_initialize()
{
	_timerElapsed = false, _pidJustCalculated = true;
	// === ASK SETPOINT ===
	_rawSetPoint = _ui->askSetPoint(DEFAULTSP);
	*(_setPointPtr) = 0;
	// === ASK TIMER ===
	_settedTime = (unsigned long)_ui->askTime(DEFAULTTIME)*1000;
	// === ASK BATCH SIZE ===
    _batchSize = (_secondPIDSetted) ? _ui->askBatchSize() : 0;
	// === PUMP SWITCHING WARN ===
	_ui->showPumpWarning();
	while(_ui->readKeysADC()==KEYNONE)
	{
		_ui->setFlow(this->getFlow());
	}
	// === HEATER SWITCHING WARN ===
	_ui->showHeaterWarning();
	while(_ui->readKeysADC()==KEYNONE) continue;
	_ui->showTempScreen();
	*(_processValPtr) = this->getTempPV();
	_ui->setTempSP(_rawSetPoint);
	_ui->setTempPV(*(_processValPtr));
	_sumStoppedTime = true;
	_runningTime = _totalStoppedTime = _timerStopTime = 0;
	_myPID.SetTunings(_kps[_batchSize],_kis[_batchSize],_kds[_batchSize]);
	_myPID.SetMode(AUTOMATIC);
	_rimsInitialized = true;
	_windowStartTime = _timerStartTime \
					 = _lastScreenSwitchTime = millis();
	_lastTimePID = _timerStartTime - PIDSAMPLETIME;
}

/*!
 * \brief Main method called for temperature regulation at each iteration
 */
void Rims::_iterate()
{
	// !!!!!!!!!!!!! TODO !!!!!!!!!!!!!!
	// better timing like identRimsBasic (not always refresh *(_processValPtr))
	// (not always update Flow)
	unsigned long currentTime = millis();
	// === READ TEMPERATURE/FLOW ===
	*(_processValPtr) = getTempPV();
	_flow = this->getFlow();
	// === SETPOINT FILTERING ===
	currentTime = millis();
	if(currentTime-_lastTimePID>=PIDSAMPLETIME)
	{
		*(_setPointPtr) = (1-_setPointFilterCsts[_batchSize]) * _rawSetPoint + \
				_setPointFilterCsts[_batchSize] * _lastSetPointFilterOutput;
		_lastSetPointFilterOutput = *(_setPointPtr);
		_lastTimePID = currentTime;
	}
	// === PID COMPUTE ===
	_pidJustCalculated = _myPID.Compute();
	// === PID FILTERING ===
	if(_pidJustCalculated)
	{
		*(_controlValPtr) = (1-_PIDFilterCsts[_batchSize]) * (*_controlValPtr) + \
							_PIDFilterCsts[_batchSize] * _lastPIDFilterOutput;
		_lastPIDFilterOutput = *(_controlValPtr);
	}
	// === SSR CONTROL ===
	_refreshSSR();
	// === TIME REMAINING ===
	_refreshTimer();
	// === REFRESH DISPLAY ===
	_refreshDisplay();
	// === KEY CHECK ===
	currentTime = millis();
	if((_ui->readKeysADC()!=KEYNONE and currentTime-_lastScreenSwitchTime>=500)\
	    or currentTime-_lastScreenSwitchTime >= SCREENSWITCHTIME)
	{
		_ui->switchScreen();
		_lastScreenSwitchTime = millis();
	}
	if(_runningTime >= _settedTime) _timerElapsed = true;
}

/*!
 * \brief Refresh timer value.
 *
 * If error on temperature >= MAXTEMPVAR, timer will not count down.
 */
void Rims::_refreshTimer()
{
	unsigned long currentTime = millis();
	if(abs(_rawSetPoint - *(_processValPtr)) <= MAXTEMPVAR)
	{
		if(_sumStoppedTime)
		{
			_sumStoppedTime = false;
			_totalStoppedTime += (_timerStartTime - 
										_timerStopTime);
		}
		_runningTime = currentTime - _totalStoppedTime;
		_timerStopTime = currentTime;
	}
	else
	{
		_timerStartTime = currentTime;
		if(not _sumStoppedTime) _sumStoppedTime = true;
	}
}

/*!
 * \brief Refresh display used by UIRims instance
 */
void Rims::_refreshDisplay()
{
	if(analogRead(_analogPinPV) >= 1023) _ui->showErrorPV("NC");
	else _ui->setTempPV(*(_processValPtr));
	_ui->setTime((_settedTime-_runningTime)/1000);
	_ui->setFlow(_flow);
		
}

/*!
 * \brief Refresh solid state relay
 * SSR will be refreshed in function of _controlValPtr value. 
 */
void Rims::_refreshSSR()
{
	unsigned long currentTime = millis();
	if(currentTime - _windowStartTime > SSRWINDOWSIZE)
	{
		_windowStartTime += SSRWINDOWSIZE;
	}
	if(currentTime - _windowStartTime <= *(_controlValPtr))
	{
		digitalWrite(_pinCV,HIGH);
		digitalWrite(_pinLED,HIGH);
	}
	else
	{
		digitalWrite(_pinCV,LOW);
		digitalWrite(_pinLED,LOW);
	}
}

/*!
 * \brief Get temperature from thermistor
 *
 * Steinhart-hart equation will be applied here. If voltage is maximal
 * (i.e. 5V) it means that the thermistor is not connected and 
 * regulation and heating is stopped until reconnection.
 */
double Rims::getTempPV()
{
	double tempPV = 0;
	int curTempADC = analogRead(_analogPinPV);
	if(curTempADC >= 1023)
	{
		if(_myPID.GetMode()==AUTOMATIC)
		{
			_myPID.SetMode(MANUAL);
			*(_controlValPtr) = 0;
		}
	}
	else
	{
		if(_myPID.GetMode()==MANUAL) _myPID.SetMode(AUTOMATIC);
		double vin = ((double)curTempADC*VALIM)/1024.0;
		double resTherm = (_res1*vin)/(VALIM-vin);
		double logResTherm = log(resTherm);
		double invKelvin = _steinhartCoefs[0]+\
						_steinhartCoefs[1]*logResTherm+\
						_steinhartCoefs[2]*pow(logResTherm,2)+\
						_steinhartCoefs[3]*pow(logResTherm,3);
		tempPV = (1/invKelvin)-273.15+_fineTuneTemp;
	}
	return tempPV;
}

/*!
 * \brief Get flow from hall-effect flow sensor.
 */
float Rims::getFlow()
{
	float flow;
	if(_flowCurTime == 0) flow = 0.0;
	else if(micros() - _flowCurTime >= 5e06) flow = 0.0;
	else flow = (1e06 / (_flowFactor* (_flowCurTime - _flowLastTime)));
	return flow;
}

/*
============================================================
static members definition for flow sensor
============================================================
*/

Rims* Rims::_rimsPtr = 0;

void Rims::_isrFlowSensor()
{
	Rims::_rimsPtr->_flowLastTime = Rims::_rimsPtr->_flowCurTime;
	Rims::_rimsPtr->_flowCurTime = micros();
}