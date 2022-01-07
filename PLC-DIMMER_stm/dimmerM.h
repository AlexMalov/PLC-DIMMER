#ifndef dimmerM
#define dimmerM
#define EEPROMdataVer 20

#include "Arduino.h"
#include <EEPROM.h>

#define acFreq 50                      // частота питающей сети в Герцах
#define zeroPeriod 1000/(2*acFreq)     // период перехода через ноль (для 50 Гц - 10 мс)

class DimmerM
{
  private:
    uint8_t _power;           // текущая мощность диммера от 0 до 100%
    uint8_t _newPower;        // новое значение мощности для fade эффекта
    uint8_t _rampCounter;     // счетчик оставшегося времени до завершения fade - эффекта в полупериодах сетевой частоты
    uint8_t _rampCycles;      // время изменения мощности для fade эффекта в полупериодах
    uint8_t _rampStartPower;
    uint8_t _isOn;            // состояние диммер - влючен или выключен
    bool _incDir;             // направления инкремента мощности
  public:
    DimmerM(uint8_t pin1, uint8_t idx);
    char* mqtt_topic_state;
    char* mqtt_topic_bri_state;
    bool useMQTT = false;
    bool mqttRegistered = false;      // канал зарегисрирован на MQTT
    uint8_t index;           // номер диммера
    uint8_t pin;              // pin Диммера
    uint8_t triacTime;        // мощность переведенная во время закрытия семистора
    void setOn();
    void setOff();
    void toggle();
    void setPower(uint8_t power);
    void incPower();
    void setRampTime(uint16_t rampTime); // ramtTime в секундах
    void zeroCross();
    void loadEEPROM();
    void saveEEPROM();
    uint8_t getRampTime();
    uint8_t getPower();
    bool getOnOff();
};


#endif
