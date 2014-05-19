/*!
 * \file Rims.cpp
 * \brief Rims class definition
 */
 
#include "Arduino.h"
#include "math.h"
#include "utility/PID_v1mod.h"
#include "Rims.h"

/*
============================================================
Global Variable
============================================================
*/

///\brief Last time interrupt was called [µSec]
volatile unsigned long g_flowLastTime = 0;
///\brief Current time interrupt is called [µSec]
volatile unsigned long g_flowCurTime = 0;
///\brief ISR for flow sensor.
void isrFlow(); /// ISR for flow sensor

/*
============================================================
RIMS class definition
============================================================
*/
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
  _pidQty(0), 
  _stopOnCriticalFlow(false), _rimsInitialized(false),
  _memInitialized(false),
  _pinLED(13),_pinHeaterVolt(-1), _noPower(false)
{
	_steinhartCoefs[0] = DEFAULTSTEINHART0;
	_steinhartCoefs[1] = DEFAULTSTEINHART1;
	_steinhartCoefs[2] = DEFAULTSTEINHART2; 
	_steinhartCoefs[3] = DEFAULTSTEINHART3;
	_res1 = DEFAULTRES1;
	_fineTuneTemp = 0;
	for(int i=0;i<=3;i++)
	{
		_kps[i] = 0; _kis[i] = 0; _kds[i] = 0; _tauFilter[i] = 0; 
		_mashWaterValues[i] = -1;
	}
	_myPID.SetSampleTime(SAMPLETIME);
	_myPID.SetOutputLimits(0,SSRWINDOWSIZE);
	_settedTime = (unsigned long)DEFAULTTIME*1000;
	*(_setPointPtr) = DEFAULTSP;
	_currentPID = 0;
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
 * G_{c}(s) = K_{p}+\frac{K_{i}}{s}+\frac{K_{d}s}{Ts+1}
 * \f]
 * Where \f$T=tauFilter[sec]\f$. It can be set to the same value as a PID output filter.
 * 
 * In practice, a good value of derivative filter is :
 * \f[ 
 * T = \frac{K_{d}}{10}
 * \f]
 * 
 * For more information : 
 * http://playground.arduino.cc/Code/PIDLibrary
 * 
 * For anti-windup, integration static saturation was replaced by
 * <b>integration clamping</b>. Basically, it stop integration at a smarter
 * time than "if limit is reached at the output of the integrator". More precisely, it
 * stop integration <b>if limit is reached at the ouput of the PID</b> <i>AND</i>
 * <b>if the temperature error and and the PID output have the same
 * sign.</b>
 * 
 * @image html pid.png "Full PID block diagram"
 *
 * @image html clamping.png "Integration clamping diagram"  
 *
 * \param Kp : float. Propotionnal gain
 * \param Ki : float. Integral gain.
 * \param Kd : float. Derivative gain.
 * \param tauFilter : float. Derivative filter time constant in sec.
 * \param mashWaterQty : int (default=-1). If multiple regulators is needed,
 *                       set this parameter to mash water volume in liter.
 *                       This mash water volume will be associated to this PID.
 *                       Total of 4 regulators is allowed (with different
 *                       mash water volume).
 */
void Rims::setTuningPID(double Kp, double Ki, double Kd, double tauFilter,
                        int mashWaterQty)
{
	if(mashWaterQty != -1) _currentPID = _pidQty;
	else _currentPID = 0;
	_kps[_currentPID] = Kp; _kis[_currentPID] = Ki; _kds[_currentPID] = Kd;
    _tauFilter[_currentPID] = tauFilter;
	_mashWaterValues[_currentPID] = mashWaterQty;
	_currentPID = 0;
	_pidQty++;
}

/*!
 * \brief Set interrupt function for flow sensor.
 * \param interruptFlow : byte. Interrupt pin number connected to the
 *                        flow sensor. For more info :
 *                        http://arduino.cc/en/Reference/attachInterrupt
 * \param flowFactor : float. Factor used to calculate flow from the
 *                     input frequency, i.e. :
 *   				   \f[
 *  				   freq[Hz] = flowFactor * flow[L/min]
 *  				   \f]
 * \param lowBound : float. Lower bound used for accepted flow rate.
 * \param upBound : float. Upper bound used for accepted flow rate
 * \param stopOnCriticalFlow : boolean (default=true). If true and flow is
 *                             <= CRITICALFLOW on getFlow() call, 
 *                             heater is turn off. Else
 *                             flow mesurment doesn't influence heater action.   
 *                             
 */
void Rims::setInterruptFlow(byte interruptFlow, float flowFactor,
							float lowBound,float upBound,
							boolean stopOnCriticalFlow)
{
	attachInterrupt(interruptFlow,isrFlow,RISING);
	_flowFactor = flowFactor;
	_ui->setFlowBounds(lowBound,upBound);
	_stopOnCriticalFlow = stopOnCriticalFlow;
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
 * \brief Set pin to detect if there is voltage applied on heater.
 * 
 * If no voltage is detect on the heater, a speaker alarm is trigerred.
 * 
 * \warning DO NOT APPLY 120V OR 240V DIRECTLY ON ARDUINO PINS.
 * 
 * It can be easily and cheaply made with a +5V DC power supply in
 * parallel with the SSR and heater. It is recommended to
 * add a 10Kohm or so resistor in series with the power supply.
 * 
 * @image html power_circuit.png "Voltage detection circuit"  
 * 
 * I use it to detect if my breaker is trigerred or if I shut it off
 * manually with an external switch. If the PID is informed that the heater
 * is shut off, it will run smoother when the heater will be re-powered.
 * This feature is really not mandatory.
 * 
 * \param pinHeaterVolt : byte. 5V power supply pin.
 */
void Rims::setHeaterPowerDetect(char pinHeaterVolt)
{
	pinMode(pinHeaterVolt,INPUT);
	_pinHeaterVolt = pinHeaterVolt;
}

#ifdef WITH_W25QFLASH
	void Rims::setMemCSPin(byte csPin)
	{ 
		_myMem.setCSPin(csPin); 
		_memInitialized = _myMem.verifyMem();
	}
	
	byte Rims::_memCountSessions()
	{
		byte i, readBuffer[256];
		//first page : table of brew sessions starting addresses
		_myMem.read(ADDRSESSIONTABLE,readBuffer,256);
		for(i=3; i<256; i+=4) if(readBuffer[i] == 0xFF) break; // null value
		return (i-3)/4;
	}
	
	unsigned long Rims::_memCountSessionData()
	{
		boolean freeBitFound = false;
		byte i,offset,page,readBuffer[256];
		for(page=0;page<16;page++) // 16 pages per sector
		{
			_myMem.read(ADDRDATACOUNT+(page*256),readBuffer,256);
			for(offset=0;offset<256;offset++)
			{
				if(readBuffer[offset] & 0xFF)
				{
					freeBitFound = true;
					break;
				}
			}
			if(freeBitFound) break;
		}
		for(i=0;i<8;i++) if((readBuffer[offset]>>i) & 0x01) break;
		return 8*((page*256)+offset)+i;
	}
	
	void Rims::_memInit()
	{
		byte brewSesQty = _memCountSessions();
		unsigned long lastSesDataQty = _memCountSessionData();
		byte buffer[4];
		unsigned long lastStartingAddr;
		Serial.print("brewSesQty:");Serial.println(brewSesQty);
		if(brewSesQty == 0) _memNextAddr = ADDRBREWDATA;
		else
		{
			_myMem.read(ADDRSESSIONTABLE+(4*(brewSesQty-1)),buffer,4);
			memcpy(&lastStartingAddr,buffer,4);
			_memNextAddr = lastStartingAddr+\
			               (BYTESPERDATA*lastSesDataQty) + 4;
		}
		Serial.print("countData:");Serial.println(lastSesDataQty);
		memcpy(buffer,&_memNextAddr,4);
		_myMem.program(ADDRSESSIONTABLE+((brewSesQty*4)%256),buffer,4);
		_myMem.erase(ADDRDATACOUNT,W25Q_ERASE_SECTOR);
		_memDataQty = 0;
		float setPoint = *_setPointPtr;
		memcpy(buffer,&setPoint,4);
		_myMem.program(_memNextAddr,buffer,4);
		_memNextAddr += 4;
	}
	
	void Rims::_memAddBrewData(unsigned long time, unsigned int cv,
							  float pv, float flow,
							  unsigned long timerRemaining)
	{
		byte writeBuffer[BYTESPERDATA], dataCountMkr;
		memcpy(writeBuffer,&time,4);
		memcpy(writeBuffer+4,&cv,2);
		memcpy(writeBuffer+6,&pv,4);
		memcpy(writeBuffer+10,&flow,4);
		memcpy(writeBuffer+14,&timerRemaining,4);
		_myMem.program(_memNextAddr,writeBuffer,BYTESPERDATA);
		dataCountMkr = 0xFF << ((_memDataQty % 8)+1);
		_myMem.program(ADDRDATACOUNT+_memDataQty/8,&dataCountMkr,1);
		_memDataQty ++;
		_memNextAddr += BYTESPERDATA;
	}
#endif

/*!
 * \brief Start and run Rims instance. 
 *
 * Should be called in the loop() function of your sketchbook
 * First time : _initialize() is called
 * Remaining time : _iterate() is called
 */
void Rims::run()
{
	if(not _rimsInitialized) _initialize();
	else _iterate();
}

/*!
 * \brief Initialize a Rims instance before starting temperature regulation.
 *
 * Initialization procedure :
 * -# Ask Temperature set point
 * -# Ask Timer time
 * -# Ask Mash water qty (if setted)
 * -# Show pump switching warning
 * -# Show heater switching warning
 */
void Rims::_initialize()
{
	Serial.begin(9600);
	_timerElapsed = false;
	// === ASK SETPOINT ===
	*(_setPointPtr) = _ui->askSetPoint(*(_setPointPtr));
	// === ASK TIMER ===
	_settedTime = (unsigned long)_ui->askTime(_settedTime/1000)*1000;
	// === ASK MASH WATER ===
	if(_pidQty != 1) _currentPID = _ui->askMashWater(_mashWaterValues,
													 _currentPID);
	// === PUMP SWITCHING WARN ===
	_ui->showPumpWarning(this->getFlow());
	_currentTime = millis();
	unsigned long lastFlowRefresh = _currentTime - SAMPLETIME;
	while(_ui->readKeysADC()==KEYNONE)
	{
		_currentTime = millis();
		if(_currentTime - lastFlowRefresh >= SAMPLETIME)
		{
			_ui->setFlow(this->getFlow(),false);
			lastFlowRefresh = _currentTime;
		}
	}
	// === HEATER SWITCHING WARN ===
	_ui->showHeaterWarning(this->getHeaterVoltage());
	while(_ui->readKeysADC()==KEYNONE)
	{
		if(_pinHeaterVolt != -1)
		{
			_ui->setHeaterVoltState(this->getHeaterVoltage(),false);
		}
	}
#ifdef WITH_W25QFLASH
	// === MEM INIT ===
	_memInit();
#else
	Serial.println("time,sp,cv,pv,flow,timerRemaining");
#endif
	_ui->showTempScreen();
	*(_processValPtr) = this->getTempPV();
	_ui->setTempSP(*(_setPointPtr));
	_ui->setTempPV(*(_processValPtr));
	_sumStoppedTime = true;
	_runningTime = _totalStoppedTime = _timerStopTime = 0;
	_buzzerState = false;
	stopHeating(true);
	_myPID.SetTunings(_kps[_currentPID],_kis[_currentPID],_kds[_currentPID]);
	_myPID.SetDerivativeFilter(_tauFilter[_currentPID]);
	*(_controlValPtr) = 0;
	stopHeating(false);
	_rimsInitialized = true;
	_currentTime = _windowStartTime = _timerStartTime = _rimsStartTime \
				 = _lastScreenSwitchTime = millis();
	_lastTimePID = _currentTime - SAMPLETIME;
}

/*!
 * \brief Main method called for temperature regulation at each iteration
 * 
 * At each PID calculation (at each SAMPLETIME sec), datas is
 * sent over Serial communication for logging purpose.
 *
 */
void Rims::_iterate()
{
	_currentTime = millis();
	if(_currentTime-_lastTimePID>=SAMPLETIME)
	{
		// === READ TEMPERATURE/FLOW ===
		*(_processValPtr) = getTempPV();
		_flow = this->getFlow();
		// === CRITCAL STATES ===
		stopHeating((_stopOnCriticalFlow and _criticalFlow) \
		            or _ncTherm or _noPower);
		// === REFRESH PID ===
		_myPID.Compute();
		// === REFRESH DISPLAY ===
		_refreshDisplay();
		// === DATA LOG ===
#ifdef WITH_W25QFLASH
		_memAddBrewData((_currentTime-_rimsStartTime)/1000.0,
						*(_controlValPtr),
						*(_processValPtr),
						_flow,
						(_settedTime-_runningTime)/1000.0);
#else
		Serial.print(
	    (double)(_currentTime-_rimsStartTime)/1000.0,3);	Serial.print(",");
		Serial.print(*(_setPointPtr));						Serial.print(",");
		Serial.print(*(_controlValPtr),0);					Serial.print(",");
		Serial.print(*(_processValPtr),15);					Serial.print(",");
		Serial.print(_flow,2);								Serial.print(",");
		Serial.println((_settedTime-_runningTime)/1000.0,0);
#endif
		_lastTimePID += SAMPLETIME;
	}
	// === SSR CONTROL ===
	_refreshSSR();
	// === TIME REMAINING ===
	_refreshTimer();
	// === KEY CHECK ===
	int keyPressed = _ui->readKeysADC();
	if((keyPressed!=KEYNONE and _currentTime-_lastScreenSwitchTime>=500)\
	    or _currentTime-_lastScreenSwitchTime >= SCREENSWITCHTIME)
	{
		_ui->switchScreen();
		_ui->timerRunningChar(not(_sumStoppedTime or _timerElapsed));
		_lastScreenSwitchTime = _currentTime;
	}
	if(keyPressed == KEYSELECT and _timerElapsed)
	{
		stopHeating(true);
		_ui->lcdLight(true);
		_ui->ring(false);
		_rimsInitialized = false;
	}
}

/*!
 * \brief Refresh timer value.
 *
 * If error on temperature >= MAXTEMPVAR, timer will not count down.
 * \param verifyTemp : boolean. If true, error on current temperature 
 *					   should not be greater than MAXTEMPVAR to count down.
 *                     Else, current temperature is ignored.
 */
void Rims::_refreshTimer(boolean verifyTemp)
{
	_currentTime = millis();
	if(not _timerElapsed)
	{
		if(abs(*(_setPointPtr)-*(_processValPtr)) <= MAXTEMPVAR or not verifyTemp)
		{
			if(_sumStoppedTime)
			{
				_sumStoppedTime = false;
				_totalStoppedTime += (_timerStartTime - 
											_timerStopTime);
			}
			_runningTime = _currentTime - _totalStoppedTime;
			_timerStopTime = _currentTime;
		}
		else
		{
			_timerStartTime = _currentTime;
			if(not _sumStoppedTime) _sumStoppedTime = true;
		}
		if(_runningTime >= _settedTime) 
		{
			_timerElapsed = true;
			_runningTime = _settedTime;
		}
	}
}

/*!
 * \brief Refresh display used by UIRims instance
 */
void Rims::_refreshDisplay()
{
	_ui->setTempPV(*(_processValPtr));
	if(_timerElapsed)
	{
		_buzzerState = not _buzzerState;
		_ui->ring(_buzzerState);
		_ui->lcdLight(_buzzerState);
	}
	_ui->setTime((_settedTime-_runningTime)/1000);
	_ui->timerRunningChar((not _sumStoppedTime) and (not _timerElapsed));
	_ui->setFlow(_flow);
	_ui->setHeaterVoltState(!_noPower);
		
}

/*!
 * \brief Refresh solid state relay
 * SSR will be refreshed in function of _controlValPtr value. 
 */
void Rims::_refreshSSR()
{
	_currentTime = millis();
	if(_currentTime - _windowStartTime > SSRWINDOWSIZE)
	{
		_windowStartTime += SSRWINDOWSIZE;
	}
	if(_currentTime - _windowStartTime <= *(_controlValPtr))
	{
		digitalWrite(_pinCV,HIGH);
		digitalWrite(_pinLED,HIGH);
	}
	else
	{
		digitalWrite(_pinCV,LOW);
		digitalWrite(_pinLED,LOW);
	}
	_noPower = !this->getHeaterVoltage();
}

/*!
 * \brief Get temperature from thermistor
 *
 * Steinhart-hart equation will be applied here. If voltage is maximal
 * (i.e. ~=5V) it means that the thermistor is not connected and 
 * regulation and heating is stopped until reconnection.
 */
double Rims::getTempPV()
{
	double tempPV = NCTHERM;
	_ncTherm = true;
	int curTempADC = analogRead(_analogPinPV);
	if(curTempADC < 1021)  // connected thermistor
	{
		_ncTherm = false;
		double vin = ((double)curTempADC)/1024.0;
		double resTherm = (_res1*vin)/(1.0-vin);
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
	if(g_flowCurTime == 0) flow = 0.0;
	else if(micros() - g_flowCurTime >= 5e06) flow = 0.0;
	else flow = (1e06 / (_flowFactor* (g_flowCurTime - g_flowLastTime)));
	_criticalFlow = (flow <= CRITICALFLOW);
	return constrain(flow,0,99.99);
}

/*!
 * \brief Check if heater is powered or not.
 */
boolean Rims::getHeaterVoltage()
{
	boolean res = true;
	if(_pinHeaterVolt != -1) res = digitalRead(_pinHeaterVolt);
	return res;
}

/*!
 * \brief Stop heater no matter what PID output
 * \param state : boolean. If true, heater is shut off. Else, heater is 
 *                         turned on.
 */
void Rims::stopHeating(boolean state)
{
	if(state == true)
	{
		_myPID.SetMode(MANUAL);
		*(_controlValPtr) = 0;
		_refreshSSR();
	}
	else _myPID.SetMode(AUTOMATIC);
}

/*
============================================================
ISR Definition
============================================================
*/
/*!
 * \brief ISR for flow sensor.
 *
 * ISR is used as a software capture mode. Time values are store in 
 * g_flowLastTime and g_flowCurTime.
 * 
 */
void isrFlow()
{
	g_flowLastTime = g_flowCurTime;
	g_flowCurTime = micros();
}
