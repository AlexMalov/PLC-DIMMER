#include <EEPROM.h>
bool doButtonsDimmers(){
  for (byte i = 0; i < btnsAmount; i++)
    myBtns[i].tick();
}

void saveDimmers2EEPROM(){
  //EEPROM.update(0, 18);        // ставим в EEPROM флаг сохраняли
  for (byte i = 0; i < channelAmount; i++)
    dimmers[i].save2EEPROM();  
}

void loadDimmersEEPROM(){
  if (EEPROM[0] != 18) return;        // в EEPROM ничего еще не сохраняли
  for (byte i = 0; i < channelAmount; i++)
    dimmers[i].loadEEPROM();
}
