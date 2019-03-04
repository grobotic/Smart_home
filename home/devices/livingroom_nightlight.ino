#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ultrasonic.h>

char* topik = "livingroom/device/nightlight";
char* topikMotion = "livingroom/sensor/motion";
char* topikLight = "livingroom/sensor/light";
char* topikStatus = "livingroom/status/nightlight";
char* topikAuto = "livingroom/auto/nightlight";
char* topikLevel = "livingroom/level/nightlight";

int releyPin = D5;
int mqttLed = D4;
int trigger = D1;
int echo = D2;
Ultrasonic sonic(trigger, echo);

char ssid[20] = "SH_grobotic"; //сеть WiFi
char password[20] = "******"; //пароль от WiFi
char mqtt_server[30] = "192.168.0.100"; //адрес сервера MQTT
char mqtt_id[20] = "lr-nightlight2";
char mqtt_user[20] = "shgrobotic";
char mqtt_psw[20] = "********";
char mqtt_port[10] = "1883";

WiFiClient espClient;
PubSubClient client(espClient);
char msg[50]; //сообщение отправляемое на сервер

int isOn = 0;              // состояние ленты
long lastSens = -2000;     // последнее включение рукой
int isConnecting = 0;      // признак начала подключения к вифи
long startConnect = 0;     // время начала подключения к вифи
long startDisconnect = 0;  // начало отсчета при отключении от сервера
int isMQTT = 0;            // признак начала подключения к серваку
long startMQTT = 0;        // время начала подключения к серваку
int countHand = 0;         // количество обнарружений руки
long lastRange = 0;        // время последнего замера расстояния
int light = 0;             // уровень освещения
int motion = 0;            // наличие движения
long lastMotion = 0;       // время последнего движения
int isAuto = 0;            // признак автоматического включения ленты
int isOnAuto = 1;          // признак разрешения автоматического режима
int isSelf = 0;            // признак отправки топика при включении автоматически, нужно для автоматического выключения
int level = 100;           // Уровень освещения в %
int curentlevel = 0;       // Текущий уровень освещения в %

void setup_wifi() { //устанавливаем соединение WiFi

  delay(10);
  // We start by connecting to a WiFi network

  if (isConnecting == 0) {
    Serial.println();
    Serial.print("Connecting to \"");
    Serial.print(ssid);
    Serial.println("\"");
    WiFi.begin(ssid, password);
    isConnecting = 1;
    startConnect = millis();
  }

  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - startConnect > 30000) {
      Serial.println("WiFi not connected!");
      isConnecting = 0;
      return;
    }
  } else {
    isConnecting = 0;
    randomSeed(micros());

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
}

void reconnect() { //подключаемся к MQTT серверу
  if (!client.connected()) {
    if (isMQTT == 0) {
      client.setServer(mqtt_server, atoi(mqtt_port));
      Serial.print("Attempting MQTT ");
      Serial.print(mqtt_server);
      Serial.print(" connection...");
      // Attempt to connect
      startMQTT = millis();
      isMQTT = 1;
    }
    if (client.connect(mqtt_id, mqtt_user, mqtt_psw)) {
      Serial.println("connected");
      client.subscribe(topik, 1);
      client.publish(topikStatus, "1");
      client.subscribe(topikStatus, 1);
      client.subscribe(topikLight, 1);
      client.subscribe(topikMotion, 1);
      client.subscribe(topikAuto, 1);
      client.subscribe(topikLevel, 1);
      digitalWrite(mqttLed, HIGH);
      isMQTT = 0;
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      if (millis() - startMQTT > 30000) {
        Serial.println("MQTT not connected!");
        isMQTT = 0;
        return;
      }
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String val = "";
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    val += (char)payload[i];
  }
  Serial.println();
  if (String(topic).equals(String(topik))) {
    if (val.equals("1") && isOn == 0) {
      relay(1);
    } else if (val.equals("0") && isOn == 1) {
      relay(0);
    }
    
    if (isSelf == 0) {
      isAuto = 0;
    }
    
    isSelf = 0;
  }

  if (String(topic).equals(String(topikLight))) {
    light = val.toInt();
  }
  if (String(topic).equals(String(topikMotion))) {
    motion = val.toInt();
    if (motion == 0) {
      lastMotion = millis();
    }
  }

  if (String(topic).equals(String(topikStatus))) {
    if (val == "2") {
      client.publish(topikStatus, "1");
    }
  }
  if (String(topic).equals(String(topikAuto))) {
    isOnAuto = val.toInt();
  }
  if (String(topic).equals(String(topikLevel))) {
    level = val.toInt();
  }
}

void relay(int state) {
  int stp = map(level,0,100,0,1024);
  if (state == 1) {
    for (int i = 0; i <= stp; i++) {
      analogWrite(releyPin, i);
      delay(1);
    }
    isOn = 1;
    client.publish(topik, "1", true);
  } else {
    for (int i = stp; i >= 0; i--) {
      analogWrite(releyPin, i);
      delay(1);
    }
    isOn = 0;
    client.publish(topik, "0", true);
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  pinMode(releyPin, OUTPUT);
  pinMode(mqttLed, OUTPUT);
  analogWrite(releyPin, 0);
  digitalWrite(mqttLed, LOW);
  setup_wifi();
  client.setServer(mqtt_server, atoi(mqtt_port));
  client.setCallback(callback);
}

void loop() {
  if ((WiFi.status() != WL_CONNECTED)) {
    setup_wifi();
  }
  if (!client.connected()) {
    digitalWrite(mqttLed, LOW);
    startDisconnect = millis();
    if (WiFi.status() == WL_CONNECTED) {
      reconnect();
    }
  }

  if (millis() - lastRange > 30) {
    if (sonic.Ranging(CM) < 20) {
      countHand++;
      if ((millis() - lastSens > 2000) && (countHand > 2)) {
        isSelf = 0;
        if (isOn == 1) {
          relay(0);
        } else {
          relay(1);
        }
        isAuto = 0;
        lastSens = millis();
      }
    }
    else {
      countHand = 0;
    }
    lastRange = millis();
  }
  //Если вклчен и поменялся уровень яркости, то меняем его.
  if ((isOn == 1) && (curentlevel != level)) {
    int v = map(level,0,100,0,1024);
    analogWrite(releyPin, v);
    curentlevel = level;
  }
 
  // если разрешен автоматический режим и уровень света низкий и есть движение то включаем подсветку
  if (isOnAuto == 1 && light < 1) {
    if (motion == 1) {
      if (isOn == 0) {
        isSelf = 1;
        relay(1);
        isAuto = 1;
      }
    }
  }

  if ((isAuto == 1) && (isOn == 1)) {
    // если уровень света стал достаточным или нет движения 5 сек (сервер отсылает после 20 сек) или пропала связь с сервером уже как 1 мин.
    if ((light>1) || ((motion == 0) && (millis() - lastMotion > 1000 * 5)) || ((millis() - startDisconnect > 1000 * 60) && (!client.connected()))) {
      isSelf = 1;
      relay(0);
      isAuto = 1;
    }
  }

  client.loop();
}
