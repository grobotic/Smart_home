#include <ESP8266WiFi.h>
#include <PubSubClient.h>

//тут надо настроить на какой топик подписываем устройство
char* topik = "home/livingroom/device/nightlight";
int releyPin = D7; //управление реле

//натсройки сети и сервера
char ssid[20] = "Honor 6X"; //сеть WiFi
char password[20] = "******"; //пароль от WiFi
char mqtt_server[30] = "m15.cloudmqtt.com"; //адрес сервера MQTT
char mqtt_id[20] = "lr-nightlight";
char mqtt_user[20] = "user";
char mqtt_psw[20] = "psw";
char mqtt_port[10] = "12035";

WiFiClient espClient;
PubSubClient client(espClient);
char msg[50]; //сообщение отправляемое на сервер

void setup_wifi() { //устанавливаем соединение WiFi

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to \"");
  Serial.print(ssid);
  Serial.println("\"");

  WiFi.begin(ssid, password);

  long now = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - now > 30000) {
      Serial.println("WiFi not connected!");
      return;
    }
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() { //подключаемся к MQTT серверу
  while (!client.connected()) {
    client.setServer(mqtt_server, atoi(mqtt_port));
    Serial.print("Attempting MQTT ");
    Serial.print(mqtt_server);
    Serial.print(" connection...");
    // Attempt to connect
    long now = millis();
    if (client.connect(mqtt_id,mqtt_user,mqtt_psw)) {
      Serial.println("connected");
      client.subscribe(topik, 1);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      if (millis() - now > 30000) {
        Serial.println("MQTT not connected!");
        return;
      }
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
//тут пишем действия при получении сообщения
void callback(char* topic, byte* payload, unsigned int length) {
  String val = "";
  for (int i=0;i<length;i++) {
    val += (char)payload[i];
  }
  if (val == "1") {
    digitalWrite(releyPin,HIGH);
  }
  else {
    digitalWrite(releyPin,LOW);
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  pinMode(releyPin, OUTPUT);
  setup_wifi();
  client.setServer(mqtt_server, atoi(mqtt_port));
  client.setCallback(callback);
}

void loop() {
  if ((WiFi.status() != WL_CONNECTED)) {
    setup_wifi();
  }
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
