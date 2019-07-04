/*
 * Gerenciador de geradores a gasolina com Arduino (pt_BR)
 * 
 * Copyright 2019 Hugo Demiglio
 *
 * Veja mais detalhes sobre o projeto:
 * https://www.youtube.com/hugodemiglio
 * 
 * https://www.youtube.com/watch?v=XlI9G0b0AHg
 * https://www.youtube.com/watch?v=Yk3Xn9f3uEg
 * https://www.youtube.com/watch?v=Uj3ReuUT1XU
 *
 * Testado com Arduino Mega 2560 (IDE 1.8.8)
 * Verificar abaixo as dependencias de bibliotecas
 */

// Pins

#define key1 51
#define key2 53
#define key3 47
#define key4 49

#define relay_ignition 50
#define relay_energy 48

#define choke 46

// Defaults
#define temperature_limit 110
#define choke_temperature 40

#define min_voltage 80
#define max_voltage 130
#define max_power 900


// Libs
#include <EEPROM.h>
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
int engine_status = 0, energy_status = 0, screen = 0, mode = 0, ignition_status = 0, engine_message = 0;
int counter_id = 0, second_counter = 0;
int i = 0, timer = 0, blink = 0, last_second = 0, second = 0, minute = 0, hour = 0, day = 0, normal_voltage_counter = 0;
int engine_hour, engine_minute, engine_second;

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

  // Engine timer
  engine_hour = EEPROM.read(0);
  engine_minute = EEPROM.read(1);
  engine_second = EEPROM.read(2);

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
  lcd.print("      v 1.0.0");
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
    if(timer < 10) lcd.print(" ");
    if(timer < 100) lcd.print(" ");
    lcd.print(timer);
    lcd.print(" min");
    lcd.print("  -  ");
    break;
    case 2:
    lcd.setCursor(0,1);
    lcd.print("   Horas do motor   ");
    lcd.setCursor(0,2);

    lcd.print("      ");
    if(engine_hour < 10) lcd.print("0");
    lcd.print(engine_hour);
    lcd.print(":");
    if(engine_minute < 10) lcd.print("0");
    lcd.print(engine_minute);
    lcd.print(":");
    if(engine_second < 10) lcd.print("0");
    lcd.print(engine_second);

    break;
    case 3:
    lcd.setCursor(0,1);
    lcd.print(" Proporcao de oleo ");
    lcd.setCursor(0,2);
    if(engine_hour < 30) lcd.print("        30:1        ");
    else lcd.print("        50:1        ");
    break;
    case 4:
    lcd.setCursor(0,0);
    lcd.print("  Modo emergencia  ");
    lcd.setCursor(0,2);
    lcd.print("Ir modo para manual?");
    lcd.setCursor(0,4);
    lcd.print("  SIM          NAO  ");
    break;
    case 5:
    lcd.setCursor(0,0);
    lcd.print("    Dados brutos    ");
    lcd.setCursor(0,1);
    lcd.print(voltage);
    lcd.print(" V | ");
    lcd.print(power);
    lcd.print(" W  ");
    lcd.setCursor(0,2);
    lcd.print(current);
    lcd.print(" A | ");
    lcd.print(temperature);
    lcd.print(" C  ");
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

      if(temperature > temperature_limit) {
        lcd.setCursor(0,3);
        lcd.print("Motor muito quente! ");
      } else if(!choke_status && temperature < choke_temperature) {
        lcd.setCursor(0,3);
        lcd.print(" > Puxar afogador.   ");
      }
    } else {
      lcd.setCursor(0,2);

      if(!choke_status && temperature < choke_temperature) {
        lcd.print(" ! Puxar afogador ! ");
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
    lcd.print(" V   ");

    lcd.setCursor((power < 100 ? (power < 10 ? 14 : 13) : 12), 1);
    if(power > 0) lcd.print(power);
    else lcd.print("0.00");
    lcd.print(" W");

    lcd.setCursor(0,2);
    switch(engine_message){
      case 0:
      if(timer != 0 && timer <= 5) {
        lcd.print("Timer restante ");
        lcd.print(timer);
        lcd.print(" min");
      } else {
        lcd.print("                    ");
      }
      break;
      case 1:
      lcd.print(" > Soltar afogador  ");
      break;
      case 2:
      lcd.print("  ! Tensao baixa !  ");
      break;
      case 3:
      lcd.print("  ! Tensao alta !   ");
      break;
      case 4:
      lcd.print("! Temperatura alta !");
      break;
      case 5:
      lcd.print("Verificando motor...");
      break;
    }

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

    if(ignition_status == 0) {
      if(temperature < temperature_limit) {
        if(temperature < choke_temperature) {
          if(choke_status) change_ignition(1);
        } else {
          change_ignition(1);
        }
      }
    } else {
      if(voltage >= 40) {
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

    // Engine stoped
    if(voltage <= 10) {
      start_counter(5);
      engine_message = 5;

      if(counter_done(5, 10)){
        change_ignition(0);
      }
    } else {
      if(engine_message == 5) engine_message = 0;
      clear_counter(5);
    }

    // Low voltage
    if(voltage > 11 && voltage <= min_voltage) {
      start_counter(2);
      engine_message = 2;

      if(counter_done(2, 5)){
        change_energy(0);
      }
    } else {
      if(engine_message == 2) engine_message = 0;
      clear_counter(2);
    }

    // High voltage
    if(voltage >= max_voltage) {
      start_counter(3);
      engine_message = 3;

      if(counter_done(3, 5)){
        change_energy(0);
      }
    } else {
      if(engine_message == 3) engine_message = 0;
      clear_counter(3);
    }

    // Normal voltage
    if(energy_status == 0 && voltage > min_voltage && voltage < max_voltage){
      start_counter(10);

      if(counter_done(10, 5)){
        change_energy(1);
      }
    } else {
      clear_counter(10);
    }

    // Temperature warning
    if(temperature > (temperature_limit - 5)){
      engine_message = 4;
    } else if(engine_message == 4){
      engine_message = 0;
    }

    // Chock warning
    if(choke_status == 1 && engine_message == 0) engine_message = 1;
    if(choke_status == 0 && engine_message == 1) engine_message = 0;

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
    if(screen >= 6) screen = 0;
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
      case 4:
        if(key_status == 3) screen = 0;
        if(key_status == 2) manual_mode();
      break;
    }
  }

  if(key_status == 4 && engine_status == 1){
    change_ignition(0);

    lcd.clear();
    lcd.setCursor(0,1);
    lcd.print("Motor foi desligado");
    lcd.setCursor(0,2);
    lcd.print("Motivo: botao power");
    delay(2000);
    lcd.clear();
  }

  last_key = key_status;
}

void get_temperature(){
  temp_bus.requestTemperatures();
  temperature = temp_bus.getTempC(temp_sensor);
}

void manual_mode(){
  digitalWrite(relay_energy, LOW);
  digitalWrite(relay_ignition, LOW);

  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("     ! ATENCAO ! ");
  lcd.setCursor(0,2);
  lcd.print("Eletronica desligada");

  delay(5000);
  lcd.setBacklight(LOW);

  while(true){
    delay(1000);
  }
}

void clear_timers(){
  second = 0;
  minute = 0;
  hour = 0;
  day = 0;
}

void start_counter(int id){
  if(counter_id == 0) {
    second_counter = 0;
    counter_id = id;
  }
}

int counter_done(int id, int to){
  if(counter_id == id && second_counter >= to){
    counter_id = 0;
    return 1;
  }

  return 0;
}

void clear_counter(int id){
  if(counter_id == id) counter_id = 0;
}

void cron(){
  int current_second = millis()/1000;

  if(current_second > last_second || current_second < last_second){
    last_second = current_second;
    second++;

    blink = blink == 1 ? 0 : 1;

    if(second_counter < 15) second_counter++;
    if(second_counter == 15) counter_id = 0;

    if(second % 5 == 0) {
      if(ignition_status) get_energy();
    }

    if(second % 10 == 0) {
      get_temperature();
    }

    if(engine_status == 1) {
      engine_second++;
      EEPROM.write(2, engine_second);
    }
  }

  if(engine_status == 1){
    if(engine_second >= 60){
      engine_minute++;
      engine_second = 0;

      EEPROM.write(2, engine_second);
      EEPROM.write(1, engine_minute);
    }

    if(engine_minute >= 60){
      engine_hour = engine_hour + 1;
      engine_minute = 0;

      EEPROM.write(1, engine_minute);
      EEPROM.write(0, engine_hour);
    }
  }

  if(second >= 60){
    minute++;
    second = 0;

    if(engine_status == 1 && timer > 0){
      timer--;

      if(timer == 0) {
        change_ignition(0);

        lcd.clear();
        lcd.setCursor(0,1);
        lcd.print(" Motor foi desligado");
        lcd.setCursor(0,2);
        lcd.print("   Motivo: timer");
        delay(2000);
        lcd.clear();
      }
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
