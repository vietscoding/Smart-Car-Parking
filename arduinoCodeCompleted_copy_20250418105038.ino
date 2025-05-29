#include <Servo.h>              // Library for controlling servo motors
#include <Wire.h>               // Library for I2C communication
#include <LiquidCrystal_I2C.h>  // Library for I2C LCD display

#define IR_THRESHOLD 100        // Threshold value for IR sensors to detect objects (0-1023)

// Define analog pins for IR sensors
const int IR_Sensor_In = A0;    // IR sensor for entrance detection
const int IR_Sensor_Out = A1;   // IR sensor for exit detection
const int IR_Sensor_Slot1 = A2; // IR sensor for parking slot 1
const int IR_Sensor_Slot2 = A3; // IR sensor for parking slot 2

// Define digital pins for slot sensor output signals
const int Slot1_Output_Pin = 2; // Output pin for slot 1 status
const int Slot2_Output_Pin = 4; // Output pin for slot 2 status

// Define servo pins
const int Servo_In_Pin = 9;     // Servo motor for entrance gate
const int Servo_Out_Pin = 10;   // Servo motor for exit gate

// Initialize servo objects
Servo servoIn;                  // Servo object for entrance gate
Servo servoOut;                 // Servo object for exit gate

// Global variables for sensor and servo states
int sensorInValue = 0;          // Value from entrance IR sensor
int sensorOutValue = 0;         // Value from exit IR sensor
int servoInPosition = 0;        // Current position of entrance servo (0-90 degrees)
int servoOutPosition = 0;       // Current position of exit servo (0-90 degrees)

// Initialize LCD object (I2C address 0x27, 16x2 display)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Function to read averaged sensor value to reduce noise
// Reads the specified analog pin 5 times and returns the average
int readAveragedSensor(int pin) {
  int total = 0;
  for (int i = 0; i < 5; i++) {
    total += analogRead(pin);   // Read analog value
    delay(1);                   // Short delay between reads for stability
  }
  return total / 5;             // Return average value
}

void setup() {
  // Attach servos to their respective pins with pulse width range (500-2500µs)
  servoIn.attach(Servo_In_Pin, 500, 2500);
  servoOut.attach(Servo_Out_Pin, 500, 2500);

  // Set initial servo positions (0 degrees)
  servoIn.write(servoInPosition);
  servoOut.write(servoOutPosition);

  // Configure slot output pins as OUTPUT
  pinMode(Slot1_Output_Pin, OUTPUT);
  pinMode(Slot2_Output_Pin, OUTPUT);

  // Initialize output pins to LOW (no signal)
  digitalWrite(Slot1_Output_Pin, LOW);
  digitalWrite(Slot2_Output_Pin, LOW);

  // Initialize LCD
  lcd.init();                   // Initialize I2C communication
  lcd.backlight();              // Turn on LCD backlight
  lcd.clear();                  // Clear the display
  lcd.setCursor(0, 0);          // Set cursor to first row
  lcd.print("Welcome!");        // Display welcome message
  lcd.setCursor(0, 1);          // Set cursor to second row
  lcd.print("Slot left: 2");    // Display initial slot count (assuming 2 slots)

  // Initialize serial communication for debugging
  Serial.begin(115200);
  while (!Serial) {}            // Wait for serial connection (if needed)
  delay(100);                   // Short delay for system stabilization
}

void loop() {
  // Read averaged values from IR sensors
  int slot1Value = readAveragedSensor(IR_Sensor_Slot1); // Slot 1 sensor value
  int slot2Value = readAveragedSensor(IR_Sensor_Slot2); // Slot 2 sensor value
  sensorInValue = readAveragedSensor(IR_Sensor_In);     // Entrance sensor value
  sensorOutValue = readAveragedSensor(IR_Sensor_Out);   // Exit sensor value

  // Map sensor values (0-1023) to servo angles (0-90 degrees)
  int targetPosIn = map(sensorInValue, 0, 1023, 0, 90);
  int targetPosOut = map(sensorOutValue, 0, 1023, 0, 90);
  // Constrain servo angles to valid range (0-90 degrees)
  targetPosIn = constrain(targetPosIn, 0, 90);
  targetPosOut = constrain(targetPosOut, 0, 90);

  // Smoothly adjust servo positions in steps of 10 degrees
  if (servoInPosition < targetPosIn) {
    servoInPosition = min(servoInPosition + 10, targetPosIn); // Move up, avoid overshoot
  } else if (servoInPosition > targetPosIn) {
    servoInPosition = max(servoInPosition - 10, targetPosIn); // Move down, avoid undershoot
  }
  if (servoOutPosition < targetPosOut) {
    servoOutPosition = min(servoOutPosition + 10, targetPosOut); // Move up, avoid overshoot
  } else if (servoOutPosition > targetPosOut) {
    servoOutPosition = max(servoOutPosition - 10, targetPosOut); // Move down, avoid undershoot
  }

  // Update servo positions
  servoIn.write(servoInPosition);
  servoOut.write(servoOutPosition);

  // Process slot status and update output pins
  int slotsLeft = 0; // Count of available parking slots
  if (slot1Value >= IR_THRESHOLD) {
    digitalWrite(Slot1_Output_Pin, LOW); // Slot 1 is empty
    slotsLeft++;
  } else {
    digitalWrite(Slot1_Output_Pin, HIGH); // Slot 1 is occupied
  }
  if (slot2Value >= IR_THRESHOLD) {
    digitalWrite(Slot2_Output_Pin, LOW); // Slot 2 is empty
    slotsLeft++;
  } else {
    digitalWrite(Slot2_Output_Pin, HIGH); // Slot 2 is occupied
  }

  // Print slot states for debugging
  Serial.print("Slot1 state: ");
  Serial.println(slot1Value < IR_THRESHOLD ? "Occupied" : "Empty");
  Serial.print("Slot2 state: ");
  Serial.println(slot2Value < IR_THRESHOLD ? "Occupied" : "Empty");

  // Update LCD display based on slot status
  static int lastSlotDisplay = -1; // Last displayed slot count
  static bool isFullDisplayed = false; // Flag to track if "FULL" is displayed
  if (slotsLeft == 0 && !isFullDisplayed) {
    lcd.clear();                // Clear LCD
    lcd.print("Status: FULL!  "); // Display full status
    isFullDisplayed = true;     // Set flag to avoid redundant updates
  } else if (slotsLeft > 0 && (lastSlotDisplay != slotsLeft || isFullDisplayed)) {
    lcd.clear();                // Clear LCD
    lcd.setCursor(0, 0);        // Set cursor to first row
    lcd.print("Welcome!");      // Display welcome message
    lcd.setCursor(0, 1);        // Set cursor to second row
    lcd.print("Slot left: ");   // Display available slots
    lcd.print(slotsLeft);
    lcd.print("   ");           // Clear remaining characters
    lastSlotDisplay = slotsLeft; // Update last displayed count
    isFullDisplayed = false;    // Reset full display flag
  }

  // Short delay to prevent system overload and reduce noise
  delay(50);
}