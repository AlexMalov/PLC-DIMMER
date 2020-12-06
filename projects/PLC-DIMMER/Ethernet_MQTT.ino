// -------------------------------------- BEGIN - Установка параметров сети ---------------------------------
//const IPAddress server(89,108,112,87);                // IP адресс MQTT брокера
const char* server = "dev.rightech.io";         // или имя MQTT брокера  

const char* mqtt_Dimmer0 = "base/state/triak0";         // Топики для получения команд с брокера
//const char* mqtt_Dimmer2 = "Dimmer/triac2";
const char* mqtt_Switch1 = "base/relay/led1";
//const char* mqtt_Switch2 = "Dimmer/switch2";

//const char* mqtt_willTopic = "Dimmer/availability"; // Настройки LWT топика для отображения статуса диммера
//const char* mqtt_payloadAvailable = "Online";
//const char* mqtt_payloaNotdAvailable = "Offline";

EthernetClient ethClient;
PubSubClient client(server, 1883, callback, ethClient);
// --------------------------------------- END - Установка параметров сети ----------------------------------


// --------------------------------------- BEGIN - Подключение и подписка на MQTT broker ----------------------------------

boolean reconnect() {
  const char* clientID = "mqtt-molot120301-t21zxy";            // ИмяКлиента, Логин и Пароль для подключения к MQTT брокеру
  //const char* mqtt_username = "mqtt";
  //const char* mqtt_password = "mqtt";

  Serial.print(F("reconnect...")); 
  //if (client.connect(clientID, mqtt_username, mqtt_password, mqtt_willTopic, 0, true, mqtt_payloaNotdAvailable, true)) {
  if (client.connect(clientID)){ 
    Serial.println(F(" ok."));
   // client.publish (mqtt_willTopic, mqtt_payloadAvailable, true) ;      
    client.subscribe(mqtt_Dimmer0); Serial.print(F("Connected to: ")); Serial.println(mqtt_Dimmer0);
    //client.subscribe(mqtt_Dimmer2); Serial.print(F("Connected to: ")); Serial.println(mqtt_Dimmer2);
    client.subscribe(mqtt_Switch1); Serial.print(F("Connected to: ")); Serial.println(mqtt_Switch1);
    //client.subscribe(mqtt_Switch2); Serial.print(F("Connected to: ")); Serial.println(mqtt_Switch2);
    Serial.println(F("MQTT connected"));    
  } else Serial.println(F(" fail."));
  return client.connected();
}


unsigned long nextReconnectAttempt = 10000;

bool doMqtt(){
  if (Ethernet.linkStatus() != LinkON) return false;
  bool res = client.connected();
  if (!res) {
    if (millis() > nextReconnectAttempt) {
      nextReconnectAttempt = millis() + 10000;
      reconnect();    // Attempt to reconnect
    }
  } else client.loop();
  return res;
}
// --------------------------------------- END - Подключение и подписка на MQTT broker ----------------------------------


unsigned long nextLinkCheck = 2000;
bool doEthernet(){
  if (millis() < nextLinkCheck) return true;
  nextLinkCheck = millis() + 2000;
  auto res = Ethernet.maintain();
  switch (res) {
    case 1:
      //renewed fail
      Serial.println(F("Err: renew fail"));
      break;

    case 2:
      //renewed success
      Serial.println(F("Renewed ok"));
      //print your local IP address:
      Serial.print(F("My IP: "));
      Serial.println(Ethernet.localIP());
      break;

    case 3:
      //rebind fail
      Serial.println(F("Err: rebind fail"));
      break;

    case 4:
      //rebind success
      Serial.println(F("Rebind success"));
      //print your local IP address:
      Serial.print(F("My IP: "));
      Serial.println(Ethernet.localIP());
      break;

    default:
      //nothing happened
      break;
  }
  
  auto link = Ethernet.linkStatus();
  Serial.print(F("Link "));
  switch (link) {
    case Unknown:
      Serial.println(F("Unknown"));
      break;
    case LinkON:
      Serial.println(F("ON"));
      break;
    case LinkOFF:
      Serial.println(F("OFF"));
      break;
  }
  if ((res == 0) && (link == LinkON)) return true; else return false; 
}

void callback(char* topic, byte* payload, uint16_t length) {
  /*
  Serial.print(F("Message arrived ["));
  Serial.print(topic);
  Serial.print(F("] "));
  for (byte i=0; i< length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
*/
  
  // проверка новых сообщений в подписках у брокера
    payload[length] = '\0';
    //Serial.print("Topic: ");
    //Serial.print(String(topic));
    //Serial.println(" - ");
  
  if (String(topic) == "base/state/triak0") {
     String value = String((char*)payload);
     int val = value.toInt();
     dimmers[0].setPower(val);
    
    Serial.print(F("Диммер0: "));
    Serial.println(val);

  }
  
  if (String(topic) == "base/relay/led1") {
     String value = String((char*)payload);
     if (value.toInt()) dimmers[0].setOn();
       else dimmers[0].setOff();
     
     //dimmer1.set(Dim1_value, Switch1_value);
    //Serial.print(F("Выключатель 0: "));
    //Serial.println(dimmers[0].getPower());
  }
/*
  if (String(topic) == "Dimmer/triac2") {
     String value = String((char*)payload);
     Dim2_value = value.substring(0, value.indexOf(';')).toInt();
     if (Dim2_value == 0) {dimmer2.set(0,Switch2_value);}
     else {
      Dim2_value=map(Dim2_value, 1, 100, 30, 100);
      dimmer2.set(Dim2_value, Switch2_value);
      }
     
    Serial.print("Диммер 2: ");
    Serial.println(Dim2_value);

  }
  if (String(topic) == "Dimmer/switch2") {
     String value = String((char*)payload);
     Switch2_value = value.substring(0, value.indexOf(';')).toInt();
     dimmer2.set(Dim2_value, Switch2_value);
    
    Serial.print("Выключатель 2: ");
    Serial.println(Switch2_value);

  }
  */
}
