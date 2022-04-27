#ifndef _triacPLC
  #define _triacPLC

//#define DEBUGPLC
// Значения по умолчанию
#define def_staticIP 192, 168, 0, 211                   // Утановить IP адресс для этой Arduino (должен быть уникальным в вашей сети)
#define def_staticDNS 192, 168, 0, 1
#define def_staticGW 192, 168, 0, 1
#define def_staticMASK 255, 255, 255, 0
#define def_macAddr { 0x90, 0xB2, 0xFA, 0x0D, 0x4E, 0x59 }  // Установить MAC адресс для этой Arduino (должен быть уникальным в вашей сети)
#define def_useDHCP false

#define def_mqtt_user "mqtt"                            // ИмяКлиента, Логин и Пароль для подключения к MQTT брокеру
#define def_mqtt_pass "mqtt"
#define def_mqtt_clientID "plc1Traic"
#define def_HomeAssistantDiscovery "HomeAssistant"
#define def_HABirthTopic "homeassistant/status"
#define def_mqtt_brokerIP 192,168,0,124   // IP адресс MQTT брокера по умочанию

#include "Arduino.h"
#include "dimmerM.h"
#include "MqttBtn.h"

#include <EthernetENC.h>
#include <PubSubClient.h>

#define zeroFreq100 2*acFreq*100         // частота прерывания для таймера (для 50 Hz - 10 000 Hz)
#define zeroDetectorPin PA0             // пин детектора нуля
#define channelAmount 9                 // количество диммеров
#define btnsAmount 9                    // количество входов PLC

struct inet_cfg_t {
  bool use_dhcp;
  uint8_t dhcp_refresh_minutes;
  uint8_t mac[6];
  uint8_t ip[4];
  uint8_t gw[4];
  uint8_t mask[4];
  uint8_t dns[4];
  uint16_t webSrvPort;
};

struct mqtt_cfg_t {
  bool useMQTT;
  char User[21];
  char Pass[9];
  char ClientID[21];
  char HADiscover[14];    // "HomeAssistant"
  uint8_t SrvIP[4];        
  uint16_t SrvPort;       //1883
  char HABirthTopic[21];  // "homeassistant/status"
  bool isHAonline = false;       // curent HA online status 
};

class triacPLC
{
  private:
    DimmerM _dimmers[channelAmount] = {DimmerM(PA1,1), DimmerM(PA2,2), DimmerM(PA3,3), DimmerM(PA4,4), DimmerM(PA5,5), DimmerM(PA6,6), DimmerM(PA7,7), DimmerM(PB0,8), DimmerM(PB1,9)};
    MqttBtn _btns[btnsAmount] = {MqttBtn(PB9,1), MqttBtn(PA11,2), MqttBtn(PA10,3), MqttBtn(PA9,4), MqttBtn(PA8,5), MqttBtn(PB15,6), MqttBtn(PB14,7), MqttBtn(PB13,8), MqttBtn(PB12,9)};  // кнопки входов PLC
    volatile uint8_t _counter;                                                     // счётчик цикла диммирования
    volatile uint8_t _zeroCrossCntr = 10; 
    volatile uint32_t _zeroCrossTime;    
    bool _doEthernet();
    bool _doMqtt();
    bool _mqttReconnect();
    bool _haDiscover();                                 // регистрация канала за каналом
    void _doButtonsDimmers();
  public:
    triacPLC();
    inet_cfg_t inet_cfg;
    mqtt_cfg_t mqtt_cfg;
    volatile int16_t ACfreq = 0;
    bool flipDisplay = false;                                                 // переворачивать дисплей
    void rtDisplay();                                                         // переворачивает дисплей
    bool begin(MQTT_CALLBACK_SIGNATURE = NULL);                               //инициализация дисплея и сетевой карты
    void mqttCallback(char* topic, byte* payload, uint16_t length);
    bool processEvents();
    void setDefInetCfg();
    void setDefMqttCfg();
    void display();
    void showFreq();
    void zeroCrossISR();
    void timerISR();
    void allOff();
    void setOn(uint8_t channel);
    void setOff(uint8_t channel);
    void toggle(uint8_t channel);
    void setPower(uint8_t channel, uint8_t power);
    void incPower(uint8_t channel);
    void setRampTime(uint8_t channel, uint16_t rampTime); // ramtTime в секундах
    void loadEEPROM();
    void saveEEPROM();
    uint8_t getRampTime(uint8_t channel);
    uint8_t getPower(uint8_t channel);
    bool getBtnState(uint8_t btn_idx);
    bool getOnOff(uint8_t channel);
    void set_useMQTT(bool new_useMQTT);
};

#endif
