#include "MqttBtn.h"
#include <EEPROM.h>
#define EEPROMdataVer 20

MqttBtn::MqttBtn(uint8_t pin1, uint8_t idx): GButton(pin1) {
  index = idx; 
  //pin = pin1;
  //MqttBtn(pin);
  //pinMode(pin, OUTPUT);
  //GBtn = GButton(pin);
}


void MqttBtn::saveEEPROM(){
  //if ( !useMQTT ) EEPROM.update(index*2, _newPower);
  EEPROM.update(0, EEPROMdataVer);
}

void MqttBtn::loadEEPROM(){
  if (EEPROM[0] != EEPROMdataVer) return;  // в EEPROM ничего еще не сохраняли
  if (EEPROM[index*2+1]) _isOn = 0xff; else _isOn = 0;
}

bool MqttBtn::getOnOff(){
  return _isOn;
}

void MqttBtn::setOn(){
  _isOn = 0xff;
  if ( !useMQTT ) EEPROM.update(index*2+1, _isOn);
  EEPROM.update(0, EEPROMdataVer);
}

void MqttBtn::setOff(){
  _isOn = 0;
  if ( !useMQTT ) EEPROM.update(index*2+1, _isOn);
  EEPROM.update(0, EEPROMdataVer);
}

void MqttBtn::toggle(){
  _isOn = ~_isOn;
  if ( !useMQTT ) EEPROM.update(index*2+1, _isOn);
  EEPROM.update(0, EEPROMdataVer);  
}
