//Imports libraries
#include <DHTesp.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

#define SoftTX 4
#define SoftRX 2

ADC_MODE(ADC_VCC);

//Set instances
WiFiClient wClient;
PubSubClient mqtt_client(wClient);
DHTesp dht;
SoftwareSerial SerialSoft(SoftRX, SoftTX); // RX, TX

//Set connection variables
const char* ssid = "root";             // WiFi network SSID
const char* password = "root";        // WiFi network password
const char* mqtt_server = "server"; // MQTT broker server address
const char* mqtt_user = "user";          // MQTT broker username
const char* mqtt_pass = "pass";         // MQTT broker password

//Set topics variables
char topic_pub_datos[256];
char topic_pub_connection[256];
char topic_sub_intensity[256];

// Gas sensor pin
const int gasSensorPin = D5;

//set id_placa variable
char id_placa[16];

unsigned long last_msg = 0;
unsigned long data_delay = 5000;
float gasValue; // Initialize gas value
bool SWITCH = true;
int LED = 50;
const int LED_PIN = 2;
const int SWITCH_PIN = 16;
int led_increment = 1;

//Create a Struct to store data
struct data_register {
  String IP;
  float temp;
  float hum;
  float Vcc;
  String RSSI;
  String SSId;
};
struct data_conexion{
  bool status;
};

//Connection json
void connection_json(struct data_conexion data, char* jsonmsg, int size) {
  DynamicJsonDocument doc(300);

  doc["CHIPID"] = id_placa;
  doc["online"] = data.status;

  serializeJson(doc, jsonmsg, size);
}

// Create a function to serialize data into JSON format
void data_json(struct data_register data, char* jsonmsg, int size) {
  DynamicJsonDocument doc(300);

  doc["CHIPID"] = id_placa;
  doc["Uptime"] = millis();
  doc["Vcc"] = data.Vcc;

  JsonObject sensor = doc.createNestedObject("Sensor");
  sensor["temp"] = data.temp;
  sensor["hum"] = data.hum;
  bool gas_leak = digitalRead(gasSensorPin);
  sensor["gas_leak"] = !gas_leak;

  doc["LED"] = LED;
  doc["SWITCH"] = SWITCH;

  JsonObject Wifi = doc.createNestedObject("Wifi");
  Wifi["SSId"] = data.SSId;
  Wifi["IP"] = data.IP;
  Wifi["RSSI"] = data.RSSI;

  serializeJson(doc, jsonmsg, size);
}

//Wifi connection
void conecta_wifi() {
  Serial.printf("\nConnecting to %s:\n", ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.printf("\nWiFi connected, IP address: %s\n", WiFi.localIP().toString().c_str());
}

//MQTT connection
void conecta_mqtt() {
  char json[512];
  struct data_conexion conexion;
  conexion.status = false;
  connection_json(conexion, json, 512);
  // Loop until we're reconnected
  while (!mqtt_client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt_client.connect(id_placa, mqtt_user, mqtt_pass, topic_pub_connection, 2, true, json)) {
      Serial.printf(" conectado a broker: %s\n", mqtt_server);
      conexion.status = true;
      connection_json(conexion, json, 512);
      Serial.println(json);
      mqtt_client.publish(topic_pub_connection, json);
      mqtt_client.subscribe(topic_sub_intensity);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void change_LED(int new_LED){
  int led_flag = -1;
  if(new_LED > LED){
    led_flag = 1;
  }
  Serial.println("Changing LED intensity");
  while(LED != new_LED){
    LED = LED + led_increment * led_flag;
    analogWrite(LED_PIN, LED);
    delay(10);
  }
  Serial.println("LED update done");
}

void callback(char* topic, byte* payload, unsigned int length) {
  char mensaje[length + 1];
  memcpy(mensaje, payload, length);
  mensaje[length] = '\0';

  Serial.printf("Mensaje recibido %s\n", mensaje);
  StaticJsonDocument<512> json;
  DeserializationError error = deserializeJson(json, mensaje, length);
  if (strcmp(topic, topic_sub_intensity)==0){
    int new_LED = int(json["level"])%100;
    Serial.printf("Old intensity:%d, New intensity:%d", LED, new_LED);
    change_LED(new_LED);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Empieza setup...");
  analogWriteRange(100);
  dht.setup(5, DHTesp::DHT11); // Changed DHT11 to dht
  pinMode(SWITCH_PIN, OUTPUT);
  digitalWrite(SWITCH_PIN, HIGH);
  pinMode(LED_PIN, OUTPUT);
  analogWrite(LED_PIN, LED);
  pinMode(gasSensorPin, INPUT);

  sprintf(id_placa, "ESP%d", ESP.getChipId());
  sprintf(topic_pub_connection, "IOT2/%s/conexion", id_placa);
  sprintf(topic_pub_datos, "IOT2/%s/data", id_placa);
  sprintf(topic_sub_intensity, "IOT2/%s/led/cmd", id_placa);

  conecta_wifi();
  mqtt_client.setServer(mqtt_server, 1883);
  mqtt_client.setBufferSize(512);
  mqtt_client.setCallback(callback);
  conecta_mqtt();
  delay(2000); // Delay to ensure MQTT connection is established
}

void loop() {
  char jsonmsg[512];
  struct data_register data;

  if (!mqtt_client.connected()) {
    conecta_mqtt();
    delay(2000); // Delay to ensure MQTT connection is established
  }
  mqtt_client.loop();

  unsigned long now = millis();
  if (now - last_msg >= data_delay) {
    last_msg = now;

    data.temp = dht.getTemperature();
    data.hum = dht.getHumidity();
    data.IP = WiFi.localIP().toString(); 
    data.RSSI = WiFi.RSSI();
    data.Vcc = ESP.getVcc()/1000.00;

    data_json(data, jsonmsg, 512);
    Serial.println(id_placa);
    Serial.println(topic_pub_datos);
    Serial.println(jsonmsg);

    mqtt_client.publish(topic_pub_datos, jsonmsg);
  }
}
