#include "Rims.h"
#include "LiquidCrystal.h"

double currentTemp, ssrControl, settedTemp;

LiquidCrystal lcd(8,9,4,5,6,7);
UIRimsIdent myUI(lcd,16,2,10,0);
RimsIdent myIdent(&myUI,1,11,&currentTemp,&ssrControl,&settedTemp);

void setup() {
  // put your setup code here, to run once:
  myIdent.setPinLED(13);
  myIdent.setInterruptFlow(1);
}
void loop() {
  myIdent.startIdent();
};
