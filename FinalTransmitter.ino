#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <NewPing.h>
#include "mbedtls/aes.h"

// Ultrasonic sensor pins
#define TRIG_PIN 18
#define ECHO_PIN 19

// Maximum distance to measure (in cm)
#define MAX_DISTANCE 400

// Create an instance of the ultrasonic sensor
NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);

// WiFi Credentials
const char* ssid = "Chotraju";  // Replace with your Wi-Fi SSID
const char* password = "rhemanth123";  // Replace with your Wi-Fi Password

// HiveMQ Cloud Broker Settings
const char* mqtt_server = "cadfcd44d9104fc3a57a58ed5043a7f8.s1.eu.hivemq.cloud";
const int mqtt_port = 8883; // Secure MQTT Port
const char* mqtt_username = "rhemanth";  // Replace with HiveMQ Username
const char* mqtt_password = "1Hivemqqt";  // Replace with HiveMQ Password

// MQTT Topic
const char* topic = "/TroopMovement";

// Hardware Pins
#define LED_PIN 2
#define LDR1PIN 36
#define LDR2PIN 35
#define LDR3PIN 39

//Values for storing ldr values
int ldr1Value = 0;
int ldr2Value = 0;
int ldr3Value = 0;

// Secure WiFi Client and MQTT Client
WiFiClientSecure espClient;
PubSubClient client(espClient);

// AES Encryption Function
void encryptAES(const uint8_t *key, const uint8_t *input, uint8_t *output, size_t length) {
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);

    // Set encryption key (128 bits = 16 bytes)
    mbedtls_aes_setkey_enc(&aes, key, 128);

    // Encrypt in ECB mode
    for (size_t i = 0; i < length; i += 16) {
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, input + i, output + i);
    }

    mbedtls_aes_free(&aes);
}

// Connect to Wi-Fi
void setup_wifi() {
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("...");
  }

  Serial.println("\nWi-Fi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// Callback for handling incoming MQTT messages (if needed)
void callback(char* topic, byte* payload, unsigned int length) {
  char msg[50];
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);

  for (unsigned int i = 0; i < length; i++) {
    msg[i] = (char)payload[i];
  }
  msg[length] = '\0';
}

// Reconnect to MQTT Broker if disconnected
void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect("ESP32Client", mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT broker!");
      client.subscribe("/LedControl");  // Subscribe to LED control topic if needed
    } else {
      Serial.print("Failed to connect, rc=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Configure pins
  pinMode(LED_PIN, OUTPUT);

  // Connect to Wi-Fi
  setup_wifi();

  // Set up MQTT connection
  espClient.setInsecure();  // Use secure connection without explicit certificate
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  // Ensure MQTT client is connected
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Measure distance with ultrasonic sensor
  long distance = sonar.ping_cm();

  // Prepare message
  uint8_t message[16];
  uint8_t encryptedMessage[16];
  uint8_t key[16] = {'M', 'y', 'S', 'e', 'c', 'r', 'e', 't', 'A', 'E', 'S', 'K', 'e', 'y', '!', '!'};

  // If distance is less than threshold (indicating movement detected)
  if (distance > 0 && distance < 100) {  // Adjust threshold as needed
    Serial.println("Troop movement detected!");
    client.publish("topic3", "Movement detected");

    char msg[] = "Movement detected";
    memset(message, 0, sizeof(message));
    memcpy(message, msg, sizeof(msg) > sizeof(message) ? sizeof(message) : sizeof(msg));
  } else {
    Serial.println("NO Troop movement detected!");
    client.publish("topic3", "No movement");
    char msg[] = "No movement";
    memset(message, 0, sizeof(message));
    memcpy(message, msg, sizeof(msg) > sizeof(message) ? sizeof(message) : sizeof(msg));
  }

  ldr1Value = analogRead(LDR1PIN);
  ldr2Value = analogRead(LDR2PIN);
  ldr3Value = analogRead(LDR3PIN);

  if(ldr1Value != 0 || ldr2Value != 0 || ldr3Value != 0 )
  {
    Serial.println("Enclosure Tempered");
    client.publish("topic1","1");
  }
  else
  {
    Serial.println("Enclosure Safe");
    client.publish("topic1","0");

  }

  // Encrypt the message
  encryptAES(key, message, encryptedMessage, sizeof(message));

  // Print encrypted message in hexadecimal format (for debugging)
  Serial.println("Encrypted message:");
  for (int i = 0; i < sizeof(encryptedMessage); i++) {
    Serial.printf("%02X ", encryptedMessage[i]);
  }
  Serial.println();

  // Publish encrypted message to MQTT
  client.publish(topic, encryptedMessage, sizeof(encryptedMessage));
  //client.publish("topic1", "000");
  delay(1000);  // Delay to avoid flooding the MQTT broker
}
