#include "RimsIdent.h"
#include "LiquidCrystal.h"

double currentTemp, ssrControl, settedTemp;

LiquidCrystal lcd(8,9,4,5,6,7);
UIRimsIdent myUI(&lcd,0,10);
RimsIdent myIdent(&myUI,1,11,&currentTemp,&ssrControl,&settedTemp);

void setup() {
  // put your setup code here, to run once:
}
void loop() {
  myIdent.run();
};
