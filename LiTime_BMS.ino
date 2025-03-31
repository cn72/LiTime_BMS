#include <BMSClient.h>
#include <WiFi.h>
#include <PubSubClient.h>
#define LED_BUILTIN 22
#include "esp_system.h"
#include "rom/ets_sys.h"

BMSClient bmsClient;
// WiFi
const char *ssid = "YourSSID";               // Enter your WiFi name
const char *password = "YourWLAN_Passwd";  // Enter WiFi password

// MQTT Broker
const char *mqtt_broker = "IP";
const char *topic = "BMS/Speicher";
const char *mqtt_username = "";
const char *mqtt_password = "";
const int mqtt_port = 1883;
String mqtt_topic = "";
String value = "";
int count_wlan = 0;

const int wdtTimeout = 600000;  //time in ms to trigger the watchdog
hw_timer_t *timer = NULL;


WiFiClient espClient;
PubSubClient client(espClient);

void ARDUINO_ISR_ATTR resetModule() {
  mqtt_topic = String(topic) + "/connect";
  client.publish(mqtt_topic.c_str(), "false");
  delay (3000);
  ets_printf("reboot\n");
  esp_restart();
}

void printCellVoltages(const std::vector<float> &cells) {
  //Serial.println("Cell Voltages:");
  for (size_t i = 0; i < cells.size(); i++) {
    //Serial.printf("  Cell %2d: %.3fV\n", i + 1, cells[i]);
    if (i < 9) {
      mqtt_topic = String(topic) + "/Cell_Voltages/Cell_0" + String(i + 1);
      client.publish(mqtt_topic.c_str(), String(cells[i]).c_str());
    } else {
      mqtt_topic = String(topic) + "/Cell_Voltages/Cell_" + String(i + 1);
      client.publish(mqtt_topic.c_str(), String(cells[i]).c_str());
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(3000);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    Serial.println("Connecting to WiFi..");
    count_wlan++;
    if (count_wlan > 10){
      count_wlan = 0;
      WiFi.disconnect();
      delay(500);
      WiFi.begin(ssid, password);
    }
  }
  delay(3000);
  Serial.println("Connected to WiFi..");
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while (!client.connected()) {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    //Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      //Serial.println("Public EMQX MQTT broker connected");
    } else {
      //Serial.print("failed with state ");
      //Serial.print(client.state());
      delay(2000);
    }
  }
  delay(3000);
  //Serial.println("BMS Client Example");

  bmsClient.init("MAC Adresse LiTime BT");  // MAC-Adresse über init()

  if (bmsClient.connect()) {
    //Serial.println("Connected to BMS!");
  } else {
    //Serial.println("Connection failed!");
  }
  timer = timerBegin(1000000);                     //timer 1Mhz resolution
  timerAttachInterrupt(timer, &resetModule);       //attach callback
  timerAlarm(timer, wdtTimeout * 1000, false, 0);  //set time in us
}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.println("-----------------------");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("LED ON");
    while (!client.connected()) {
      String client_id = "esp32-client-";
      client_id += String(WiFi.macAddress());
      //Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
      if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
        //Serial.println("Public EMQX MQTT broker connected");
      } else {
        //Serial.print("failed with state ");
        //Serial.print(client.state());
        delay(2000);
      }
    }
    if (bmsClient.isConnected()) {
      bmsClient.update();
      mqtt_topic = String(topic) + "/connect";
      client.publish(mqtt_topic.c_str(), "true");
      //Serial.println("\n=== BMS Data ===");
      //Serial.printf("Total Voltage:    %.2f V\n", bmsClient.getTotalVoltage());
      mqtt_topic = String(topic) + "/TotalVoltage";
      value = String(bmsClient.getTotalVoltage());
      if ((value != "0") && (value != NULL)){
        client.publish(mqtt_topic.c_str(), value.c_str());
        //Serial.printf("Current:          %.2f A\n", bmsClient.getCurrent());
      }  
      mqtt_topic = String(topic) + "/Current";
      client.publish(mqtt_topic.c_str(), String(bmsClient.getCurrent()).c_str());
      //Serial.printf("SOC:              %d %%\n", bmsClient.getSOC());
      mqtt_topic = String(topic) + "/SOC";
      value = String(bmsClient.getSOC());
      if (value != "0"){
        client.publish(mqtt_topic.c_str(), value.c_str());
        //Serial.printf("SOH:              %s\n", bmsClient.getSOH().c_str());
      }
      mqtt_topic = String(topic) + "/SOH";
      value = String(bmsClient.getSOH());
      if ((value != "0") && (value != NULL)){
        client.publish(mqtt_topic.c_str(), value.c_str());
        //Serial.printf("MOSFET Temp:      %d °C\n", bmsClient.getMosfetTemp());
      }
      mqtt_topic = String(topic) + "/MOSFET Temp";
      value = String(bmsClient.getMosfetTemp());
      if (value != "0"){
        client.publish(mqtt_topic.c_str(), value.c_str());
        //Serial.printf("Cell Temp:        %d °C\n", bmsClient.getCellTemp());
      }
      mqtt_topic = String(topic) + "/CELL Temp";
      value = String(bmsClient.getCellTemp());
      if (value != "0"){
        client.publish(mqtt_topic.c_str(), value.c_str());
        //Serial.printf("Remaining:        %.2f Ah\n", bmsClient.getRemainingAh());
      }
      mqtt_topic = String(topic) + "/Remaining(AH)";
      value = String(bmsClient.getRemainingAh());
      if (value != "0"){
        client.publish(mqtt_topic.c_str(), value.c_str());
        //Serial.printf("Full Capacity:    %.2f Ah\n", bmsClient.getFullCapacityAh());
      }
      mqtt_topic = String(topic) + "/Full Capacity(AH)";
      value = String(bmsClient.getFullCapacityAh());
      if ((value != "0") && (value != NULL)){
        client.publish(mqtt_topic.c_str(), value.c_str());
        //Serial.printf("Protection State: %s\n", bmsClient.getProtectionState().c_str());
      }
      //Serial.printf("Balancing:        %s\n", bmsClient.getBalancingState().c_str());
      mqtt_topic = String(topic) + "/Discharge Cycles";
      value = String(bmsClient.getDischargesCount());
      if (value != "0"){
        client.publish(mqtt_topic.c_str(), value.c_str());
        //Serial.printf("Discharge Cycles: %u\n", bmsClient.getDischargesCount());
      }
      printCellVoltages(bmsClient.getCellVoltages());
      //Serial.println("==================\n");
    } else {
      if (bmsClient.connect()) {
        //Serial.println("Connected to BMS!");
      } else {
        //Serial.println("Connection failed!");
        mqtt_topic = String(topic) + "/connect";
        client.publish(mqtt_topic.c_str(), "false");
      }
    }
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("LED OFF");
    delay(7000);
    WiFi.reconnect();
    //WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      Serial.println("Connecting to WiFi..");
      count_wlan++;
    if (count_wlan > 10){
      count_wlan = 0;
      WiFi.disconnect();
      delay(500);
      WiFi.begin(ssid, password);
    }
    } 
  }
  delay(10000);
}
