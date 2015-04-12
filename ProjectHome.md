# Recirculation infusion mash system library for Arduino #

This library implement RIMS controls for home brewers. For definition of a RIMS, see [What is a RIMS?](http://www.hbd.org/kroyster/definition.html)

For me, an Arduino micro controller + a LCD Keypad shield was cheaper and a lot more customizable than a commercial PID controller. So, with this library, a commercial PID controller is unnecessary.

## BASIC FEATURES ##

  * User interface made with [DFRobot LCD Keypad Shield for Arduino](http://www.dfrobot.com/index.php?route=product/product&product_id=51#.Ua_nKBXEnXQ) or similar (can be customized)
  * PID temperature regulation
  * SSR time proportioning control
  * thermistor reading with a voltage divider and Steinhart Hart coefficient
  * countdown timer
  * basic data logging through Serial communication (usb port)

## ADDITIONAL FEATURES ##

  * PID derivative filter
  * Smart integration clamping anti-windup (better than basic integration saturation)
  * Process identification tools
  * Multiple regulator (up to 4) optional : useful for different mash water quantities
  * heater led indicator
  * hall effect flow sensor
  * turn off heater if flow is critically low (< 1 L/min)
  * alarm with speaker when timer is elapsed or when flow\thermistor error
  * alarm if no voltage on heater (if breaker is triggered)
  * different frequencies for each alarms (alarm distinction)
  * Winbond 1 Mbytes SPI flash memory for brew datas
  * USB Menu to access brew data on memory (by holding `<OK`> at startup).

[More info in docs](http://docs.rims-arduino-library.googlecode.com/hg/html/index.html)

## SCREENSHOTS ##

https://dl.dropboxusercontent.com/u/49508582/rims_arduino_library/setPoint.JPG

Set point setup

https://dl.dropboxusercontent.com/u/49508582/rims_arduino_library/timer.JPG

Timer setup

https://dl.dropboxusercontent.com/u/49508582/rims_arduino_library/temp.JPG

Temp screen

https://dl.dropboxusercontent.com/u/49508582/rims_arduino_library/timeFlow.JPG

Timer and flow screen

https://dl.dropboxusercontent.com/u/49508582/rims_arduino_library/ident.JPG

Identification tools screen

https://dl.dropboxusercontent.com/u/49508582/rims_arduino_library/mash_water.JPG

Mash water selection for multiple regulators

## DOWNLOAD LINKS ##

**[Last version download link](https://dl.dropboxusercontent.com/u/49508582/rims_arduino_library/rims_arduino_library_v2.0.3.zip)**

If you like my works : [![](https://www.paypalobjects.com/en_US/i/btn/btn_donate_SM.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=ZZYTKVVQYRSTE&lc=CA&item_name=franckgaga&currency_code=CAD&bn=PP%2dDonationsBF%3abtn_donate_SM%2egif%3aNonHosted)
