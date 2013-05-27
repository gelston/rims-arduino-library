#include <LiquidCrystal.h>
#include "utility/UIRims.h"
#include "Rims.h"

double settedTemp, currentTemp, ssrControl;

double Kp = 0, Ki = 0, Kd = 0, tauFilter = 0;

LiquidCrystal lcd(8,9,4,5,6,7);
UIRims myUI(lcd,16,2,10,0);
//PID myPID(&input, &output, &setpoint, Kp, Ki, Kd, DIRECT);
Rims myRims(myUI,1,1,&currentTemp,&ssrControl,&settedTemp);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  //myRims.setPIDFilter(tauFilter);

}
void loop() {
  myRims.start();
};
