#include <BMSClient.h>
#include <WiFi.h>
#include <PubSubClient.h>

BMSClient bmsClient;
// WiFi
const char *ssid = "YourSSID";               // Enter your WiFi name
const char *password = "Your WLAN PASWD";  // Enter WiFi password

// MQTT Broker
const char *mqtt_broker = "0.0.0.0";
const char *topic = "BMS/Speicher";
const char *mqtt_username = "";
const char *mqtt_password = "";
const int mqtt_port = 1883;
String mqtt_topic ="";

WiFiClient espClient;
PubSubClient client(espClient);

void printCellVoltages(const std::vector<float> &cells) {
  //Serial.println("Cell Voltages:");
  for (size_t i = 0; i < cells.size(); i++) {
    //Serial.printf("  Cell %2d: %.3fV\n", i + 1, cells[i]);
    if (i < 9) {
      mqtt_topic = String(topic)+"/Cell_Voltages/Cell_0"+String(i+1);
      client.publish(mqtt_topic.c_str(), String(cells[i]).c_str());
    } else {
      mqtt_topic = String(topic)+"/Cell_Voltages/Cell_"+String(i+1);
      client.publish(mqtt_topic.c_str(), String(cells[i]).c_str());
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(3000);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.println("Connecting to WiFi..");
  }
  delay(3000);
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

  bmsClient.init("MAC Adresse des BMS (Bluetooth)");  // MAC-Adresse über init()

  if (bmsClient.connect()) {
    //Serial.println("Connected to BMS!");
  } else {
    //Serial.println("Connection failed!");
  }
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
  if (bmsClient.isConnected()) {
    bmsClient.update();
    mqtt_topic = String(topic)+"/connect";
    client.publish(mqtt_topic.c_str(), "true");
    //Serial.println("\n=== BMS Data ===");
    //Serial.printf("Total Voltage:    %.2f V\n", bmsClient.getTotalVoltage());
    mqtt_topic = String(topic)+"/TotalVoltage";
    client.publish(mqtt_topic.c_str(), String(bmsClient.getTotalVoltage()).c_str());
    //Serial.printf("Current:          %.2f A\n", bmsClient.getCurrent());
    mqtt_topic = String(topic)+"/Current";
    client.publish(mqtt_topic.c_str(), String(bmsClient.getCurrent()).c_str());
    //Serial.printf("SOC:              %d %%\n", bmsClient.getSOC());
    mqtt_topic = String(topic)+"/SOC";
    client.publish(mqtt_topic.c_str(), String(bmsClient.getSOC()).c_str());
    //Serial.printf("SOH:              %s\n", bmsClient.getSOH().c_str());
    mqtt_topic = String(topic)+"/SOH";
    client.publish(mqtt_topic.c_str(), String(bmsClient.getSOH()).c_str());
    //Serial.printf("MOSFET Temp:      %d °C\n", bmsClient.getMosfetTemp());
    mqtt_topic = String(topic)+"/MOSFET Temp";
    client.publish(mqtt_topic.c_str(), String(bmsClient.getMosfetTemp()).c_str());
    //Serial.printf("Cell Temp:        %d °C\n", bmsClient.getCellTemp());
    mqtt_topic = String(topic)+"/CELL Temp";
    client.publish(mqtt_topic.c_str(), String(bmsClient.getCellTemp()).c_str());
    //Serial.printf("Remaining:        %.2f Ah\n", bmsClient.getRemainingAh());
    mqtt_topic = String(topic)+"/Remaining(AH)";
    client.publish(mqtt_topic.c_str(), String(bmsClient.getRemainingAh()).c_str());
    //Serial.printf("Full Capacity:    %.2f Ah\n", bmsClient.getFullCapacityAh());
    mqtt_topic = String(topic)+"/Full Capacity(AH)";
    client.publish(mqtt_topic.c_str(), String(bmsClient.getFullCapacityAh()).c_str());
    //Serial.printf("Protection State: %s\n", bmsClient.getProtectionState().c_str());
    //Serial.printf("Balancing:        %s\n", bmsClient.getBalancingState().c_str());
    //Serial.printf("Discharge Cycles: %u\n", bmsClient.getDischargesCount());
    mqtt_topic = String(topic)+"/Discharge Cycles";
    client.publish(mqtt_topic.c_str(), String(bmsClient.getDischargesCount()).c_str());

    printCellVoltages(bmsClient.getCellVoltages());
    //Serial.println("==================\n");
  } else {
    if (bmsClient.connect()) {
      //Serial.println("Connected to BMS!");
    } else {
      //Serial.println("Connection failed!");
      client.publish(topic, "false");
    }
  }
  delay(10000);
}