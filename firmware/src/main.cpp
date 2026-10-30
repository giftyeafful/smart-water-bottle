
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#ifndef SIMULATION
#include <HX711.h>
#endif

// ESP32 pin connections
constexpr int LED_PIN = 18;
constexpr int BUTTON_PIN = 15;
constexpr int BUZZER_PIN = 19;
constexpr int SDA_PIN = 21;
constexpr int SCL_PIN = 22;
constexpr int HX_DT = 4;
constexpr int HX_SCK = 5;
constexpr int SIM_ADC = 34;

// Bottle settings
constexpr float CAPACITY_ML = 750.0f;
constexpr float HX_CALIBRATION = -420.0f;

// Timing settings for testing
constexpr unsigned long REMINDER_MS = 10000UL;
constexpr unsigned long ALERT_MS = 3000UL;
constexpr unsigned long SAMPLE_MS = 300UL;

// Drink detection settings
constexpr float DRINK_THRESHOLD_ML = 8.0f;
constexpr float NOISE_TOLERANCE_ML = 3.0f;

// OLED display
Adafruit_SSD1306 display(128, 64, &Wire, -1);

#ifndef SIMULATION
HX711 scale;
#endif

// Water tracking
float currentMl = 0;
float previousMl = 0;
float consumedMl = 0;

// Timer variables
unsigned long lastSample = 0;
unsigned long lastReminder = 0;
unsigned long alertStart = 0;
unsigned long lastButton = 0;

// System states
bool alertActive = false;
bool displayReady = false;
bool initialized = false;

// Measure water level
float measureMl() {

#ifdef SIMULATION

  // Read simulated water level
  int raw = analogRead(SIM_ADC);

  return constrain(
    raw * (CAPACITY_ML / 4095.0f),
    0.0f,
    CAPACITY_ML
  );

#else

  // Read physical load cell
  if (!scale.is_ready()) {
    return NAN;
  }

  float grams = scale.get_units(3);

  return constrain(
    grams,
    0.0f,
    CAPACITY_ML
  );

#endif
}

// Start hydration reminder
void startAlert(unsigned long now) {

  if (alertActive) return;

  alertActive = true;
  alertStart = now;

  digitalWrite(LED_PIN, HIGH);
  tone(BUZZER_PIN, 1800);

  Serial.println("REMINDER: Time to hydrate!");
}

// Stop hydration reminder
void stopAlert() {

  if (!alertActive) return;

  alertActive = false;

  noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  Serial.println("REMINDER: Alert stopped.");
}

// Update OLED screen
void drawScreen() {

  if (!displayReady) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Display title
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("SMART WATER BOTTLE");

  // Display current water level
  display.setTextSize(2);
  display.setCursor(0, 16);
  display.print((int)roundf(currentMl));
  display.println(" mL");

  // Display total consumption
  display.setTextSize(1);
  display.setCursor(0, 42);
  display.print("Drank: ");
  display.print((int)roundf(consumedMl));
  display.println(" mL");

  // Display reminder status
  display.setCursor(0, 54);

  if (alertActive) {
    display.print("TIME TO HYDRATE!");
  } else {
    display.print("Press button: reset");
  }

  display.display();
}

// Initialize system
void setup() {

  Serial.begin(115200);

  // Configure pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(SIM_ADC, INPUT);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Initialize OLED
  Wire.begin(SDA_PIN, SCL_PIN);

  displayReady = display.begin(
    SSD1306_SWITCHCAPVCC,
    0x3C
  );

  Serial.println(
    displayReady ? "OLED OK" : "OLED FAILED"
  );

#ifndef SIMULATION

  // Initialize physical load cell
  scale.begin(HX_DT, HX_SCK);
  scale.set_scale(HX_CALIBRATION);

  Serial.println("Place EMPTY bottle on sensor.");
  delay(3000);

  if (scale.is_ready()) {
    scale.tare(15);
  } else {
    Serial.println("ERROR: HX711 not ready.");
  }

#else

  // Initialize simulation input
  analogReadResolution(12);
  Serial.println("SIMULATION MODE ACTIVE");

#endif

  // Start reminder countdown
  lastReminder = millis();

  drawScreen();

  Serial.println("Smart Water Bottle started.");
}

// Main program loop
void loop() {

  unsigned long now = millis();

  // Reset button
  if (
    digitalRead(BUTTON_PIN) == LOW &&
    now - lastButton > 350
  ) {

    lastButton = now;
    consumedMl = 0;

    // Reset consumption only during alarm testing
    Serial.println("Session reset");

    drawScreen();
  }

  // Monitor water level
  if (now - lastSample >= SAMPLE_MS) {

    lastSample = now;

    float reading = measureMl();

    if (!isnan(reading)) {

      currentMl = reading;

      if (!initialized) {

        previousMl = currentMl;
        initialized = true;

      } else {

        float delta = previousMl - currentMl;

        // Detect water consumption
        if (delta >= DRINK_THRESHOLD_ML) {

          consumedMl += delta;

          // Do not interrupt reminder during testing
          Serial.printf(
            "Drink detected: %.1f mL; total %.1f mL\n",
            delta,
            consumedMl
          );
        }

        // Update previous reading
        if (
          fabsf(currentMl - previousMl) >=
          NOISE_TOLERANCE_ML
        ) {
          previousMl = currentMl;
        }
      }
    }

    drawScreen();
  }

  // Start reminder after 10 seconds
  if (
    !alertActive &&
    now - lastReminder >= REMINDER_MS
  ) {

    startAlert(now);
    drawScreen();
  }

  // Stop reminder after 3 seconds
  if (
    alertActive &&
    now - alertStart >= ALERT_MS
  ) {

    Serial.printf(
      "ALERT DURATION: %lu ms\n",
      now - alertStart
    );

    stopAlert();

    // Restart countdown
    lastReminder = now;

    drawScreen();
  }
}
