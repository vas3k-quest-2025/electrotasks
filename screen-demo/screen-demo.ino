#include "Display_ST7789.h"
#include "LVGL_Driver.h"
#include "ui.h"
#include <WiFi.h>
#include <time.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// WiFi credentials
const char* ssid = "ssid";
const char* password = "password";

// NTP settings
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;  // UTC+1 для Белграда
const int daylightOffset_sec = 3600;  // +1 час для летнего времени

// Target date constant
const char* TARGET_DATE = "2025-06-05 12:00:00";

// WiFi and NTP objects
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, daylightOffset_sec);

// Function to get time difference strings
std::pair<String, String> getTimeDifference() {
    struct tm target_tm;
    strptime(TARGET_DATE, "%Y-%m-%d %H:%M:%S", &target_tm);
    time_t target_time = mktime(&target_tm);
    
    time_t current_time = time(nullptr);
    
    if (current_time >= target_time) {
        return {"00", "00:00:00"};
    }
    
    time_t diff = target_time - current_time;
    int days = diff / (24 * 3600);
    diff = diff % (24 * 3600);
    int hours = diff / 3600;
    diff = diff % 3600;
    int minutes = diff / 60;
    int seconds = diff % 60;
    
    char timeBuffer[20];
    char daysBuffer[10];
    snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d:%02d", hours, minutes, seconds);
    snprintf(daysBuffer, sizeof(daysBuffer), "%02d", days);
    
    return {String(daysBuffer), String(timeBuffer)};
}

// Function to sync time with NTP
void syncTime() {
    if (WiFi.status() == WL_CONNECTED) {
        timeClient.update();
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    }
}

void setup()
{       
    LCD_Init();
    Lvgl_Init();
    ui_init();

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    // Initial time sync
    syncTime();

    // Update display with initial values
    auto [days, time] = getTimeDifference();
    lv_label_set_text(uic_days_label, days.c_str());
    lv_label_set_text(uic_time_label, time.c_str());
}

void loop()
{
    static unsigned long lastSync = 0;
    static unsigned long lastUpdate = 0;
    const unsigned long SYNC_INTERVAL = 24 * 60 * 60 * 1000; // 24 hours
    const unsigned long UPDATE_INTERVAL = 1000; // 1 second

    Timer_Loop();

    unsigned long currentMillis = millis();

    // Sync time once per day
    if (currentMillis - lastSync >= SYNC_INTERVAL) {
        syncTime();
        lastSync = currentMillis;
    }

    // Update display every second
    if (currentMillis - lastUpdate >= UPDATE_INTERVAL) {
        auto [days, time] = getTimeDifference();
        lv_label_set_text(uic_days_label, days.c_str());
        lv_label_set_text(uic_time_label, time.c_str());
        lastUpdate = currentMillis;
    }

    delay(5);
}
