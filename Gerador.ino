#include "EmonLib.h"             // Include Emon Library
EnergyMonitor emon1;             // Create an instance

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,2,1,0,4,5,6,7,3, POSITIVE);

void setup()
{
  Serial.begin(9600);
  lcd.begin(20, 4);
  lcd.clear();

  emon1.voltage(0, 211.6, 1.7);  // Voltage: input pin, calibration, phase_shift
  emon1.current(1, 29);       // Current: input pin, calibration.
}

void loop()
{
  emon1.calcVI(20, 2000);         // Calculate all. No.of half wavelengths (crossings), time-out
  // emon1.serialprint();           // Print out all variables (realpower, apparent power, Vrms, Irms, power factor)

  float realPower       = emon1.realPower;        //extract Real Power into variable
  float apparentPower   = emon1.apparentPower;    //extract Apparent Power into variable
  float powerFActor     = emon1.powerFactor;      //extract Power Factor into Variable
  float supplyVoltage   = emon1.Vrms;             //extract Vrms into Variable
  float Irms            = emon1.Irms;             //extract Irms into Variable

  lcd.setCursor(0,0);
  lcd.print(supplyVoltage);
  lcd.print(" V");

  lcd.setCursor(0,1);
  lcd.print(realPower);
  lcd.print(" W");

  lcd.setCursor(0,2);
  lcd.print(Irms);
  lcd.print(" A");

  Serial.print(supplyVoltage);
  Serial.println(" V");
  delay(1000);
}

// #include "EmonLib.h"
// #include <Wire.h>
// #include <LiquidCrystal_I2C.h>
//
// #define current_pin 1
// #define voltage_pin A0
//
// #define key1 51
// #define key2 53
// #define key3 47
// #define key4 49
//
// EnergyMonitor current_monitor;
// LiquidCrystal_I2C lcd(0x27,2,1,0,4,5,6,7,3, POSITIVE);
//
// int seconds = 0;
// double total_current;
//
// int voltage = 121.0, current = 0;
//
// double sensorValue = 0, sensorValue1 = 0, VmaxD = 0, VeffD, Veff;
// int crosscount = 0, climbhill = 0;
//
// int key1_status, key2_status, key3_status, key4_status;
// int screen_clear = 1;
//
// void setup(){
//   // display
//   lcd.begin(20, 4);
//   lcd.clear();
//   Serial.begin(9600);
//
//   // current meter
//   current_monitor.current(current_pin, 29);
//
//   // keypad
//   pinMode(key1, INPUT_PULLUP);
//   pinMode(key2, INPUT_PULLUP);
//   pinMode(key3, INPUT_PULLUP);
//   pinMode(key4, INPUT_PULLUP);
// }
//
// void loop() {
//   key1_status = digitalRead(key1);
//   key2_status = digitalRead(key2);
//   key3_status = digitalRead(key3);
//   key4_status = digitalRead(key4);
//
//   Serial.print(key1_status);
//   Serial.print(key2_status);
//   Serial.print(key3_status);
//   Serial.println(key4_status);
//
//   getVoltage();
//
//   if(!key1_status) {
//     clearScreen();
//     lcd.print("Botao 1");
//     delay(1000);
//   } else if(!key2_status) {
//     clearScreen();
//     lcd.print("Botao 2");
//     delay(1000);
//   } else if(!key3_status) {
//     clearScreen();
//     lcd.print("Botao 3");
//     delay(1000);
//   } else if(!key4_status) {
//     clearScreen();
//     lcd.print("Botao 4");
//     delay(1000);
//   } else {
//     get_current();
//   }
// }
//
// void get_current() {
//   if(screen_clear){
//     lcd.clear();
//     lcd.setCursor(0,0);
//     lcd.print("Corrente (A):");
//     lcd.setCursor(0,1);
//     lcd.print("Potencia (W):");
//     lcd.setCursor(0,2);
//     lcd.print("0.0 kWh");
//     screen_clear = 0;
//   }
//
//   //Calcula a corrente
//   double Irms = current_monitor.calcIrms(1480);
//   //Mostra o valor da corrente
//   Serial.print("Corrente : ");
//   Serial.print(Irms); // Irms
//   lcd.setCursor(14,0);
//   lcd.print(Irms);
//
//   //Calcula e mostra o valor da potencia
//   Serial.print(" Potencia : ");
//   Serial.println(Irms*voltage);
//   lcd.setCursor(14,1);
//   lcd.print("      ");
//   lcd.setCursor(14,1);
//   lcd.print(Irms*voltage,1);
//
//   total_current = total_current + (Irms*voltage);
//   seconds++;
//
//   lcd.setCursor(0,2);
//   lcd.print(total_current / 3600000);
//   lcd.print(" kWh");
//
//   lcd.print(" - R$ ");
//   lcd.print((total_current / 3600000) * 0.71);
//
//   lcd.setCursor(0,3);
//   lcd.print((Irms*voltage) / 1000);
//   lcd.print(" kWh");
//
//   lcd.print(" - R$ ");
//   lcd.print( ((Irms*voltage) / 1000) * 0.71);
//
//   delay(1000);
// }
//
// void getVoltage(){
//   sensorValue1 = sensorValue;
//
//   delay(100);
//   sensorValue = analogRead(A0);
//
//   if(sensorValue > sensorValue1 && sensorValue > 511){
//     climbhill = 1;
//     VmaxD = sensorValue;
//     // Serial.println(sensorValue);
//   }
//
//   if(sensorValue < sensorValue1 && climbhill == 1){
//     climbhill = 0;
//     VmaxD = sensorValue1;
//     VeffD = VmaxD/sqrt(2);
//     Veff = (((VeffD-420.76)/-90.24)*-210.2)+210.2;
//
//     Serial.print("Voltage: ");
//     Serial.println(Veff);
//     VmaxD = 0;
//   }
// }
//
// void clearScreen(){
//   lcd.clear();
//   lcd.setCursor(0,0);
//   screen_clear = 1;
// }
