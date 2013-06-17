
#include "LiquidCrystal.h"
#include "Rims.h"

double currentTemp, ssrControl, settedTemp;

LiquidCrystal lcd(8,9,4,5,6,7);
UIRims myUI(&lcd,0,10);
Rims myRims(&myUI,1,11,&currentTemp,&ssrControl,&settedTemp);

void setup() {
  // put your setup code here, to run once:
  myRims.setTunningPID(2,5,1,0.1);
  myRims.setSetPointFilter(1);
}
void loop() {
  myRims.run();
}
