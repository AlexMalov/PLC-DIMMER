#include "dimmerM.h"
#include <EEPROM.h>

const PROGMEM uint8_t power2time[] = {
     0,    0,  0,  11,  13,  14,  16,  17,  18,  19,
     20,  22, 23,  23,  24,  25,  26,  27,  28,  29,
     29,  30, 31,  32,  32,  33,  34,  35,  35,  36,
     37,  37, 38,  39,  39,  40,  41,  41,  42,  43,
     43,  44, 45,  45,  46,  47,  47,  48,  49,  50,
     50,  51, 51,  52,  52,  53,  54,  54,  55,  56,
     56,  57, 58,  58,  59,  60,  60,  61,  62,  62,
     63,  64, 64,  65,  66,  67,  67,  68,  69,  70,
     70,  71, 72,  73,  74,  75,  76,  77,  78,  79,
     80,  81, 82,  83,  84,  85,  87,  89,  90,  93,
     100
};

DimmerM::DimmerM(uint8_t pin1, uint8_t index) {
  _index = index; 
  pin = pin1;
  digitalWrite(pin, LOW);
  pinMode(pin, OUTPUT);
  defPower = 30;
  _power = 0;
  setRampTime(2);
  //setOn();
}

void DimmerM::setPower(uint8_t power){
  _newPower = constrain( power, 0, 100);
  if (_newPower) defPower = _newPower;
  _rampCounter = 0;
  _rampStartPower = _power;
  triakTime = pgm_read_byte(&power2time[_power]);
  save2EEPROM();
}
    
void DimmerM::setRampTime(uint16_t rampTime){
  _rampCycles = rampTime * 2 * acFreq;
  _rampCounter = _rampCycles;
}

uint8_t DimmerM::getRampTime(){
  return _rampCycles /(2 * acFreq);
}

void DimmerM::zeroCross(){
  if (_rampCycles) _power = (_rampStartPower + ((int32_t) _newPower - _rampStartPower) * _rampCounter / _rampCycles) & _isOn;
    else _power = 0;
  triakTime = pgm_read_byte(&power2time[_power]);
  if (_rampCounter < _rampCycles ) _rampCounter++;
}

void DimmerM::save2EEPROM(){
  EEPROM.update(_index*2, _newPower);
  EEPROM.update(_index*2+1, _isOn);
  EEPROM.update(0, 18);  
}

void DimmerM::loadEEPROM(){
  setPower(EEPROM[_index*2]);
  _isOn =  EEPROM[_index*2+1];
//  if (EEPROM[_index*2+1]) setOn(); 
//    else setOff();
}

uint8_t DimmerM::getPower(){
  return _newPower;
}

void DimmerM::setOn(){
  _isOn = 0xff;
  //setPower(defPower);
}

void DimmerM::setOff(){
  _isOn = 0;
  //defPower = _newPower;
  //setPower(0);
}
