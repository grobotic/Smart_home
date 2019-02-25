#include <ESP8266WiFi.h>
#include <PubSubClient.h>

char* topik = "livingroom/device/ventilation";
char* topikStatus = "livingroom/status/ventilation";
char* topikHumidity = "livingroom/sensor/humidity";
char* topikAuto = "livingroom/auto/ventilation";     //топик для авторежима
char* topikMin = "livingroom/min/humidity";          //влажность при которой выключается
char* topikMax = "livingroom/max/humidity";          //влажность при которой включается

int releyPin = D3;                                   //сюда подключаем ТТ-реле
int mqttLed = D4;                                    //тут встроенная лампочка WeMos D1 mini

char ssid[20] = "******";                            //сеть WiFi
char password[20] = "******";                        //пароль от WiFi
char mqtt_server[30] = "192.168.0.100";              //адрес сервера MQTT
char mqtt_id[20] = "lr-ventilation";
char mqtt_user[20] = "******";
char mqtt_psw[20] = "******";
char mqtt_port[10] = "1883";

WiFiClient espClient;
PubSubClient client(espClient);
char msg[50];                                        //сообщение отправляемое на сервер

int isOn = 0;              // состояние вентиляции
int isConnecting = 0;      // признак начала подключения к вифи
long startConnect = 0;     // время начала подключения к вифи
long startDisconnect = 0;  // начало отсчета при отключении от сервера
int isMQTT = 0;            // признак начала подключения к серваку
long startMQTT = 0;        // время начала подключения к серваку
int isAuto = 0;            // признак автоматического включения вентиляции
int isOnAuto = 1;          // признак разрешения автоматического режима
int isSelf = 0;            // признак отправки топика при включении автоматически, нужно для автоматического выключения
int humidity = 0;          // уровень влажности
int minH = 45;             // уровень влажности для отключения вытяжки
int maxH = 55;             // уровень влажности для включения вытяжки

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
      client.subscribe(topikHumidity, 1);
      client.subscribe(topikAuto, 1);
      client.subscribe(topikMin, 1);
      client.subscribe(topikMax, 1);
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
    if (val.equals("1")) {
      digitalWrite(releyPin, LOW);
      isOn = 1;
    } else {
      digitalWrite(releyPin, HIGH);
      isOn = 0;
    }
    
    if (isSelf == 0) {
      isAuto = 0;
    }
    
    isSelf = 0;
  }

  if (String(topic).equals(String(topikHumidity))) {
    humidity = val.toInt();
  }
  
  if (String(topic).equals(String(topikStatus))) {
    if (val == "2") {
      client.publish(topikStatus, "1");
    }
  }
  if (String(topic).equals(String(topikAuto))) {
    isOnAuto = val.toInt();
  }
  if (String(topic).equals(String(topikMin))) {
    minH = val.toInt();
  }
  if (String(topic).equals(String(topikMax))) {
    maxH = val.toInt();
  }
}

void relay(int state) {
  if (state == 1) {
    digitalWrite(releyPin, LOW);
    isOn = 1;
    client.publish(topik, "1", true);
  } else {
    digitalWrite(releyPin, HIGH);
    isOn = 0;
    client.publish(topik, "0", true);
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  pinMode(releyPin, OUTPUT);
  pinMode(mqttLed, OUTPUT);
  digitalWrite(releyPin, HIGH);
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

  // если разрешен автоматический режим и уровень влажности выше нормы
  if (isOnAuto == 1 && humidity > maxH) {
    if (isOn == 0) {
      isSelf = 1;
      relay(1);
      isAuto = 1;
    }
  }

  if ((isAuto == 1) && (isOn == 1)) {
    // если уровень влажности стал достаточным или пропала связь с сервером уже как 1 мин.
    if ((humidity <= minH) || ((millis() - startDisconnect > 1000 * 60) && (!client.connected()))) {
      isSelf = 1;
      relay(0);
      isAuto = 1;
    }
  }

  client.loop();
}
