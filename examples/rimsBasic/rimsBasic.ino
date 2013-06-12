
#include "LiquidCrystal.h"
#include "Rims.h"

double currentTemp, ssrControl, settedTemp;

LiquidCrystal lcd(8,9,4,5,6,7);
UIRims myUI(lcd,16,2,10,0);
Rims myRims(&myUI,1,11,&currentTemp,&ssrControl,&settedTemp);

void setup() {
  // put your setup code here, to run once:
  myRims.setPinLED(13);
  myRims.setInterruptFlow(1,9.0);
  myRims.setTunningPID(2,5,1,0.1);
  myRims.setSetPointFilter(1);
}
void loop() {
  myRims.start();
}
