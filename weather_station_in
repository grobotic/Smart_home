#include <SimpleDHT.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

SimpleDHT22 dht22(D2);

char ssid[20] = "Honor 6X"; //сеть WiFi
char password[20] = "*****"; //пароль от WiFi
char mqtt_server[30] = "m15.cloudmqtt.com"; //адрес сервера MQTT
char mqtt_id[20] = "WS-livingroom";
char mqtt_user[20] = "user";
char mqtt_psw[20] = "*******";
char mqtt_port[10] = "12035";

String portData = "";     // Переменная приема команды
boolean endOfString = false;
long lastSend = -1000*60*10; //когда последний раз отправляли данные

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

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  setup_wifi();
  client.setServer(mqtt_server, atoi(mqtt_port));
}

void loop() {
  while (Serial.available() > 0 && !endOfString) {
    char c = Serial.read();    // Очередной символ в строке
    if (c != '\n') portData += c;     // Если это не символ конца строки, то добавляем его в строку
    else endOfString = true;
  }
  if (endOfString) {    // Если получен символ конца строки то разбор строки на ключ - значение и сравнение с готовыми
    Serial.println("portData: " + portData);
    String key = "", value = "";   // ключ, значение
    int i = 0;
    while (portData.length()>i) {
      if (portData[i]==' ') break;
      key += portData[i];
      i++;
    }
    if (portData[i] == ' ') {
      i++;
    }
    while (portData.length() > i) {
      value += portData[i];
      i++;
    }
    portData = "";
    endOfString = false;

    Serial.print("key: ");
    Serial.println(key);
      
    if (key.equals("ssid")) {  //указываем имя сети
      memset(ssid,0,sizeof(ssid));
      for(int i=0;i<value.length()&&i<20;i++){
        ssid[i] = value[i];
      }
      Serial.print("ssid: ");
      Serial.println(ssid);
    }
    if (key.equals("password")) {  //указываем пароль от вифи
      memset(password,0,sizeof(password));
      for(int i=0;i<value.length()&&i<20;i++){
        password[i] = value[i];
      }
      Serial.print("password: ");
      Serial.println(password);
    }
    if (key.equals("mqtt_server")) {  //указываем сервер mqtt
      memset(mqtt_server,0,sizeof(mqtt_server));
      for(int i=0;i<value.length()&&i<30;i++){
        mqtt_server[i] = value[i];
      }
      Serial.print("mqtt_server: ");
      Serial.println(mqtt_server);
    }
    if (key.equals("mqtt_id")) {  //указываем id устройства
      memset(mqtt_id,0,sizeof(mqtt_id));
      for(int i=0;i<value.length()&&i<20;i++){
        mqtt_id[i] = value[i];
      }
      Serial.print("mqtt_id: ");
      Serial.println(mqtt_id);
    }
    if (key.equals("mqtt_user")) {  //указываем логин mqtt
      memset(mqtt_user,0,sizeof(mqtt_user));
      for(int i=0;i<value.length()&&i<10;i++){
        mqtt_user[i] = value[i];
      }
      Serial.print("mqtt_user: ");
      Serial.println(mqtt_user);
    }
    if (key.equals("mqtt_psw")) {  //указываем пароль mqtt
      memset(mqtt_psw,0,sizeof(mqtt_psw));
      for(int i=0;i<value.length()&&i<20;i++){
        mqtt_psw[i] = value[i];
      }
      Serial.print("mqtt_psw: ");
      Serial.println(mqtt_psw);
    }
    if (key.equals("mqtt_port")) {  //указываем пароль mqtt
      memset(mqtt_port,0,sizeof(mqtt_port));
      for(int i=0;i<value.length()&&i<10;i++){
        mqtt_port[i] = value[i];
      }
      Serial.print("mqtt_port: ");
      Serial.println(mqtt_port);
    }
    if (key.equals("wifi_reconnect")) {  //переподключаем вифи
      Serial.println("wifi_reconnect");
      setup_wifi();
    }
    if (key.equals("mqtt_reconnect")) {  //переподключаем mqtt
      Serial.println("mqtt_reconnect");
      reconnect();
    }
    if (key.equals("reconnect")) {  //переподключаем все
      Serial.println("reconnect all");
      setup_wifi();
      reconnect();
    }
  }
  if (millis() - lastSend > 1000*60*10) { //период между отправками, 
    float temperature = 0;
    float t = 0;
    float humidity = 0;
    float h = 0;
    int n = 10;
    
    for (int i = 1; i<=10; i++){ //делаем 10 замеров
      int err = SimpleDHTErrSuccess;
      if ((err = dht22.read2(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
        delay(2000);
        n--; //если ошибка, то уменьшаем колчество успешных замеров
        if (n == 0){ //если успешных замеров 0 то вообще выходим
          return;
        }
      }
      t += temperature;
      h += humidity;
      
      delay(2500);
    }
  
    //находим средние значения
    temperature = t / n;
    temperature = floor(temperature * 10)/10;
    humidity = h / n;
    humidity = floor(humidity * 10)/10;
    
    Serial.print((float)temperature); Serial.print(" *C, ");
    Serial.print((float)humidity); Serial.println(" RH%");

    if ((WiFi.status() != WL_CONNECTED)) {
      setup_wifi();
    }
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
    dtostrf(temperature, 5, 1, msg);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("home/livingroom/sensor/temperature", msg); //отправляем данные в топик
    sprintf(msg, "%d", (int)humidity);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("home/livingroom/sensor/humidity", msg); //отправляем данные в топик

    lastSend = millis();
  }
  delay(500);
  //ESP.deepSleep(1000*3); //1000*60*10 - 10 мин. пока не работает будем думать
}
