#include <EEPROM.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// EEPROM settings
#define EEPROM_SIZE 512
#define PASSWORD_ADDR 0
#define PASSWORD_LENGTH 20
#define FLAG_ADDR 100 // Address to store the write flag

// WiFi settings
const char* ssid = "Chotraju";           // Replace with your Wi-Fi SSID
const char* wifi_password = "rhemanth123";    // Replace with your Wi-Fi Password

// MQTT settings
const char* mqtt_server = "cadfcd44d9104fc3a57a58ed5043a7f8.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "rhemanth";  // Replace with HiveMQ Username
const char* mqtt_password = "1Hivemqqt"; // Replace with HiveMQ Password
const char* mqtt_topic_erase = "erasePassword"; // Topic for triggering password erase
const char* mqtt_topic1 = "topic1";     // New topic 1
const char* mqtt_topic2 = "topic2";     // New topic 2
const char* mqtt_topic3 = "donterase";  // New topic 3

// Secure WiFi client and MQTT client
WiFiClientSecure espClient;
PubSubClient client(espClient);

// Button settings
#define BUTTON_PIN 4  // Button pin
bool lastButtonState = LOW;
bool buttonState = LOW;

// Test password to write
const char* testPassword = "TestPassword123";

// Variables to store topic data
int topic1Data = 0;
int topic2Data = 0;
char topic3Data[50] = ""; // Variable to store the topic3 message

// Function to write password to EEPROM
void writePasswordToEEPROM(const char* password) {
  if (EEPROM.read(FLAG_ADDR) == 1) {
    Serial.println("EEPROM flag is set. Password will not be written.");
    return;
  }

  Serial.println("Writing password to EEPROM...");
  for (int i = 0; i < PASSWORD_LENGTH; i++) {
    if (i < strlen(password)) {
      EEPROM.write(PASSWORD_ADDR + i, password[i]);
    } else {
      EEPROM.write(PASSWORD_ADDR + i, 0); // Fill remaining space with null characters
    }
  }
  EEPROM.write(FLAG_ADDR, 1); // 1 indicates password is written
  EEPROM.commit();
  Serial.println("Password written to EEPROM.");
}

// Function to read password from EEPROM
void readPasswordFromEEPROM() {
  char readPassword[PASSWORD_LENGTH + 1] = {0}; // Buffer to store password
  for (int i = 0; i < PASSWORD_LENGTH; i++) {
    readPassword[i] = EEPROM.read(PASSWORD_ADDR + i);
  }
  Serial.print("Current EEPROM Data: ");
  Serial.println(readPassword);
}

// Function to erase password from EEPROM
void erasePasswordInEEPROM() {
  Serial.println("Erasing password from EEPROM...");
  for (int i = PASSWORD_ADDR; i < PASSWORD_ADDR + PASSWORD_LENGTH; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.write(FLAG_ADDR, 0); // Reset flag
  EEPROM.commit();
  Serial.println("Password erased from EEPROM and flag reset.");
}

// Connect to Wi-Fi
void setupWiFi() {
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// Dummy function to perform operations on topic data
void dummyFunction() {
  // Perform OR operation on topic1Data and topic2Data
  int result = topic1Data || topic2Data;
  Serial.print("OR result of topic1 and topic2: ");
  Serial.println(result);

  // Check topic3Data if OR result is 1
  if (result == 1) {
    delay(5000);
    if (strcmp(topic3Data, "Authorize") != 0) {
      Serial.println("Authorization failed! Erasing password...");
      erasePasswordInEEPROM();
    } else {
      Serial.println("Authorization granted!");
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char message[50]; // Buffer for payload
  strncpy(message, (char*)payload, length);
  message[length] = '\0'; // Null-terminate the string

  Serial.print("Message arrived on topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  Serial.println(message);

  // Check if message is from topic1
  if (strcmp(topic, mqtt_topic1) == 0) {
    topic1Data = atoi(message); // Convert string to integer
    Serial.print("Topic1 data: ");
    Serial.println(topic1Data);
  }
  // Check if message is from topic2
  else if (strcmp(topic, mqtt_topic2) == 0) {
    topic2Data = atoi(message); // Convert string to integer
    Serial.print("Topic2 data: ");
    Serial.println(topic2Data);
  }
  // Check if message is from topic3
  else if (strcmp(topic, mqtt_topic3) == 0) {
    strncpy(topic3Data, message, sizeof(topic3Data)); // Store message in topic3Data
    topic3Data[sizeof(topic3Data) - 1] = '\0';       // Ensure null termination
    Serial.print("Stored topic3 data: ");
    Serial.println(topic3Data);
  }
  // Check if message is from the erasePassword topic
  else if (strcmp(topic, mqtt_topic_erase) == 0) {
    if (strcmp(message, "erase") == 0) {
      erasePasswordInEEPROM();
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT broker...");
    if (client.connect("ESP32Client", mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT broker!");
      client.subscribe(mqtt_topic_erase);
      client.subscribe(mqtt_topic1);
      client.subscribe(mqtt_topic2);
      client.subscribe(mqtt_topic3);
    } else {
      Serial.print("Failed to connect, rc=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT);

  EEPROM.begin(EEPROM_SIZE);

  writePasswordToEEPROM(testPassword);

  setupWiFi();
  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  readPasswordFromEEPROM();
  dummyFunction();

  buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == HIGH && lastButtonState == LOW) {
    Serial.println("Button pressed! Erasing password...");
    erasePasswordInEEPROM();
  }
  lastButtonState = buttonState;

  delay(1000);
}
