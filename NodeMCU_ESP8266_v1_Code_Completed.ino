#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <BlynkTimer.h> // Include BlynkTimer for non-blocking delays

#define BLYNK_TEMPLATE_ID "TMPL6chk6m_2o"
#define BLYNK_TEMPLATE_NAME "NodeMCU"
#define BLYNK_AUTH_TOKEN "VfoRmqTGg9toSHYTV5tfz2ioAjdwQ1yh"

// Wi-Fi credentials
char ssid[] = "Room_109";
char pass[] = "Btecfpt@2025";

// Define input pins
const int pinD6 = 12;  // GPIO12
const int pinD7 = 13;  // GPIO13

// Define virtual pins for Blynk app
#define VIRTUAL_PIN_D6 V6
#define VIRTUAL_PIN_D7 V7

// Initialize BlynkTimer
BlynkTimer timer;

// Sync virtual pins when connected to Blynk
BLYNK_CONNECTED() {
  Blynk.syncVirtual(V6);
  Blynk.syncVirtual(V7);
}

// Debounced reading to prevent signal bouncing
int readDebounced(int pin) {
  int state = digitalRead(pin);
  delay(10); // Short delay to check stability
  if (state == digitalRead(pin)) {
    return state; // Return stable state
  }
  return -1; // Invalid state, skip update
}

// Function to read and send sensor data to Blynk
void sendSensorData() {
  int stateD6 = readDebounced(pinD6);
  int stateD7 = readDebounced(pinD7);
  
  if (stateD6 != -1) {
    Blynk.virtualWrite(VIRTUAL_PIN_D6, stateD6);
    Serial.print("D6: "); Serial.print(stateD6);
  }
  if (stateD7 != -1) {
    Blynk.virtualWrite(VIRTUAL_PIN_D7, stateD7);
    Serial.print(" | D7: "); Serial.println(stateD7);
  }
}

void setup() {
  // Initialize Serial at a higher baud rate
  Serial.begin(115200);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to Wi-Fi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi connected!");
    Blynk.config(BLYNK_AUTH_TOKEN);
    Blynk.connect();
  } else {
    Serial.println("\nWi-Fi connection failed!");
    // Optionally, restart ESP8266 or enter fallback mode
  }

  // Configure D6 and D7 as input
  pinMode(pinD6, INPUT);
  pinMode(pinD7, INPUT);
  
  // Set up timer to call sendSensorData every 500ms
  timer.setInterval(500L, sendSensorData);
}

void loop() {
  // Maintain Blynk connection
  if (Blynk.connected()) {
    Blynk.run();
  } else {
    Serial.println("Blynk disconnected, attempting to reconnect...");
    Blynk.connect();
  }
  timer.run(); // Run BlynkTimer
}