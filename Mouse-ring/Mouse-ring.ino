/*
 * ESP32-C3 Bluetooth Mouse Ring
 * FINAL STABLE VERSION - No OLED (hardware conflict on your board)
 */

#include <BleMouse.h>

#define DEVICE_NAME "Mouse Ring"

// Your working joystick mapping
#define PIN_JOY_UP     6
#define PIN_JOY_DOWN   3
#define PIN_JOY_LEFT   1
#define PIN_JOY_RIGHT  4
#define PIN_JOY_CENTER 0
#define LED_PIN 8

// Mouse settings
#define MOVE_SPEED_INITIAL   4
#define MOVE_SPEED_ACCEL     10
#define ACCEL_DELAY         300
#define LONG_PRESS_TIME    500
#define SLEEP_PRESS_TIME   5000
#define IDLE_TIMEOUT      300000

BleMouse bleMouse(DEVICE_NAME, "DIY", 100);

unsigned long lastActivityTime = 0;
unsigned long centerPressTime = 0;
bool centerWasPressed = false;
bool isMoving = false;
unsigned long moveStartTime = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n╔═══════════════════════════════╗");
  Serial.println("║  ESP32-C3 BLE Mouse Ring      ║");
  Serial.println("║  Stable Version               ║");
  Serial.println("╚═══════════════════════════════╝\n");
  
  pinMode(PIN_JOY_UP, INPUT_PULLUP);
  pinMode(PIN_JOY_DOWN, INPUT_PULLUP);
  pinMode(PIN_JOY_LEFT, INPUT_PULLUP);
  pinMode(PIN_JOY_RIGHT, INPUT_PULLUP);
  pinMode(PIN_JOY_CENTER, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  
  Serial.println("✓ GPIO configured");
  Serial.println("Starting BLE Mouse...");
  
  bleMouse.begin();
  
  Serial.println("✓ Ready!\n");
  Serial.println("Mac: System Preferences → Bluetooth");
  Serial.println("     Connect to 'Mouse Ring'\n");
  Serial.println("Controls:");
  Serial.println("  • Move joystick = cursor");
  Serial.println("  • Quick press = left click");
  Serial.println("  • Hold 0.5s = right click");
  Serial.println("  • Hold 5s = sleep");
  Serial.println("\nNote: OLED disabled due to hardware conflict\n");
  
  lastActivityTime = millis();
}

void loop() {
  // LED blink
  static unsigned long lastBlink = 0;
  unsigned long blinkInterval = bleMouse.isConnected() ? 2000 : 250;
  if (millis() - lastBlink > blinkInterval) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    lastBlink = millis();
  }
  
  if (bleMouse.isConnected()) {
    handleMouseMovement();
    handleMouseClicks();
    checkSleep();
  }
  
  delay(10);
}

void handleMouseMovement() {
  int moveX = 0, moveY = 0;
  bool anyDirection = false;
  
  if (digitalRead(PIN_JOY_UP) == LOW) {
    moveY = -MOVE_SPEED_INITIAL;
    anyDirection = true;
  }
  if (digitalRead(PIN_JOY_DOWN) == LOW) {
    moveY = MOVE_SPEED_INITIAL;
    anyDirection = true;
  }
  if (digitalRead(PIN_JOY_LEFT) == LOW) {
    moveX = -MOVE_SPEED_INITIAL;
    anyDirection = true;
  }
  if (digitalRead(PIN_JOY_RIGHT) == LOW) {
    moveX = MOVE_SPEED_INITIAL;
    anyDirection = true;
  }
  
  if (anyDirection) {
    if (!isMoving) {
      isMoving = true;
      moveStartTime = millis();
    }
    
    unsigned long moveDuration = millis() - moveStartTime;
    if (moveDuration > ACCEL_DELAY) {
      if (moveX != 0) moveX = (moveX > 0) ? MOVE_SPEED_ACCEL : -MOVE_SPEED_ACCEL;
      if (moveY != 0) moveY = (moveY > 0) ? MOVE_SPEED_ACCEL : -MOVE_SPEED_ACCEL;
    }
    
    bleMouse.move(moveX, moveY);
    lastActivityTime = millis();
  } else {
    isMoving = false;
  }
}

void handleMouseClicks() {
  bool centerPressed = (digitalRead(PIN_JOY_CENTER) == LOW);
  
  if (centerPressed && !centerWasPressed) {
    centerPressTime = millis();
    centerWasPressed = true;
  }
  
  if (!centerPressed && centerWasPressed) {
    unsigned long pressDuration = millis() - centerPressTime;
    centerWasPressed = false;
    
    if (pressDuration >= SLEEP_PRESS_TIME) {
      Serial.println("→ Entering sleep mode");
      delay(500);
      enterDeepSleep();
      return;
    }
    
    if (pressDuration >= LONG_PRESS_TIME) {
      Serial.println("→ RIGHT CLICK");
      bleMouse.click(MOUSE_RIGHT);
    } else {
      Serial.println("→ LEFT CLICK");
      bleMouse.click(MOUSE_LEFT);
    }
    
    lastActivityTime = millis();
  }
}

void checkSleep() {
  if (millis() - lastActivityTime > IDLE_TIMEOUT) {
    Serial.println("→ Idle timeout");
    enterDeepSleep();
  }
}

void enterDeepSleep() {
  Serial.println("💤 Going to sleep");
  Serial.println("Press center button to wake\n");
  delay(500);
  
  esp_deep_sleep_enable_gpio_wakeup(1ULL << PIN_JOY_CENTER, ESP_GPIO_WAKEUP_GPIO_LOW);
  esp_deep_sleep_start();
}