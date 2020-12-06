#ifndef dimmerM
#define dimmerM

#include "Arduino.h"

#define acFreq 50                      // частота питающей сети в Герцах
#define zeroPeriod 1000/(2*acFreq)     // период перехода через ноль (для 50 Гц - 10 мс)

class DimmerM
{
  private:
    uint8_t _power;            // текущая мощность диммера от 0 до 100%
    uint8_t _newPower;         // новое значение мощности для fade эффекта
    uint8_t _rampCounter;     // счетчик оставшегося времени до завершения fade - эффекта в полупериодах сетевой частоты
    uint8_t _rampCycles;      // время изменения мощности для fade эффекта в полупериодах
    uint8_t _rampStartPower;
    uint8_t _index;           // номер диммера
    byte _isOn;                // состояние диммер - влючен или выключен
    
  public:
    DimmerM(uint8_t pin1, uint8_t index);
    uint8_t pin;              // pin Диммера
    uint8_t triakTime;        // мощность переведенная во время закрытия семистора
    uint8_t defPower;         // значение мощности после включения
    void setOn();
    void setOff();
    void setPower(uint8_t power);
    void setRampTime(uint16_t rampTime); // ramtTime в секундах
    void zeroCross();
    void save2EEPROM();
    void loadEEPROM();
    uint8_t getRampTime();
    uint8_t getPower();
};


#endif
