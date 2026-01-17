#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// RFID setup
#define RST_PIN 9
#define SS_PIN 10
MFRC522 mfrc522(SS_PIN, RST_PIN);

// LCD setup (0x27 or 0x3F based on module)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// LEDs
int red1 = 2, orange1 = 3, green1 = 4;
int red2 = 5, orange2 = 6, green2 = 7;

// IR Sensor
int irSensor = 8;

// Current lane
int currentLane = 1;

// RFID card IDs
byte lane1Card[4] = {0x43, 0x89, 0x6C, 0xFA};
byte lane2Card[4] = {0xA3, 0xBB, 0xE7, 0x34};

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();

  pinMode(red1, OUTPUT); pinMode(orange1, OUTPUT); pinMode(green1, OUTPUT);
  pinMode(red2, OUTPUT); pinMode(orange2, OUTPUT); pinMode(green2, OUTPUT);
  pinMode(irSensor, INPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Traffic Sys");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();

  allOff();
  Serial.println("🚦 Smart Traffic System Initialized");
}

void loop() {
  currentLane = 1;
  lcdMsg("Normal Cycle", "Lane 1 GREEN");
  lane1Cycle();

  currentLane = 2;
  lcdMsg("Normal Cycle", "Lane 2 GREEN");
  lane2Cycle();
}

// ====================== Lane Cycles ======================
void lane1Cycle() {
  digitalWrite(red2, HIGH);
  digitalWrite(red1, LOW);
  digitalWrite(green1, HIGH);

  if (!priorityDelay(5000)) return;

  digitalWrite(green1, LOW);
  digitalWrite(orange1, HIGH);
  if (!priorityDelay(3000)) return;

  digitalWrite(orange1, LOW);
  digitalWrite(red1, HIGH);
}

void lane2Cycle() {
  digitalWrite(red1, HIGH);
  digitalWrite(red2, LOW);
  digitalWrite(green2, HIGH);

  if (!priorityDelay(5000)) return;

  digitalWrite(green2, LOW);
  digitalWrite(orange2, HIGH);
  if (!priorityDelay(3000)) return;

  digitalWrite(orange2, LOW);
  digitalWrite(red2, HIGH);
}

// ====================== Priority Delay ======================
bool priorityDelay(int totalDelay) {
  int interval = 100;
  int elapsed = 0;

  while (elapsed < totalDelay) {
    // RFID priority
    int rfidLane = checkRFID();
    if (rfidLane != 0) {
      Serial.print("RFID detected for Lane ");
      Serial.println(rfidLane);
      lcdMsg("RFID Priority", "Lane " + String(rfidLane) + " Active");
      giveLanePriority(rfidLane);
      lcdMsg("Resuming", "Prev Lane...");
      resumePreviousLane();
      return false;
    }

    // IR detection
    if (digitalRead(irSensor) == LOW && currentLane != 1) {
      Serial.println("IR detected - Lane 1 priority");
      lcdMsg("IR Priority", "Lane 1 Active");
      giveLanePriority(1);
      lcdMsg("Resuming", "Prev Lane...");
      resumePreviousLane();
      return false;
    }

    delay(interval);
    elapsed += interval;
  }
  return true;
}

// ====================== RFID Functions ======================
int checkRFID() {
  if (!mfrc522.PICC_IsNewCardPresent()) return 0;
  if (!mfrc522.PICC_ReadCardSerial()) return 0;

  byte *id = mfrc522.uid.uidByte;

  if (compareIDs(id, lane1Card)) return 1;
  if (compareIDs(id, lane2Card)) return 2;

  return 0;
}

bool compareIDs(byte *id, byte *card) {
  for (byte i = 0; i < 4; i++) {
    if (id[i] != card[i]) return false;
  }
  return true;
}

// ====================== Priority Execution ======================
void giveLanePriority(int lane) {
  allOff();
  if (lane == 1) {
    Serial.println("➡️ Lane 1 PRIORITY ACTIVE");
    digitalWrite(red2, HIGH);
    digitalWrite(green1, HIGH);
  } else {
    Serial.println("➡️ Lane 2 PRIORITY ACTIVE");
    digitalWrite(red1, HIGH);
    digitalWrite(green2, HIGH);
  }

  delay(5000);
  if (lane == 1) {
    digitalWrite(green1, LOW);
    digitalWrite(orange1, HIGH);
  } else {
    digitalWrite(green2, LOW);
    digitalWrite(orange2, HIGH);
  }

  delay(3000);
  allOff();
}

// ====================== Resume Control ======================
void resumePreviousLane() {
  Serial.print("🔁 Resuming previous lane: ");
  Serial.println(currentLane);
  if (currentLane == 1) lane1Cycle();
  else lane2Cycle();
}

// ====================== LCD Helper ======================
void lcdMsg(String line1, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

// ====================== Utility ======================
void allOff() {
  digitalWrite(red1, LOW);
  digitalWrite(orange1, LOW);
  digitalWrite(green1, LOW);
  digitalWrite(red2, LOW);
  digitalWrite(orange2, LOW);
  digitalWrite(green2, LOW);
}
