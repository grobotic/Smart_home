#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ultrasonic.h>

char* topik = "livingroom/device/nightlight";
char* topikStatus = "livingroom/status/nightlight";

int releyPin = D1;
int mqttLed = D4;
int trigger = D2;
int echo = D3;
Ultrasonic sonic(trigger, echo);

char ssid[20] = "SH_grobotic"; //сеть WiFi
char password[20] = "******"; //пароль от WiFi
char mqtt_server[30] = "192.168.0.100"; //адрес сервера MQTT
char mqtt_id[20] = "lr-nightlight";
char mqtt_user[20] = "user";
char mqtt_psw[20] = "pswrd";
char mqtt_port[10] = "1883";

WiFiClient espClient;
PubSubClient client(espClient);
char msg[50]; //сообщение отправляемое на сервер

int isOn = 0;
long lastSens = -2000;
int isConnecting = 0;
long startConnect = 0;
int isMQTT = 0;
long startMQTT = 0;
int isHand = 0;
int countHand = 0;
long lastRange = 0;

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
    if (client.connect(mqtt_id,mqtt_user,mqtt_psw)) {
      Serial.println("connected");
      client.subscribe(topik, 1);
      client.publish(topikStatus, "1");
      client.subscribe(topikStatus, 1);
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
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
    val += (char)payload[i];
  }
  Serial.println();
  if (String(topic).equals(String(topik))) {
    if (val.equals("1")) {
      digitalWrite(releyPin, HIGH);
      isOn = 1;
    } else {
      digitalWrite(releyPin, LOW);
      isOn = 0;
    }
  }
   
  if (String(topic).equals(String(topikStatus))) {
    if (val == "2") {
      client.publish(topikStatus, "1");
    }
  }
}

void relay(int state) {
  if (state == 1) {
    digitalWrite(releyPin, HIGH);
    isOn = 1;
    client.publish(topik, "1");
  } else {
    digitalWrite(releyPin, LOW);
    isOn = 0;
    client.publish(topik, "0");
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  pinMode(releyPin, OUTPUT);
  pinMode(mqttLed, OUTPUT);
  digitalWrite(releyPin, LOW);
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
    if (WiFi.status() == WL_CONNECTED) {
      reconnect();
    }
  }

  if (millis() - lastRange > 30) {
    if (sonic.Ranging(CM) < 20) {
      isHand = 1;
      countHand++;
      if ((millis() - lastSens > 2000) && (countHand > 2)) {
        if (isOn == 1) {
          relay(0);
        } else {
          relay(1);
        }
        lastSens = millis();
      }
    }
    else {
      isHand = 0;
      countHand = 0;
    }
    lastRange = millis();
  }
  
  client.loop();
}
