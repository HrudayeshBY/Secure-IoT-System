#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "mbedtls/aes.h"

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
#define LED_PIN 2  // Pin where the onboard LED is connected
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

// AES Decrypt function
void decryptData(uint8_t *input, uint8_t *output, uint8_t *key) {
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);

    // Set decryption key
    mbedtls_aes_setkey_dec(&aes, key, 128);

    // Decrypt input ciphertext
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, input, output);

    mbedtls_aes_free(&aes);
}

// Connect to Wi-Fi
void setup_wifi() {
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// Callback for handling incoming MQTT messages
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);

  // Decrypt the received message
  uint8_t key[16] = {'M', 'y', 'S', 'e', 'c', 'r', 'e', 't', 'A', 'E', 'S', 'K', 'e', 'y', '!', '!'};
  uint8_t decryptedMessage[16] = {0};
  
  decryptData(payload, decryptedMessage, key);  // Decrypt message payload

  // Print the decrypted message as hexadecimal for better visibility
  Serial.print("Decrypted Message (Hex): ");
  for (int i = 0; i < 16; i++) {
    Serial.print(decryptedMessage[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Convert decrypted message to a string
  String message = String((char*)decryptedMessage);
  message.trim();  // Remove any trailing spaces or null characters

  // Print the decrypted message as string
  Serial.print("Decrypted Message (String): ");
  Serial.println(message);

  // Check decrypted message content
  if (strcmp(message.c_str(), "Yesmovement") == 0) {
    digitalWrite(LED_PIN, HIGH);  // Turn LED on
    Serial.println("LED ON");
  } else if (strcmp(message.c_str(), "No movement") == 0) {
    digitalWrite(LED_PIN, LOW);   // Turn LED off
    Serial.println("LED OFF");
  } else {
    Serial.println("Unexpected Message");
  }
}

// Reconnect to MQTT Broker if disconnected
void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect("ESP32Receiver", mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT broker!");
      client.subscribe(topic);  // Subscribe to the same topic
    } else {
      Serial.print("Failed to connect, rc=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Configure LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // Start with LED off
  pinMode(LDR1PIN, INPUT);
  pinMode(LDR2PIN, INPUT);
  pinMode(LDR3PIN, INPUT);

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

  ldr1Value = analogRead(LDR1PIN);
  ldr2Value = analogRead(LDR2PIN);
  ldr3Value = analogRead(LDR3PIN);

  if(ldr1Value != 0 || ldr2Value != 0 || ldr3Value != 0 )
  {
    Serial.println("Enclosure Tampered");
    client.publish("topic2","1");//1 implies tampered box open
  }
  else
  {
    Serial.println("Enclosure Safe");
    client.publish("topic2","0");//0 implies not tampered box close
  }
  delay(1000);
}