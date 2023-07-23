#include <DHT.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ThingSpeak.h> // Include ThingSpeak library

#define DHTPIN 0   // NodeMCU pin D3
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "OPPO A7";     // Replace with your WiFi SSID
const char* pass = "okgantiann"; // Replace with your WiFi password

const char* mqtt_server = "public.mqtthq.com"; // Replace with your MQTT server IP
const char* mqtt_topic_temperature = "home/temperature";
const char* mqtt_topic_humidity = "home/humidity";

const char* thingspeak_api_key = "87KNPABL8V7VWPKD"; // Replace with your ThingSpeak API key
const unsigned long thingspeak_channel = 2222607; // Replace with your ThingSpeak channel ID

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  delay(10);

  setupWiFi();
  client.setServer(mqtt_server, 1883);
  dht.begin();
  ThingSpeak.begin(espClient); // Initialize ThingSpeak library with the client
}

void setupWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int h = dht.readHumidity();
  int t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  char temperatureMsg[10];
  char humidityMsg[10];
  snprintf(temperatureMsg, sizeof(temperatureMsg), "%d", t);
  snprintf(humidityMsg, sizeof(humidityMsg), "%d", h);

  // Publish to MQTT
  if (client.publish(mqtt_topic_temperature, temperatureMsg)) {
    Serial.println("Temperature sent to MQTT");
  } else {
    Serial.println("Failed to send temperature to MQTT");
  }

  if (client.publish(mqtt_topic_humidity, humidityMsg)) {
    Serial.println("Humidity sent to MQTT");
  } else {
    Serial.println("Failed to send humidity to MQTT");
  }

  // Send to ThingSpeak
  ThingSpeak.setField(1, t); // Field 1 for temperature
  ThingSpeak.setField(2, h); // Field 2 for humidity

  int statusCode = ThingSpeak.writeFields(thingspeak_channel, thingspeak_api_key);

  if (statusCode == 200) {
    Serial.println("Data sent to ThingSpeak");
  } else {
    Serial.print("Failed to send data to ThingSpeak, HTTP error code: ");
    Serial.println(statusCode);
  }

  delay(10000);
}