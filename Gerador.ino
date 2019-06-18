// Pins

#define key1 51
#define key2 53
#define key3 47
#define key4 49

#define relay_ignition 50
#define relay_energy 48

#define choke 46

// Defaults
#define temperature_limit 100
#define choke_temperature 40

#define min_voltage 40
#define max_voltage 130
#define max_power 900

#include <Wire.h>

#include "EmonLib.h"
EnergyMonitor emon1;

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,2,1,0,4,5,6,7,3, POSITIVE);

#include <OneWire.h>
#include <DallasTemperature.h>

// Keypad
int key_status, last_key = 0, key1_status, key2_status, key3_status, key4_status;

// Choke
int choke_status;

// Energy Monitor
float power, voltage, current;

// System Status
int engine_status = 0, energy_status = 0, screen = 0, mode = 0, ignition_status = 0;
int low_voltage_counter = 0, high_voltage_counter = 0, normal_voltage_counter = 0;
int i = 0, timer = 0, blink = 0, last_second = 0, second = 0, minute = 0, hour = 0, day = 0;

// Thermometer
OneWire temp_pin(3);
DallasTemperature temp_bus(&temp_pin);
DeviceAddress temp_sensor;
float temperature;

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

  // Relays
  pinMode(relay_ignition, OUTPUT);
  pinMode(relay_energy, OUTPUT);

  // Choke
  pinMode(choke, INPUT_PULLUP);

  // Thermometer
  temp_bus.begin();
  temp_bus.getAddress(temp_sensor, 0);

  init_system();
}

void init_system(){
  digitalWrite(relay_ignition, HIGH);
  digitalWrite(relay_energy, HIGH);

  get_temperature();

  lcd.setCursor(0,0);
  lcd.print(" Gerador de Energia");
  lcd.setCursor(0,1);
  lcd.print("      HG Brasil");
  lcd.setCursor(0,3);
  lcd.print("       v 1.0.0");
  delay(2000);
  lcd.clear();
}

void loop(){
  get_inputs();

  cron();
  engine_cron();

  switch (screen) {
    case 0:
    engine_screen();
    break;
    case 1:
    lcd.setCursor(0,0);
    lcd.print("    Temporizador    ");
    lcd.setCursor(0,1);
    lcd.print(" desligar motor em  ");
    lcd.setCursor(0,3);
    lcd.print("   +   ");
    lcd.print(timer);
    lcd.print(" min   -  ");
    break;
    case 2:
    lcd.setCursor(0,1);
    lcd.print("   Horas do motor   ");
    lcd.setCursor(0,2);
    lcd.print("      00:00:00      ");
    break;
    case 3:
    lcd.setCursor(0,0);
    lcd.print("    Dados brutos    ");
    break;
  }
}

void engine_screen(){
  if(engine_status == 0) {
    lcd.setCursor(0,0);
    lcd.print("   Motor desligado  ");
    lcd.setCursor(0,1);
    lcd.print("Temp. motor: ");
    lcd.print(temperature);
    lcd.print(" C");

    if(ignition_status == 0) {
      lcd.setCursor(0,2);
      lcd.print("Partida n permitida ");

      if(temperature > 100) {
        lcd.setCursor(0,3);
        lcd.print("Motor muito quente! ");
      } else if(!choke_status && temperature < choke_temperature) {
        lcd.setCursor(0,3);
        lcd.print("> Puxar afogador.   ");
      }
    } else {
      lcd.setCursor(0,2);

      if(!choke_status && temperature < choke_temperature && blink == 1) {
        lcd.print("   Puxar afogador!  ");
      } else {
        lcd.print("                    ");
      }

      lcd.setCursor(0,3);
      lcd.print("> Aguardando partida");
    }

  } else {
    lcd.setCursor(0,0);
    lcd.print("    Motor ligado    ");

    lcd.setCursor(0,1);
    lcd.print(voltage);
    lcd.print(" V | ");
    lcd.print(power);
    lcd.print(" W   ");

    lcd.setCursor(0,3);
    lcd.print(" ");
    lcd.print(temperature);
    lcd.print(" C ");

    lcd.setCursor(11,3);
    if(hour < 10) lcd.print("0");
    lcd.print(hour);
    lcd.print(":");
    if(minute < 10) lcd.print("0");
    lcd.print(minute);
    lcd.print(":");
    if(second < 10) lcd.print("0");
    lcd.print(second);
  }
}

void engine_cron(){
  if(engine_status == 0) {

    if(ignition_status == 0 && temperature < temperature_limit) {
      if(temperature < choke_temperature) {
        if(choke_status) change_ignition(1);
      } else {
        change_ignition(1);
      }
    } else {
      if(voltage >= min_voltage) {
        engine_status = 1;
        clear_timers();
        lcd.clear();
      }
    }

  } else {

    // High temperature
    if(temperature > temperature_limit){
      change_ignition(0);
    }

    // Low voltage
    if(voltage <= min_voltage) {
      if(blink == 1) low_voltage_counter++;

      if(low_voltage_counter >= 5){
        change_ignition(0);
        low_voltage_counter = 0;
      }
    } else {
      low_voltage_counter = 0;
    }

    // High voltage
    if(voltage >= max_voltage) {
      if(blink == 1) high_voltage_counter++;

      if(high_voltage_counter >= 3){
        change_energy(0);
      }
    } else {
      high_voltage_counter = 0;
    }

    // Normal voltage
    if(energy_status == 0 && voltage > min_voltage && voltage < max_voltage){
      if(blink == 1) normal_voltage_counter++;

      if(normal_voltage_counter >= 5){
        change_energy(1);
        normal_voltage_counter = 0;
      }
    }

  }
}

void change_ignition(int to){
  if(to == 1){
    digitalWrite(relay_ignition, LOW);
    ignition_status = 1;
  } else {
    digitalWrite(relay_ignition, HIGH);
    ignition_status = 0;
    engine_status = 0;
    timer = 0;
    change_energy(0);
  }
}

void change_energy(int to){
  if(to == 1){
    digitalWrite(relay_energy, LOW);
    energy_status = 1;
  } else {
    digitalWrite(relay_energy, HIGH);
    energy_status = 0;
  }
}

void get_energy(){
  emon1.calcVI(20, 2000);

  power = emon1.realPower;
  voltage = emon1.Vrms;
  current = emon1.Irms;
}

void get_inputs(){
  key1_status = digitalRead(key1);
  key2_status = digitalRead(key2);
  key3_status = digitalRead(key3);
  key4_status = digitalRead(key4);

  choke_status = digitalRead(choke) ? 0 : 1;

  if(!key1_status) {
    key_status = 1;
  } else if(!key2_status) {
    key_status = 2;
  } else if(!key3_status) {
    key_status = 3;
  } else if(!key4_status) {
    key_status = 4;
  } else {
    key_status = 0;
  }

  if(key_status == 1 && last_key != 1) {
    screen++;
    lcd.clear();
    if(screen >= 4) screen = 0;
  }

  if(key_status != last_key){
    switch (screen) {
      case 1:
        if(timer < 10) i = 1;
        else if(timer < 60) i = 5;
        else i = 20;

        if(key_status == 2) timer += i;
        if(key_status == 3) timer -= i;
        if(timer < 0) timer = 0;
      break;
    }
  }

  if(key_status == 4 && engine_status == 1){
    change_ignition(0);
  }

  last_key = key_status;
}

void get_temperature(){
  temp_bus.requestTemperatures();
  temperature = temp_bus.getTempC(temp_sensor);
}

void clear_timers(){
  second = 0;
  minute = 0;
  hour = 0;
  day = 0;
}

void cron(){
  int current_second = millis()/1000;

  if(current_second > last_second || current_second < last_second){
    last_second = current_second;
    second++;

    blink = blink == 1 ? 0 : 1;

    if(second % 5 == 0) {
      if(ignition_status) get_energy();
    }

    if(second % 10 == 0) {
      get_temperature();
    }
  }

  if(second >= 60){
    minute++;
    second = 0;

    if(engine_status == 1 && timer > 0){
      timer--;
      if(timer == 0) change_ignition(0);
    }
  }

  if(minute >= 60){
    hour = hour + 1;
    minute = 0;
  }

  if(hour >= 24){
    day = day + 1;
    hour = 0;
  }

  if(hour < 0){
    hour = 0;
  }

  if(minute < 0){
    minute = 0;
  }

}
