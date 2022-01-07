//const char* mqtt_willTopic = "Dimmer/availability"; // Настройки LWT топика для отображения статуса диммера
//const char* mqtt_payloadAvailable = "Online";
//const char* mqtt_payloaNotdAvailable = "Offline";

#define changeLightSpeed 40       // 40 - соответствует 4 секундам. Для регулирования яркости ламп при удержании
#define LinkCheckPeriod 1000        // период проверки подключения к сети
#define MqttReconnectPeriod 10000   // период переподключения к Mqtt при обрыве

#include "triacPLC.h"
#include <SPI.h>
#include "IWatchdog.h"
#include <ArduinoJson.h>
#include <GyverOLED.h>

GyverOLED<SSD1306_128x64, OLED_BUFFER> plcOLED(0x3C);
EthernetClient ethClient;
PubSubClient mqttClient(ethClient);//(server, 1883, mqttCallback, ethClient);


triacPLC::triacPLC(){
  loadEEPROM();
}

bool triacPLC::begin(MQTT_CALLBACK_SIGNATURE){         //инициализация дисплея и сетевой карты
  bool res = true;
  pinF1_DisconnectDebug(PB_3);  // disable Serial wire JTAG configuration
  pinF1_DisconnectDebug(PB_4);  // disable Serial wire JTAG configuration
  pinF1_DisconnectDebug(PA_15); // disable Serial wire JTAG configuration
  SPI.setMOSI(PB5);             // переносим SPI1 на альтернаывные пины
  SPI.setMISO(PB4);
  SPI.setSCLK(PB3);
 // loadEEPROM();
 // if (callback != NULL)_useMQTT = true;
 //   else _useMQTT = false;
  delay(45);                    // ждем дисплей 
  plcOLED.init();               //инициализируем дисплей  myOLED.begin();
  plcOLED.clear();              //Очищаем буфер дисплея.
  display();
  // myOLED.rotateDisplay(true);
  plcOLED.print(F("PLC I0T 7-CH DIMMER"));
  plcOLED.setCursorXY(0, 56);
  plcOLED.print(F("\115\105\130\101\124\120\117\110\40\104\111\131"));
  display();
  
// start the Ethernet connection:
  IPAddress ip; memcpy( &ip[0], inet_cfg.ip, sizeof(inet_cfg.ip));                   
  IPAddress myDNS; memcpy( &myDNS[0], inet_cfg.dns, sizeof(inet_cfg.dns));
  IPAddress gateway; memcpy( &gateway[0], inet_cfg.gw, sizeof(inet_cfg.gw));
  IPAddress subnet; memcpy( &subnet[0], inet_cfg.mask, sizeof(inet_cfg.mask));
  plcOLED.setCursorXY(0, 16);
  Ethernet.init(PA15);
  bool dhcpOK = false;
  delay(5000);                    // ждем сетевуху
  if (inet_cfg.use_dhcp and (Ethernet.hardwareStatus() != EthernetNoHardware)) {
    plcOLED.print(F("Init DHCP.."));
    display();
    #ifdef DEBUGPLC 
      Serial.print(F("Init Ethernet with DHCP: ")); 
    #endif
    dhcpOK = Ethernet.begin(inet_cfg.mac, 5000, 3000);
    if (!dhcpOK){                         // 
      #ifdef DEBUGPLC 
        Serial.println(F("failed"));         
      #endif
      plcOLED.println(F("fail"));
      plcOLED.print(F("Init staticIP.."));
      display();
    }
  } else {
    plcOLED.print(F("Init staticIP.."));
    display(); 
  }
  if (!dhcpOK) Ethernet.begin(inet_cfg.mac, ip, myDNS, gateway);
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    plcOLED.print(F("No Eth Card"));
    #ifdef DEBUGPLC 
      Serial.println(F("No Eth Card")); 
    #endif
    //_useMQTT = false;
    res = false;
  } else {
    plcOLED.print(F("OK"));
    #ifdef DEBUGPLC 
      Serial.println(F("OK")); 
    #endif
  }
  plcOLED.setCursorXY(0, 8);
  plcOLED.print(F("IP: "));
  plcOLED.print(Ethernet.localIP());
  display();
  #ifdef DEBUGPLC 
    Serial.print(F("My ip: "));
    Serial.println(Ethernet.localIP());
  #endif
  if (Ethernet.hardwareStatus() != EthernetNoHardware) Ethernet.maintain();
  if (mqtt_cfg.useMQTT){
    IPAddress mqttBrokerIP;
    memcpy( &mqttBrokerIP[0], mqtt_cfg.SrvIP, sizeof(mqtt_cfg.SrvIP));
    mqttClient.setServer(mqttBrokerIP, mqtt_cfg.SrvPort);
    mqttClient.setCallback(callback);
    char HA_autoDiscovery[40];
    strcpy(HA_autoDiscovery, mqtt_cfg.HADiscover);
    strcat(HA_autoDiscovery, "/light/");
    strcat(HA_autoDiscovery, mqtt_cfg.ClientID);    // "HomeAssistant/light/plc1Traic";

  //  String st1 = HomeAssistantDiscovery;
  //  st1 += "/light/" + plcName;
    String st2;
    for (byte i = 1; i <= channelAmount; i++) {
      _dimmers[i-1].useMQTT = true;
      char tmp_str[65];
      strcpy(tmp_str, HA_autoDiscovery); 
      char num[3]; itoa(i, num, DEC);
      strcat(tmp_str, num);              // "HomeAssistant/light/plc1Traic1..9"; 
      strcat(tmp_str, "/state");        // "HomeAssistant/light/plc1Traic1..9/state"; 
      _dimmers[i-1].mqtt_topic_state = new char[strlen(tmp_str) + 1];
      strcpy(_dimmers[i-1].mqtt_topic_state, tmp_str);
      strcpy(tmp_str, HA_autoDiscovery); 
      strcat(tmp_str, num);              // "HomeAssistant/light/plc1Traic1..9"; 
      strcat(tmp_str, "/bri_state");        // "HomeAssistant/light/plc1Traic1..9/bri_state";
      _dimmers[i-1].mqtt_topic_bri_state = new char[strlen(tmp_str)+1];
      strcpy(_dimmers[i-1].mqtt_topic_bri_state, tmp_str);
    }
  }
  _zeroCrossTime = millis();
  pinMode(zeroDetectorPin, INPUT_PULLUP);
  IWatchdog.begin(10000000);                          // wathDog 10 sec
  return res;
}

bool triacPLC::_haDiscover(){
  if (!mqttClient.connected()) return false;
  StaticJsonDocument<256> JsonDoc;
  JsonDoc["cmd_t"] = "~/set";
  JsonDoc["stat_t"] = "~/state";
  JsonDoc["bri_cmd_t"] = "~/bri_cmd";
  JsonDoc["bri_stat_t"] = "~/bri_state";
  JsonDoc["bri_scl"] = 100;
  JsonDoc["pl_off"] = "0";
  JsonDoc["pl_on"] = "1";
  JsonDoc["retain"] = true;
  for (uint8_t i = 1; i <= channelAmount; i++){
    if (_dimmers[i-1].mqttRegistered) continue;
    char HA_autoDiscovery[40];
    strcpy(HA_autoDiscovery, mqtt_cfg.HADiscover);
    strcat(HA_autoDiscovery, "/light/");
    strcat(HA_autoDiscovery, mqtt_cfg.ClientID);
    char num[3]; itoa(i, num, DEC);
    strcat(HA_autoDiscovery, num);              // "HomeAssistant/light/plc1Traic1..9"; 

    JsonDoc["~"] = HA_autoDiscovery;            //"ha/light/" + plcName + i;
    char tmp_str[65];
    strcpy(tmp_str, mqtt_cfg.ClientID);
    strcat(tmp_str, num);
    JsonDoc["unique_id"] = tmp_str;// plcName + i;
    JsonDoc["name"] = tmp_str;// plcName + i;
    strcpy(tmp_str, HA_autoDiscovery); 
    strcat(tmp_str, "/config");        // "HomeAssistant/light/plc1Traic1..9/config"; 
    
    mqttClient.beginPublish(tmp_str, measureJson(JsonDoc), false);
    serializeJson(JsonDoc, mqttClient);
    _dimmers[i-1].mqttRegistered = mqttClient.endPublish();
    strcpy(tmp_str, HA_autoDiscovery);
    strcat(tmp_str, "/set");                  // "HomeAssistant/light/plc1Traic1..9/set"; 
    mqttClient.subscribe(tmp_str);
    Ethernet.maintain();
    strcpy(tmp_str, HA_autoDiscovery);
    strcat(tmp_str, "/bri_cmd");                  // "HomeAssistant/light/plc1Traic1..9/bri_cmd"; 
    mqttClient.subscribe(tmp_str);
    break;   
  }
  return true;
}

bool triacPLC::_mqttReconnect() {
  #ifdef DEBUGPLC 
    Serial.print(F("connectig..")); 
  #endif
  plcOLED.setCursorXY(0, 40);
  plcOLED.print("MQTT CONNECT.."); 
  //if (client.connect(clientID, mqtt_username, mqtt_password, mqtt_willTopic, 0, true, mqtt_payloaNotdAvailable, true)) {
  if (mqttClient.connect(mqtt_cfg.ClientID, mqtt_cfg.User, mqtt_cfg.Pass)){ 
    plcOLED.print(F("OK  "));
   // client.publish (mqtt_willTopic, mqtt_payloadAvailable, true) ;      
//    mqttClient.subscribe(mqtt_Dimmer0); //Serial.print(F("Connected to: ")); Serial.println(mqtt_Dimmer0);
    #ifdef DEBUGPLC 
      Serial.println(F("OK")); 
    #endif
    //char* st = "{\"~\": \"ha/light/PLC2\",\r\"unique_id\": \"litPLC2\",\r\"cmd_t\": \"~/set\",\r\"stat_t\": \"~/state\",\r\"bri_cmd_t\": \"~/bri_cmd\",\r\"bri_stat_t\": \"~/bri_state\",\r\"bri_scl\": 100,\r\"pl_off\": \"0\",\r\"pl_on\": \"1\"}";
    //msgLen = strlen(st);
    //mqttClient.publish(mqtt_autoDiscovery, st);
  } else {
    plcOLED.print(F("FAIL")); 
    #ifdef DEBUGPLC 
      Serial.println(F(" fail")); 
    #endif
  }
  return mqttClient.connected();
}

bool triacPLC::_doMqtt(){
  if (!mqtt_cfg.useMQTT) return true;  
  static uint32_t nextReconnectAttempt = MqttReconnectPeriod/2;
  if (Ethernet.linkStatus() != LinkON) {
    plcOLED.setCursorXY(0, 40);
    plcOLED.print(F("MQTT CONNECT.. FAIL")); 
    return false;
  }
  bool res = mqttClient.connected();
  if (!res) {
    if (millis() > nextReconnectAttempt) {
      nextReconnectAttempt = millis() + MqttReconnectPeriod;
    for (uint8_t i = 1; i <= channelAmount; i++) _dimmers[i-1].mqttRegistered = false;
      _mqttReconnect();    // Attempt to reconnect
    }
  } else {
    plcOLED.setCursorXY(90, 40);
    plcOLED.print(F("OK  "));
    mqttClient.loop();
  }
  return res;
}

bool triacPLC::_doEthernet(){
  //if (!mqtt_cfg.useMQTT) return true;
  static uint32_t nextLinkCheck = LinkCheckPeriod;
  if (millis() < nextLinkCheck) return true;
  nextLinkCheck = millis() + LinkCheckPeriod;
  auto res = Ethernet.maintain();
  plcOLED.setCursorXY(0, 24);
  switch (res) {
    case 1:
      plcOLED.print(F("RENEW DHCP FAIL"));
      break;
    case 2:
      plcOLED.print(F("RENEW DHCP OK"));
      //Serial.print(F("My IP: ")); Serial.println(Ethernet.localIP());
      break;
    case 3:
      plcOLED.print(("REBIND FAIL"));
      break;
    case 4:
      plcOLED.print(("REBIND OK"));
      //Serial.print(F("My IP: ")); Serial.println(Ethernet.localIP());
      break;
    default:
      plcOLED.print(("def"));
      //nothing happened
      break;
  }
  auto link = Ethernet.linkStatus();
  plcOLED.setCursorXY(0, 32);
  switch (link) {
    case Unknown:
      #ifdef DEBUGPLC 
        Serial.println(F("No Eth card")); 
      #endif
      plcOLED.print(("No Eth card"));
      break;
    case LinkON:
      #ifdef DEBUGPLC 
        Serial.println(F("LINK ON ")); 
      #endif
      plcOLED.print(F("LINK ON "));
      break;
    case LinkOFF:
      #ifdef DEBUGPLC 
        Serial.println(F("LINK OFF ")); 
      #endif
      plcOLED.print(F("LINK OFF"));
      break;
  }
  if ((res == 0) && (link == LinkON)) return true; 
    else return false; 
}

void triacPLC::mqttCallback(char* topic, byte* payload, uint16_t length) {
 // ha/light/plc1Traic1/set
  char *uk1, *uk2, stemp[4];//, pl1[4];
  char *pl1 = new char[length+1];
  strncpy(pl1, (char*)payload, length);
  pl1[length] = '\0';
  uk1 = strstr(topic, mqtt_cfg.ClientID);
  uk1 += strlen(mqtt_cfg.ClientID);
  uk2 = strchr(uk1,'/');
  uint8_t num = uk2-uk1; 
  strncpy(stemp, uk1, num);
  stemp[num] = 0;
  num = atoi(stemp);
  if (num > 9) return;      // ошика
  uk2++;
  if (strcmp(uk2, "bri_cmd") == 0)
    setPower(num-1, atoi((char*)pl1));
  if (strcmp(uk2, "set") == 0) {
     if (atoi((char*)pl1)) _dimmers[num-1].setOn();
       else _dimmers[num-1].setOff();
     itoa(_dimmers[num-1].getOnOff(), stemp, DEC);
     mqttClient.publish(_dimmers[num-1].mqtt_topic_state, stemp, true);
  }
  delete [] pl1;
}

void triacPLC::_doButtonsDimmers(){
  static uint32_t endTm2 = changeLightSpeed;
  for (uint8_t i = 0; i < btnsAmount; i++) _btns[i].tick();
  char st[4];
   //двойной клик
  for (byte i = 0; i < channelAmount; i++)
    if (_btns[i].isDouble())
      setPower(i, 50);
  
  // включение/выключение
  for (byte i = 0; i < btnsAmount; i++){
    if (_btns[i].isSingle()){    
      for (byte j = 0; j < channelAmount; j++){
        _dimmers[j].toggle();
        if (mqtt_cfg.useMQTT){
          itoa(_dimmers[j].getOnOff(), st, DEC);
          mqttClient.publish(_dimmers[j].mqtt_topic_state, st, true);
        }
      }
    }
  }

  if (millis() < endTm2) return;
  endTm2 = millis() + changeLightSpeed;

  // кнопка удерживается
  for (byte i = 0; i < channelAmount; i++){
    if (_btns[i].isHold()){
      _dimmers[i].incPower();
      if (mqtt_cfg.useMQTT){
        itoa(_dimmers[i].getPower(), st, DEC);
        mqttClient.publish(_dimmers[i].mqtt_topic_bri_state, st, true);
      }
    }
  }

  for (byte i = 0; i < channelAmount; i++){
    if (_btns[i].isRelease()){
      _dimmers[i].saveEEPROM();
    }
  }
}

bool triacPLC::processEvents(){
  static uint32_t endTm1 = 200;
  IWatchdog.reload();
  _doButtonsDimmers();
  _doEthernet();
  bool res = _doMqtt();
  if (millis() < endTm1) return res;
  endTm1 = millis() + 200;
  return _haDiscover();
}

void triacPLC::display(){                                       // Эа функция нужна из-за аппараного бага при пререносе SPI1 и одновременной раоте I2C1
  __HAL_RCC_I2C1_CLK_ENABLE();
  plcOLED.update();
  __HAL_RCC_I2C1_CLK_DISABLE();  
}

void triacPLC::showFreq(){
  plcOLED.setCursorXY(82, 56);
  plcOLED.print(ACfreq/10.0, 1); 
  plcOLED.print(" Hz");
  display();
}

void triacPLC::zeroCrossISR(){
  _counter = 200;
  for (byte i = 0; i < channelAmount; i++) _dimmers[i].zeroCross();
  _zeroCrossCntr--;
  if (_zeroCrossCntr == 0) {
    _zeroCrossCntr = 200;
    ACfreq = 1000000/(millis() - _zeroCrossTime);
    _zeroCrossTime = millis();
  }    
}

void triacPLC::timerISR(){
  uint8_t _counter1 = _counter, n = 3;  
  if (_counter1 > 100) _counter1 -= 100; 
  if (_counter1 > 80) n = 10;
  for (uint8_t i = 0; i < channelAmount; i++)
    if ((_counter1 < _dimmers[i].triacTime) && (_counter1 + n > _dimmers[i].triacTime)) digitalWrite(_dimmers[i].pin, HIGH);          //  включаем на 100мкс
      else digitalWrite(_dimmers[i].pin, LOW);                                                                  //  выключаем, но семистор остается включенным до переполюсовки        
  _counter--;
}

void triacPLC::allOff(){
  for (uint8_t i = 0; i < channelAmount; i++) setOff(i);
}

void triacPLC::setOn(uint8_t channel){
  if (channel >= channelAmount) return;
  _dimmers[channel].setOn();
  if (mqtt_cfg.useMQTT){
    char st[4];        
    itoa(_dimmers[channel].getOnOff(), st, DEC);
    mqttClient.publish(_dimmers[channel].mqtt_topic_state, st, true);
  }
}

void triacPLC::setOff(uint8_t channel){
  if (channel>=channelAmount) return;
  _dimmers[channel].setOn();
  if (mqtt_cfg.useMQTT){
    char st[4];        
    itoa(_dimmers[channel].getOnOff(), st, DEC);
    mqttClient.publish(_dimmers[channel].mqtt_topic_state, st, true);
  }
}

void triacPLC::toggle(uint8_t channel){
  if (channel>=channelAmount) return;
  _dimmers[channel].toggle();
  if (mqtt_cfg.useMQTT){
    char st[4];        
    itoa(_dimmers[channel].getOnOff(), st, DEC);
    mqttClient.publish(_dimmers[channel].mqtt_topic_state, st, true);
  }
}

void triacPLC::setPower(uint8_t channel, uint8_t power){
  if (channel>=channelAmount) return; 
  _dimmers[channel].setPower(power);
  if (mqtt_cfg.useMQTT){
    char st[4];    
    itoa(_dimmers[channel].getPower(), st, DEC);
    mqttClient.publish(_dimmers[channel].mqtt_topic_bri_state, st, true);
  }
}

void triacPLC::incPower(uint8_t channel){
  if (channel>=channelAmount) return;
  _dimmers[channel].incPower();
}

void triacPLC::setRampTime(uint8_t channel, uint16_t rampTime){    // ramtTime в секундах
  if (channel>=channelAmount) return;
  _dimmers[channel].setRampTime(rampTime);
}

void triacPLC::setDefInetCfg(){
  inet_cfg.use_dhcp = def_useDHCP;
  inet_cfg.dhcp_refresh_minutes = 5;
  {uint8_t mac1[] = def_macAddr; memcpy(inet_cfg.mac, mac1, sizeof(inet_cfg.mac)); }
  {uint8_t ip1[] = {def_staticIP}; memcpy(inet_cfg.ip, ip1, sizeof(inet_cfg.ip)); }
  {uint8_t ip1[] = {def_staticGW}; memcpy(inet_cfg.gw, ip1, sizeof(inet_cfg.gw)); }
  {uint8_t ip1[] = {def_staticDNS}; memcpy(inet_cfg.dns, ip1, sizeof(inet_cfg.dns)); }
  {uint8_t ip1[] = {def_staticMASK}; memcpy(inet_cfg.mask, ip1, sizeof(inet_cfg.mask)); }
  inet_cfg.webSrvPort = 80;
}

void triacPLC::setDefMqttCfg(){
  strcpy(mqtt_cfg.User, def_mqtt_user);
  strcpy(mqtt_cfg.Pass, def_mqtt_pass);
  strcpy(mqtt_cfg.ClientID, def_mqtt_clientID);
  strcpy(mqtt_cfg.HADiscover, def_HomeAssistantDiscovery);    // "HomeAssistant"
  uint8_t ip1[] = {def_mqtt_brokerIP}; memcpy(mqtt_cfg.SrvIP, ip1, sizeof(mqtt_cfg.SrvIP));
  mqtt_cfg.SrvPort = 1883;  
}


void triacPLC::loadEEPROM(){
// https://github.com/sirleech/Webduino/blob/master/examples/Web_Net_Setup/Web_Net_Setup.pde
  for (uint8_t i = 0; i < channelAmount; i++) _dimmers[i].loadEEPROM();                // загружаем сохраненные состояния диммеров из eeprom
  if (EEPROM[0] != EEPROMdataVer) {     // в EEPROM ничего еще не сохраняли
    setDefInetCfg();
    setDefMqttCfg();
    saveEEPROM();
  } else {
     EEPROM.get(channelAmount*2+2, inet_cfg);
     EEPROM.get(channelAmount*2+2 + sizeof(inet_cfg), mqtt_cfg);
  }
}
    
void triacPLC::saveEEPROM(){
  for (uint8_t i = 0; i < channelAmount; i++) _dimmers[i].saveEEPROM();
  EEPROM.put(channelAmount*2+2, inet_cfg);
  EEPROM.put(channelAmount*2+2 + sizeof(inet_cfg), mqtt_cfg);
}

uint8_t triacPLC::getRampTime(uint8_t channel){
  if (channel>=channelAmount) return 0;
  return _dimmers[channel].getRampTime();
}

uint8_t triacPLC::getPower(uint8_t channel){
  if (channel >= channelAmount) return 0;
  return _dimmers[channel].getPower();
}

bool triacPLC::getBtnState(uint8_t btn_idx){
  if (btn_idx >= btnsAmount) return false;
  return _btns[btn_idx].state();
}

bool triacPLC::getOnOff(uint8_t channel){
  if (channel>=channelAmount) return false;
  return _dimmers[channel].getOnOff();
}

void triacPLC::set_useMQTT(bool new_useMQTT){
  mqtt_cfg.useMQTT = new_useMQTT;
  for (uint8_t i = 0; i < channelAmount; i++) _dimmers[i].useMQTT = new_useMQTT;
}
