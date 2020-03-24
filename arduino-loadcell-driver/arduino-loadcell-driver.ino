/*

 An arduino nano driver for HX711 load cell

 This example demonstrates basic scale output. See the calibration sketch to get the calibration_factor for your
 specific load cell setup.

 This example code uses bogde's excellent library: https://github.com/bogde/HX711
 bogde's library is released under a GNU GENERAL PUBLIC LICENSE


*/

#include "HX711.h"
#include "EEPROM.h"

// this was discovered
#define calibration_factor 441

#define DOUT PD4
#define CLK  PD2
#define TARE_BUTTON PD3

volatile int  tareButtonInterrupt = 0;

const long sendScaleIntervalMS = 10000;

const long takeReadingIntervalMS = 500;
long lastReadingMS = 0;

const long checkTareIntervalMS = 100;
long lastCheckTareMS = 0;
int tareLowCount = 0;
int tareLowThreshold = 10; 

const long checkTareButtonIntervalMS = 100;
long lastCheckTareButtonMS = 0;

const int scaleReadingLength = sendScaleIntervalMS / takeReadingIntervalMS;
float scaleReadings[scaleReadingLength];
int scaleReadingIndex = 0;

int tareValueStoreAddress = 0;

HX711 scale;



void resetSaleReadings() {
  for (int i; i < scaleReadingLength; i++) {
    scaleReadings[i] = 0;
  }
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

  long storedTareOffset = readTareOffset();
  Serial.print("stored offset "); Serial.println(storedTareOffset);
  scale.set_offset(storedTareOffset);
  Serial.print("reading offset back from scale "); Serial.println(scale.get_offset());

  resetSaleReadings();
}

void storeTareOffset(long offsetValue) {
  Serial.print("tare value to store "); Serial.println(offsetValue);
  EEPROM.put(tareValueStoreAddress, offsetValue);

  long checkStoredValue = readTareOffset();
  Serial.print("value stored and retrieved from eeprom is "); Serial.println(checkStoredValue);
}

long readTareOffset() {
  long storedValue;
  EEPROM.get(tareValueStoreAddress, storedValue);
  return storedValue;
}

void loop() {
  // take a reading every 500ms and store in array
  if (millis() - lastReadingMS > takeReadingIntervalMS && scaleReadingIndex < scaleReadingLength) {
    lastReadingMS = millis();
    scaleReadings[scaleReadingIndex++] = scale.get_units(3);
  }

  // send the scale reading only when we've taken 20 readings
  if (scaleReadingIndex == scaleReadingLength) {
      
      // compute average
      float totalReading = 0.0;
      for (int i = 0; i < scaleReadingLength; i++) {
        totalReading += scaleReadings[i];
      }
      float averageReading = totalReading / scaleReadingLength;
      scaleReadingIndex = 0;
      resetSaleReadings();
      Serial.print("weight grams:"); Serial.println(averageReading, 2);
  }

  // if the tare button stays low (debounce) for a defined period
  if (millis() - lastCheckTareButtonMS > checkTareIntervalMS) {
    // Serial.print("reading tare button "); Serial.println(digitalRead(TARE_BUTTON));
    lastCheckTareMS = millis();
    if (tareLowCount >= tareLowThreshold) {
      tareLowCount = 0;
      scale.tare();
      delay(100); // ?
      long offset = scale.get_offset();
      Serial.print("tare offset value from cell "); Serial.println(offset);
      storeTareOffset(offset);
      delay(100);
      Serial.print("reading offset back from cell "); Serial.println(scale.get_offset());
      scaleReadingIndex = 0;
      resetSaleReadings();
      Serial.println("TARE");
    }
    if (digitalRead(TARE_BUTTON) == LOW) {
      tareLowCount++;
    } else {
      tareLowCount = 0;
    }
  }
}