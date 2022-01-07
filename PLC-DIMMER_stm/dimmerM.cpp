#include "dimmerM.h"

const PROGMEM uint8_t power2time[] = {
     0,    0,  0,  0,  0,  0,  0,  17,  18,  19,
     20,  22, 23,  23,  24,  25,  26,  27,  28,  29,
     29,  30, 31,  32,  32,  33,  34,  35,  35,  36,
     37,  37, 38,  39,  39,  40,  41,  41,  42,  43,
     43,  44, 45,  45,  46,  47,  47,  48,  49,  50,
     50,  51, 51,  52,  52,  53,  54,  54,  55,  56,
     56,  57, 58,  58,  59,  60,  60,  61,  62,  62,
     63,  64, 64,  65,  66,  67,  67,  68,  69,  70,
     70,  71, 72,  73,  74,  75,  76,  77,  78,  79,
     80,  81, 82,  83,  84,  85,  87,  89,  90,  93,
     99
};

DimmerM::DimmerM(uint8_t pin1, uint8_t idx) {
  index = idx; 
  pin = pin1;
  pinMode(pin, OUTPUT);
  _power = 0;
  _incDir = true;
  setRampTime(2);
}

void DimmerM::setPower(uint8_t power){
  _newPower = constrain(power, 0, 100);
  if (_newPower >= 100) _incDir = false;
  if (_newPower == 0) _incDir = true;
  _rampCounter = 0;
  _rampStartPower = _power;
}

void DimmerM::incPower(){
  if (_newPower >= 100) _incDir = false;
  if (_newPower == 0) _incDir = true;
  if (_incDir) _newPower++; else _newPower--;
}
    
void DimmerM::setRampTime(uint16_t rampTime){
  _rampCycles = rampTime * 2 * acFreq;
  _rampCounter = _rampCycles;
}

uint8_t DimmerM::getRampTime(){
  return _rampCycles /(2 * acFreq);
}

void DimmerM::zeroCross(){
  _power = _rampStartPower + ((int32_t) ((_newPower & _isOn) - _rampStartPower)) * _rampCounter / _rampCycles;
  triacTime = pgm_read_byte(&power2time[_power]);
  if (_rampCounter < _rampCycles ) _rampCounter++;
}

void DimmerM::saveEEPROM(){
  if ( !useMQTT ) EEPROM.update(index*2, _newPower);
  EEPROM.update(0, EEPROMdataVer);
}

void DimmerM::loadEEPROM(){
  if (EEPROM[0] != EEPROMdataVer) return;  // в EEPROM ничего еще не сохраняли
  setPower(EEPROM[index*2]);
  if (EEPROM[index*2+1]) _isOn = 0xff; else _isOn = 0;
}

uint8_t DimmerM::getPower(){
  return _newPower;
}
 
bool DimmerM::getOnOff(){
  return _isOn;
}

void DimmerM::setOn(){
  _isOn = 0xff;
  _rampCounter = 0;
  _rampStartPower = _power;
  if ( !useMQTT ) EEPROM.update(index*2+1, _isOn);
  EEPROM.update(0, EEPROMdataVer);
}

void DimmerM::setOff(){
  _isOn = 0;
  _rampCounter = 0;
  _rampStartPower = _power;
  if ( !useMQTT ) EEPROM.update(index*2+1, _isOn);
  EEPROM.update(0, EEPROMdataVer);
}

void DimmerM::toggle(){
  _isOn = ~_isOn;
  _rampCounter = 0;
  _rampStartPower = _power;
  if ( !useMQTT ) EEPROM.update(index*2+1, _isOn);
  EEPROM.update(0, EEPROMdataVer);  
}
