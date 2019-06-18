#define key1 51
#define key2 53
#define key3 47
#define key4 49

#include <Wire.h>

#include "EmonLib.h"
EnergyMonitor emon1;

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,2,1,0,4,5,6,7,3, POSITIVE);

// Keypad
int key1_status, key2_status, key3_status, key4_status;

// Energy Monitor
float power, voltage, current;

void setup(){
  Serial.begin(9600);
  lcd.begin(20, 4);
  lcd.clear();

  // Energy Monitor
  emon1.voltage(0, 211.6, 1.7);
  emon1.current(1, 29);

  // Keypad
  pinMode(key1, INPUT_PULLUP);
  pinMode(key2, INPUT_PULLUP);
  pinMode(key3, INPUT_PULLUP);
  pinMode(key4, INPUT_PULLUP);
}

void loop(){
  get_energy();
  get_keypad();

  lcd.setCursor(19,0);
  if(!key1_status) {
    lcd.print("1");
  } else if(!key2_status) {
    lcd.print("2");
  } else if(!key3_status) {
    lcd.print("3");
  } else if(!key4_status) {
    lcd.print("4");
  } else {
    lcd.print("0");
  }

  lcd.setCursor(0,0);
  lcd.print(voltage);
  lcd.print(" V");

  lcd.setCursor(0,1);
  lcd.print(power);
  lcd.print(" W");

  lcd.setCursor(0,2);
  lcd.print(current);
  lcd.print(" A");

  lcd.setCursor(0,3);
  lcd.print("0.0");
  lcd.print(" C");

  Serial.print(voltage);
  Serial.println(" V");

  delay(1000);
}

void get_energy(){
  emon1.calcVI(20, 2000);

  power = emon1.realPower;
  voltage = emon1.Vrms;
  current = emon1.Irms;
}

void get_keypad(){
  key1_status = digitalRead(key1);
  key2_status = digitalRead(key2);
  key3_status = digitalRead(key3);
  key4_status = digitalRead(key4);
}
