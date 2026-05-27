#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;
RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servo;

// Pin Definitions
#define SERVO_PIN   13
#define BUZZER_PIN  14
#define LED_PIN     12
#define BUTTON_PIN  27

struct PillSlot {
  int hour;
  int minute;
  String name;
  int angle;
};

PillSlot slots[3];

bool dispensing = false;
int currentSlot = -1;
bool alreadyDispensed[3] = {false, false, false};

void setup() {
  Serial.begin(115200);

  // Bluetooth
  SerialBT.begin("SmartPillDispenser");

  // RTC
  Wire.begin();
  rtc.begin();

  // LCD
  lcd.begin();
  lcd.backlight();

  // Pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // Using INPUT_PULLUP
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  // Servo
  servo.attach(SERVO_PIN);
  servo.write(0);

  // Welcome Message
  lcd.setCursor(0, 0);
  lcd.print("Smart Dispenser");
  delay(2000);
  lcd.clear();
}

void loop() {

  DateTime now = rtc.now();

  // ---------------- DISPLAY CURRENT TIME ----------------
  if (!dispensing) {
    lcd.setCursor(0, 0);
    lcd.print("Time: ");

    if (now.hour() < 10) lcd.print("0");
    lcd.print(now.hour());

    lcd.print(":");

    if (now.minute() < 10) lcd.print("0");
    lcd.print(now.minute());

    lcd.print("   ");
  }

  // ---------------- BLUETOOTH SETUP ----------------
  if (SerialBT.available()) {

    String input = SerialBT.readStringUntil('\n');
    input.trim();

    // Format:
    // slot,hour,minute,pillname
    // Example:
    // 1,10,30,Paracetamol

    int slot = input.substring(0, input.indexOf(',')).toInt();
    input = input.substring(input.indexOf(',') + 1);

    int hour = input.substring(0, input.indexOf(',')).toInt();
    input = input.substring(input.indexOf(',') + 1);

    int minute = input.substring(0, input.indexOf(',')).toInt();
    String pillName = input.substring(input.indexOf(',') + 1);

    slots[slot - 1].hour = hour;
    slots[slot - 1].minute = minute;
    slots[slot - 1].name = pillName;

    // Servo angles
    if (slot == 1)
      slots[slot - 1].angle = 60;
    else if (slot == 2)
      slots[slot - 1].angle = 120;
    else if (slot == 3)
      slots[slot - 1].angle = 180;

    // Insert Pill Mode
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Insert the pill");

    lcd.setCursor(0, 1);
    lcd.print(pillName);

    // Rotate tray for inserting pill
    servo.write(slots[slot - 1].angle);

    dispensing = true;
    currentSlot = slot - 1;

    // No buzzer during inserting
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }

  // ---------------- BUTTON PRESS ----------------
  if (dispensing && digitalRead(BUTTON_PIN) == LOW) {

    delay(200);

    if (digitalRead(BUTTON_PIN) == LOW) {

      // Return to home position
      servo.write(0);

      digitalWrite(LED_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Smart Dispenser");

      delay(1500);

      lcd.clear();

      dispensing = false;
      currentSlot = -1;
    }
  }

  // ---------------- AUTOMATIC DISPENSING ----------------
  if (!dispensing) {

    for (int i = 0; i < 3; i++) {

      if (now.hour() == slots[i].hour &&
          now.minute() == slots[i].minute &&
          !alreadyDispensed[i]) {

        lcd.clear();

        lcd.setCursor(0, 0);
        lcd.print("Take Pill:");

        lcd.setCursor(0, 1);
        lcd.print(slots[i].name);

        // Rotate Servo
        servo.write(slots[i].angle);

        // Alerts ON
        digitalWrite(LED_PIN, HIGH);
        digitalWrite(BUZZER_PIN, HIGH);

        dispensing = true;
        currentSlot = i;

        alreadyDispensed[i] = true;

        break;
      }
    }
  }

  // Reset dispensing flag after minute changes
  for (int i = 0; i < 3; i++) {

    if (now.minute() != slots[i].minute) {
      alreadyDispensed[i] = false;
    }
  }

  delay(300);
}