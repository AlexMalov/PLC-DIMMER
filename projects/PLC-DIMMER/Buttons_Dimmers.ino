#include <EEPROM.h>
bool doButtonsDimmers(){
  for (byte i = 0; i < btnsAmount; i++)
    myBtns[i].tick();
}

void loadDimmersEEPROM(){       
  for (byte i = 0; i < channelAmount; i++)
    dimmers[i].loadEEPROM();
}
