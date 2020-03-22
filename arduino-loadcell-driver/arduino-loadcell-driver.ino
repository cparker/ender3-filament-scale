/*

 An arduino nano driver for HX711 load cell

 This example demonstrates basic scale output. See the calibration sketch to get the calibration_factor for your
 specific load cell setup.

 This example code uses bogde's excellent library: https://github.com/bogde/HX711
 bogde's library is released under a GNU GENERAL PUBLIC LICENSE


*/

#include "HX711.h"

// this was discovered
#define calibration_factor 441

#define DOUT  3
#define CLK  2
#define TARE_BUTTON INT0

volatile int  tareButtonInterrupt = 0;

long sendScaleIntervalMS = 10000;
long lastSendScaleMS = 0;

long checkTareIntervalMS = 100;
long lastCheckTareMS = 0;

HX711 scale;



void handleTareButton() {
    tareButtonInterrupt = 1;
}



void setup() {
  Serial.begin(9600);
  Serial.println("setup");

  scale.begin(DOUT, CLK);
  // This calibration value is obtained by using the SparkFun_HX711_Calibration sketch
  scale.set_scale(calibration_factor); 

  // connect the tare button pin to ground directly, then bias the pin high with the input pullup
  // then when the button is pressed it will go low
  pinMode(TARE_BUTTON, INPUT_PULLUP);

  attachInterrupt(TARE_BUTTON, handleTareButton, FALLING);
}

void loop() {

  // send the scale reading over serial every 10 seconds (set by variable)
  if (millis() - lastSendScaleMS > sendScaleIntervalMS) {
      lastSendScaleMS = millis();
      Serial.print("weight grams:"); Serial.println(scale.get_units(10), 2); //scale.get_units() returns a float
  }

  // check for the tare button 10 times a second 
  if (millis() - lastCheckTareMS > checkTareIntervalMS) {
      lastCheckTareMS = millis();

    if (tareButtonInterrupt == 1) {
        tareButtonInterrupt = 0;
        Serial.println("TARE");
        scale.tare();
    }
  }
}