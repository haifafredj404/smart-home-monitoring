#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ========================================
// WIFI
// ========================================

const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ========================================
// MQTT
// ========================================

const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

const char* mqtt_topic = "smart-home-hayfa/test";

// ========================================
// SENSORS
// ========================================

#define DHT_PIN 15
#define DHT_TYPE DHT22
#define PIR_PIN 13

DHT dht(DHT_PIN, DHT_TYPE);

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ========================================
// WIFI CONNECTION
// ========================================

void connectWiFi() {

  Serial.println();
  Serial.print("Connecting to WiFi");

  WiFi.begin(ssid, password, 6);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// ========================================
// MQTT CONNECTION
// ========================================

void connectMQTT() {

  while (!mqttClient.connected()) {

    Serial.println();
    Serial.println("Connecting to MQTT...");

    // Client ID unique
    String clientId = "SmartHomeESP32-";
    clientId += String((uint32_t)ESP.getEfuseMac(), HEX);

    if (mqttClient.connect(clientId.c_str())) {

      Serial.println("MQTT connected!");

    } else {

      Serial.print("MQTT connection FAILED");
      Serial.print(" | state = ");
      Serial.println(mqttClient.state());

      delay(3000);
    }
  }
}

// ========================================
// SETUP
// ========================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("========================================");
  Serial.println("     SMART HOME MONITORING SYSTEM");
  Serial.println("========================================");

  // Sensors
  dht.begin();
  pinMode(PIR_PIN, INPUT);

  // WiFi
  connectWiFi();

  // MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);

  // Keep MQTT connection alive
  mqttClient.setKeepAlive(60);

  connectMQTT();
}

// ========================================
// LOOP
// ========================================

void loop() {

  // --------------------------------------
  // Check MQTT connection
  // --------------------------------------

  if (!mqttClient.connected()) {

    Serial.println();
    Serial.println("MQTT disconnected!");

    Serial.print("MQTT state = ");
    Serial.println(mqttClient.state());

    connectMQTT();
  }

  mqttClient.loop();

  // --------------------------------------
  // Read DHT22
  // --------------------------------------

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // --------------------------------------
  // Read PIR
  // --------------------------------------

  int motion = digitalRead(PIR_PIN);

  // --------------------------------------
  // Check DHT22
  // --------------------------------------

  if (isnan(temperature) || isnan(humidity)) {

    Serial.println("DHT22 reading error");

    delay(2000);

    return;
  }

  // --------------------------------------
  // Motion status
  // --------------------------------------

  String motionStatus;

  if (motion == HIGH) {
    motionStatus = "DETECTED";
  } else {
    motionStatus = "NONE";
  }

  // --------------------------------------
  // Create JSON
  // --------------------------------------

  String payload = "{";

  payload += "\"temperature\":";
  payload += String(temperature, 2);

  payload += ",";

  payload += "\"humidity\":";
  payload += String(humidity, 2);

  payload += ",";

  payload += "\"motion\":\"";
  payload += motionStatus;
  payload += "\"";

  payload += "}";

  // --------------------------------------
  // Display information
  // --------------------------------------

  Serial.println();
  Serial.println("------------------------------");

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.print("Motion: ");
  Serial.println(motionStatus);

  Serial.print("MQTT topic: ");
  Serial.println(mqtt_topic);

  Serial.print("MQTT message: ");
  Serial.println(payload);

  // --------------------------------------
  // Publish MQTT
  // --------------------------------------

  bool success = mqttClient.publish(
    mqtt_topic,
    payload.c_str()
  );

  if (success) {

    Serial.println("MQTT PUBLISH: SUCCESS");

  } else {

    Serial.println("MQTT PUBLISH: FAILED");

    Serial.print("MQTT state = ");
    Serial.println(mqttClient.state());
  }

  // --------------------------------------
  // Wait
  // --------------------------------------

  delay(5000);
}