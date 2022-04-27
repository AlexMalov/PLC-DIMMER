#include "WebPages.h"

HardwareTimer *MyTim;

void mqttCallback(char* topic, byte* payload, uint16_t length) {
  myPLC.mqttCallback(topic, payload, length);
}

void setup() {
  //pinMode(PC13, OUTPUT);
  #ifdef DEBUGPLC 
    Serial.begin(115200); 
  #endif
  if ( myPLC.begin(mqttCallback) ) websetup();
  attachInterrupt(digitalPinToInterrupt(zeroDetectorPin), zeroCrossISR, CHANGE); // подключаем прерывание детектора пререхода через 0
  #if defined(TIM1)                    //HardwareTimer
    TIM_TypeDef *Instance = TIM1;
  #else 
    TIM_TypeDef *Instance = TIM2;
  #endif 
  MyTim = new HardwareTimer(Instance); // Instantiate HardwareTimer object. Thanks to 'new' instanciation, HardwareTimer is not destructed when setup() function is finished.
  MyTim->setOverflow(zeroFreq100, HERTZ_FORMAT);      // 10 kHz
  MyTim->attachInterrupt(timerISR);                   // bind argument to callback: When Update_IT_callback is called MyData will be given as argument
  MyTim->resume(); 
}

void loop() {
  static uint32_t endTm1 = 200;
  myPLC.processEvents();
  static int len = 200;
  static char buff[200]; 
  if (Ethernet.hardwareStatus() != EthernetNoHardware) webserver.processConnection(buff, &len);
  if (millis() < endTm1) return;
  endTm1 = millis() + 500;
  myPLC.showFreq();
  //digitalWrite(PC13, !digitalRead(PC13));
}

void zeroCrossISR(){
  myPLC.zeroCrossISR();
  MyTim->setCount(0);
}

void timerISR(){
  myPLC.timerISR();
}
