#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// === Pin Definitions ===
#define SENSOR_EMPTY A0
#define SENSOR_MID   A1
#define SENSOR_FULL  A2
#define PUMP_PIN 4
#define BUZZER_PIN 6
#define BUTTON_PIN 12

#define LED_EMPTY 9
#define LED_MID 8
#define LED_FULL 10

// === LCD ===
LiquidCrystal_I2C lcd(0x27, 16, 2);

bool pumpState = false;
int threshold = 400;  // Adjust after calibration

bool lastEmpty = false;
bool lastMid   = false;
bool lastFull  = false;

void setup() {
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  pinMode(LED_EMPTY, OUTPUT);
  pinMode(LED_MID, OUTPUT);
  pinMode(LED_FULL, OUTPUT);

  digitalWrite(PUMP_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Water Tank Sys");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();

  Serial.begin(9600);
}

void loop() {
  static bool lastButton = HIGH;
  bool buttonState = digitalRead(BUTTON_PIN);

  // === MANUAL BUTTON OVERRIDE ===
  if (lastButton == HIGH && buttonState == LOW) {
    pumpState = !pumpState;       // Toggle pump ON/OFF
    digitalWrite(PUMP_PIN, pumpState);
    beep(2000);                   // 2 sec beep
  }
  lastButton = buttonState;

  // === SENSOR READINGS ===
  int emptyVal = analogRead(SENSOR_EMPTY);
  int midVal   = analogRead(SENSOR_MID);
  int fullVal  = analogRead(SENSOR_FULL);

  bool empty = emptyVal < threshold;
  bool mid   = midVal   > threshold;
  bool full  = fullVal  > threshold;

  // Debug values
  Serial.print("Empty: "); Serial.print(emptyVal);
  Serial.print(" | Mid: "); Serial.print(midVal);
  Serial.print(" | Full: "); Serial.println(fullVal);

  // === LED STATUS (only one ON at a time) ===
  if (full) {
    digitalWrite(LED_FULL, HIGH);
    digitalWrite(LED_MID, LOW);
    digitalWrite(LED_EMPTY, LOW);
  } else if (mid) {
    digitalWrite(LED_MID, HIGH);
    digitalWrite(LED_FULL, LOW);
    digitalWrite(LED_EMPTY, LOW);
  } else if (empty) {
    digitalWrite(LED_EMPTY, HIGH);
    digitalWrite(LED_MID, LOW);
    digitalWrite(LED_FULL, LOW);
  } else {
    // Unknown: all LEDs OFF
    digitalWrite(LED_EMPTY, LOW);
    digitalWrite(LED_MID, LOW);
    digitalWrite(LED_FULL, LOW);
  }

  // === AUTO CONTROL ONLY IF NOT MANUALLY FORCED ===
  if (!digitalRead(BUTTON_PIN) && pumpState) {
    // do nothing, manual override keeps pump ON
  } else if (!pumpState) { 
    if ((empty || mid || (!empty && !mid && !full)) && !full) {  
      // If level is Empty OR Mid OR Unknown → Pump ON
      pumpState = true;
      digitalWrite(PUMP_PIN, HIGH);
      beep(2000);
    } 
  }

  // Pump OFF if Full detected
  if (full && !lastFull) {
    pumpState = false;
    digitalWrite(PUMP_PIN, LOW);
    beep(2000);
  }

  // Save last states
  lastEmpty = empty;
  lastMid   = mid;
  lastFull  = full;

  // === LCD DISPLAY ===
  lcd.setCursor(0, 0);
  if (full) lcd.print("Level: FULL     ");
  else if (mid) lcd.print("Level: MID      ");
  else if (empty) lcd.print("Level: LOW    ");
  else lcd.print("Level: LOW  ");

  lcd.setCursor(0, 1);
  if (pumpState) lcd.print("Pump: ON        ");
  else lcd.print("Pump: OFF       ");

  delay(300);
}

void beep(int duration) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}
