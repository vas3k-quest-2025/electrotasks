#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Define the OLED dimensions
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1305 display connected to I2C (4-wire)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

// Define I2C pins for ESP32
#define OLED_SDA 16
#define OLED_SCL 17

// Define button pin
#define BUTTON_PIN 27

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Stopwatch state
enum { STOPPED, RUNNING, PAUSED } stopwatchState = STOPPED;

unsigned long startTime = 0; // Time when stopwatch started (in milliseconds)
unsigned long elapsedTime = 0; // Time elapsed since stopwatch started or last resumed (in milliseconds)
unsigned long pausedTime = 0; // Time when stopwatch was paused (in milliseconds)

// Time components
int minutes = 0;
int seconds = 0;
int tenths = 0;

// Buzzer pin (define the actual pin when connected)
#define BUZZER_PIN 2 // Example pin, change if needed

// Function to update the display
void updateDisplay() {
  display.clearDisplay();
  display.setCursor(0, 16);
  
  if (stopwatchState == RUNNING || stopwatchState == PAUSED) {
    char timeString[10]; // Buffer for formatted time
    sprintf(timeString, "%03d:%02d.%1d", minutes, seconds, tenths);
    display.println(timeString);
  } else { // STOPPED
     display.println("000:00.0");
  }
  
  display.display();
}

void handleButton() {
  if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("Button pressed.");

      changeClockState();

      updateDisplay();
      while(digitalRead(BUTTON_PIN) == LOW);
    }
}

void changeClockState() {
    if (stopwatchState == RUNNING) {
        pausedTime = millis() - startTime;
        stopwatchState = PAUSED;
    } else {
        if (stopwatchState == STOPPED) {
            startTime = millis();
            elapsedTime = 0;
            minutes = 0;
            seconds = 0;
            tenths = 0;
        } else { // PAUSED
            startTime = millis() - pausedTime;
        }
        stopwatchState = RUNNING;
    }

    tone(BUZZER_PIN, 1000, 500);
}

void setup() {
  Serial.begin(115200);

  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  display.display();
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  updateDisplay();

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  handleButton();

  // Stopwatch update logic
  if (stopwatchState == RUNNING) {
    unsigned long currentTime = millis();
    elapsedTime = currentTime - startTime;

    // Calculate time components
    tenths = (elapsedTime / 100) % 10;
    seconds = (elapsedTime / 1000) % 60;
    minutes = (elapsedTime / 60000) % 1000; // Up to 999 minutes

    // Update display (only update periodically to avoid flickering and save resources)
    // For simplicity now, update every tenth of a second
    static unsigned long lastDisplayUpdateTime = 0;
    if (currentTime - lastDisplayUpdateTime >= 100) { // Update every 100ms (tenth of a second)
      lastDisplayUpdateTime = currentTime;
      
      // Update display
      updateDisplay();

      if (tenths == 0 && seconds == 0 && elapsedTime > 0) {
         tone(BUZZER_PIN, 500, 500);
      }
    }
  } else if (stopwatchState == PAUSED) {
  }

  delay(1); 
}
