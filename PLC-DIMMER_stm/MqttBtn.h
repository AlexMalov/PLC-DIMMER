#ifndef _mqttBtn
#define _mqttBtn

#include "Arduino.h"
#include <GyverButton.h>

class MqttBtn: public GButton
{
  private:
    uint8_t _isOn;            // состояние диммер - влючен или выключен
  public: 
    MqttBtn(uint8_t pin1, uint8_t idx);
    char* mqtt_topic_state;
    bool useMQTT = false;
    bool mqttRegistered = false;      // канал зарегисрирован на MQTT
    uint8_t index;           // номер кнопки
    //uint8_t pin;              // pin Кнопки
    void setOn();
    void setOff();
    void toggle();
    void loadEEPROM();
    void saveEEPROM();
    bool getOnOff();
};

#endif
