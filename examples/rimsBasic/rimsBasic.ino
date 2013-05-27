#include <LiquidCrystal.h>
#include "utility/UIRims.h"
#include "Rims.h"

double settedTemp, currentTemp, ssrControl;

LiquidCrystal lcd(8,9,4,5,6,7);
UIRims myUI(lcd,16,2,10,0);
Rims myRims(myUI,1,11,&currentTemp,&ssrControl,&settedTemp);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  myRims.setLedPin(13);
  myRims.setInterruptFlow(1);
  myRims.setTunningPID(1,1,1,10);

}
void loop() {
  myRims.start();
};
