#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Display settings
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address. 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pin definitions
const int BUTTON_PIN = 27;
const int TRIG_PIN = 12;
const int ECHO_PIN = 14;

// Distance measurement variables

long duration;
int distance;

// State variables
bool isMeasuring = false;
unsigned long measurementStartTime = 0;
unsigned long lastMeasurementTime = 0;
const unsigned long MEASUREMENT_INTERVAL = 300; // Measure every 300 ms
const unsigned long MEASUREMENT_DURATION = 5000; // Measure for 5 seconds

void setup() {
  Serial.begin(115200);

  // Initialize display
  Wire.begin(16, 17); // SDA, SCL
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
  display.display();

  // Pin modes
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Use internal pull-up resistor
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.println("System ready. Waiting for button press.");
  displayMessage("Waiting for button...");
}

void loop() {
  if (!isMeasuring) {
    // Waiting for button press
    if (digitalRead(BUTTON_PIN) == LOW) { // Button is pressed (assuming active low)
      Serial.println("Button pressed. Starting measurement.");
      isMeasuring = true;
      measurementStartTime = millis();
      lastMeasurementTime = millis() - MEASUREMENT_INTERVAL; // To trigger immediate first measurement
      displayMessage("Measuring...");
    }
  } else {
    // Measuring state
    unsigned long currentTime = millis();

    // Check if 5 seconds have passed
    if (currentTime - measurementStartTime >= MEASUREMENT_DURATION) {
      Serial.println("5 seconds passed. Stopping measurement.");
      isMeasuring = false;
      displayMessage("Measurement complete.\nWaiting for button...");
      return; // Exit this loop iteration to return to button waiting
    }

    // Perform measurement if interval has passed
    if (currentTime - lastMeasurementTime >= MEASUREMENT_INTERVAL) {
      lastMeasurementTime = currentTime;
      measureDistance();
      displayDistance(distance);
    }
  }
}

void measureDistance() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH);
  distance = duration * 0.0344 / 2;

  // Debug output
  Serial.print("Duration: ");
  Serial.println(duration);
  Serial.print("Distance (calc): ");
  Serial.println(distance);
}

void displayMessage(const char* message) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println(message);
  display.display();
}

void displayDistance(int dist) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.print("Dist: ");
  display.print(dist);
  display.println(" cm");
  display.display();
} 