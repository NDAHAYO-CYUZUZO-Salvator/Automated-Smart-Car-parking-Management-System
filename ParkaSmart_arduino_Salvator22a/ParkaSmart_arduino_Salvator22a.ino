#include <Nano33BLE_System.h>

#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal.h>
#include <Servo.h>
#include <SPI.h>
#include <SD.h>

// ===== MODULE OBJECTS =====
RTC_DS3231 rtc;
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
Servo gateServo;

// ===== PIN DEFINITIONS =====
#define TRIG_PIN 2
#define ECHO_PIN 3
#define SERVO_PIN 5
#define SD_CS 4

// ===== VARIABLES =====
String plateNumber = "";
unsigned long entryTime = 0;

// ===== DEBUG FUNCTION =====
void debug(String msg) {
  Serial.println("[DEBUG] " + msg);
}

// ===== ULTRASONIC SENSOR =====
long getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  long distance = duration * 0.034 / 2;
  
  return distance;
}

// ===== INITIALIZATION =====
void setup() {
  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  lcd.begin(16, 2);
  gateServo.attach(SERVO_PIN);

  // RTC
  if (!rtc.begin()) {
    debug("RTC NOT FOUND!");
    while (1);
  }

  // SD Card
  if (!SD.begin(SD_CS)) {
    debug("SD CARD FAILED!");
  } else {
    debug("SD CARD OK");
  }

  lcd.print("System Ready");
  debug("System Initialized");
}

// ===== MAIN LOOP =====
void loop() {
  long distance = getDistance();

  debug("Distance: " + String(distance));

  // VEHICLE DETECTED
  if (distance < 10) {
    handleVehicleEntry();
  }

  delay(500); // small delay for stability
}

// ===== ENTRY FUNCTION =====
void handleVehicleEntry() {
  debug("Vehicle Detected!");

  lcd.clear();
  lcd.print("Scanning Plate");

  // ===== RECEIVE FROM ESP32 =====
  if (Serial.available()) {
    plateNumber = Serial.readStringUntil('\n');
    debug("Plate: " + plateNumber);
  }

  // ===== TIME RECORD =====
  DateTime now = rtc.now();
  entryTime = now.unixtime();

  debug("Entry Time Saved");

  saveToSD(plateNumber, entryTime);

  // ===== OPEN GATE =====
  openGate();

  lcd.clear();
  lcd.print("Welcome!");
}

// ===== EXIT FUNCTION =====
void handleVehicleExit(String plate) {
  unsigned long exitTime = rtc.now().unixtime();

  unsigned long duration = exitTime - entryTime;
  float fee = calculateFee(duration);

  lcd.clear();
  lcd.print("Fee: ");
  lcd.print(fee);

  debug("Exit processed");

  openGate();
}

// ===== GATE CONTROL =====
void openGate() {
  gateServo.write(90);
  delay(3000);
  gateServo.write(0);
}

// ===== FEE CALCULATION =====
float calculateFee(unsigned long duration) {
  float hours = duration / 3600.0;
  return hours * 500; // example: 500 RWF/hour
}

// ===== SD CARD SAVE =====
void saveToSD(String plate, unsigned long time) {
  File file = SD.open("parking.txt", FILE_WRITE);

  if (file) {
    file.print(plate);
    file.print(",");
    file.println(time);
    file.close();
    debug("Saved to SD");
  } else {
    debug("SD Write Failed!");
  }
}