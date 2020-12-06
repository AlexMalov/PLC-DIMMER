#define zeroDetectorPin 2               // пин детектора нуля
#define channelAmount 7                 // количество диммеров
#define btnsAmount 4                    // количество входов PLC
#define zeroPeriod1 zeroPeriod*1000/100 // период прерывания для таймера (для 50 Гц - 100 мкс)

//#include <iarduino_OLED_txt.h>                                         // Подключаем библиотеку iarduino_OLED_txt.
//iarduino_OLED_txt myOLED(0x3C);                                        // Объявляем объект myOLED, указывая адрес дисплея на шине I2C: 0x3C или 0x3D.                                                                       //
//extern uint8_t MicroFont[];
 
#include <PubSubClient.h>
#include <EthernetENC.h>

#include <GyverTimers.h>    // библиотека таймера
#include <GyverButton.h>
#include "dimmerM.h"

DimmerM dimmers[channelAmount] = {DimmerM(3,1), DimmerM(4,2), DimmerM(5,3), DimmerM(6,4), DimmerM(7,5), DimmerM(8,6), DimmerM(9,7)};
volatile byte counter;                                                     // счётчик цикла диммирования
GButton myBtns[btnsAmount] = {GButton(A0), GButton(A1), GButton(A2), GButton(A3)};       // кнопки входов PLC

void setup() {
  pinMode(zeroDetectorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(zeroDetectorPin), handleInt0, FALLING);    //pin 2 int0
  Timer2.enableISR();
  Timer2.setPeriod(zeroPeriod1);
  //myOLED.begin();                             // запускаем дисплей
  Serial.begin(115200);
  loadDimmersEEPROM();
  //myOLED.clrScr();                                          //Очищаем буфер дисплея.
  //myOLED.setFont(MicroFont);                                //Перед выводом текста необходимо выбрать шрифт
  //myOLED.rotateDisplay(1);
  //myOLED.print(F("HELLO, MY FRIEND!"), 0, 0);
  //myOLED.print(F("\115\105\130\101\124\120\117\110\40\104\111\131"), 0, 4);

  // start the Ethernet connection:
  const byte mac[] = { 0x90, 0xB2, 0xFA, 0x0D, 0x4E, 0x59 };  // Установить MAC адресс для этой Arduino (должен быть уникальным в вашей сети)
  const IPAddress ip(192, 168, 0, 211);                       // Утановить IP адресс для этой Arduino (должен быть уникальным в вашей сети)
  const IPAddress myDns(192, 168, 0, 1);
  const IPAddress gateway(192, 168, 0, 1);
  const IPAddress subnet(255, 255, 255, 0);

  Serial.println(F("Init Ether with DHCP:"));
  //myOLED.print(F("INIT NET CARD..."), 0, 1);
  Ethernet.begin(mac, ip, myDns, gateway);
  if (!true){//Ethernet.begin(mac, 10000)                         // Ethernet.begin(mac, ip, myDns, gateway, subnet);
    Serial.println(F("Failed to conf DHCP"));
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println(F("Eth card not found. Run w/o hardware"));
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println(F("Eth cable is't connected."));
    } else if (Ethernet.linkStatus() == LinkON){
      Ethernet.begin(mac, ip, myDns, gateway, subnet);
    }
  }
  //myOLED.print(F("IP:"), 0, 2);
  //uint32_t ip1 = Ethernet.localIP();
  //myOLED.print(ip1 & 0xff, 4*6, 2); myOLED.print(F("."), 7*6, 2);
  //myOLED.print((ip1>>24) & 0xff, 8*6, 2); myOLED.print(F("."), 11*6, 2);
  //myOLED.print((ip1>>16) & 0xff, 12*6, 2); myOLED.print(F("."), 15*6, 2);
  //myOLED.print((ip1>>24) & 0xff, 16*6, 2);
  Serial.print(F("My ip addr: "));
  Serial.println(Ethernet.localIP());
  
}

unsigned long endTm1 = 100;
void loop() {
  doButtonsDimmers();
  doEthernet();
  doMqtt();
  
  if (millis() < endTm1) return;
  endTm1 = millis() + 200;
  if (Serial.available()>0) { 
    int a1 = Serial.parseInt();
    dimmers[0].setPower(a1);
    Serial.parseInt();
  }
}

// прерывание детектора нуля
void handleInt0() {
  counter = 101;
  for (byte i = 0; i < channelAmount; i++) dimmers[i].zeroCross();
  Timer2.restart();
}

// прерывание таймера
ISR(TIMER2_A) {
  for (byte i = 0; i < channelAmount; i++)
    if (counter == dimmers[i].triakTime) digitalWrite(dimmers[i].pin, HIGH);          //  включаем на 100мкс
      else if (counter+1 == dimmers[i].triakTime) digitalWrite(dimmers[i].pin, LOW);  //  выключаем, но семистор остается включенным до переполюсовки  
  counter--;
}
