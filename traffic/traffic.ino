#include "Display_ST7789.h"
#include "LVGL_Driver.h"
#include "ui.h"
#include <Wire.h>
#include "Adafruit_TCS34725.h"

const static String code = "ПОГНАЛИ";

const static int MESSAGE_DISPLAY_TIME = 5000; // Время показа кодового слова в миллисекундах

// Пины
const static int LED_CONTROL = 5;
const static int COLOR_SCL = 2;
const static int COLOR_SDA = 3;
const static int TOUCH = 4;
const static int COUNTDOWN_SECONDS = 5;
const static int LED_COUNT = 1;

// Цветовые пороги для датчика
const static int RED_THRESHOLD = 800;
const static int YELLOW_THRESHOLD = 1000;
const static int GREEN_THRESHOLD = 900;

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_600MS, TCS34725_GAIN_1X);

// Переменные состояния
enum State {
    IDLE,
    WAITING_RED,
    WAITING_YELLOW,
    WAITING_GREEN,
    SHOWING_CODE
};

State currentState = IDLE;
unsigned long lastUpdateTime = 0;
int remainingSeconds = COUNTDOWN_SECONDS;

void setup()
{       
    Serial.begin(115200);
    Serial.println("Инициализация...");
    
    LCD_Init();
    Lvgl_Init();
    ui_init();

    pinMode(LED_CONTROL, OUTPUT);
    pinMode(TOUCH, INPUT);
    digitalWrite(LED_CONTROL, LOW);
    
    Wire.begin(COLOR_SDA, COLOR_SCL);
    if (!tcs.begin()) {
        Serial.println("Ошибка инициализации датчика цвета!");
    } else {
        Serial.println("Датчик цвета инициализирован успешно");
    }

    lv_obj_add_flag(uic_arc, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(uic_message, "Ну что там,\nгде светофор?");
    Set_Backlight(40);
    lv_obj_set_style_text_color(uic_message, lv_color_make(255, 255, 255), 0);
    
    Serial.println("Инициализация завершена");
}

void resetToIdle() {
    currentState = IDLE;
    digitalWrite(LED_CONTROL, LOW);
    Set_Backlight(40);
    lv_obj_add_flag(uic_arc, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(uic_message, "Ну что там,\nгде светофор?");
    lv_obj_set_style_text_color(uic_message, lv_color_make(255, 255, 255), 0);
}

void startCountdown(const char* message, lv_color_t color) {
    remainingSeconds = COUNTDOWN_SECONDS;
    lastUpdateTime = millis();
    lv_label_set_text(uic_message, message);
    lv_obj_set_style_text_color(uic_message, color, 0);
    lv_obj_clear_flag(uic_arc, LV_OBJ_FLAG_HIDDEN);
    lv_arc_set_value(uic_arc, 100);
}

bool isRed(u_int16_t r, uint16_t g, uint16_t b) {
    if (r < b * 1.5 || r < g * 1.5) {
        return false;
    }

    return r > RED_THRESHOLD;
}

bool isYellow(uint16_t r, uint16_t g, uint16_t b) {
    if (r < b * 1.5 || g < b * 1.5) {
        return false;
    }

    if (1.5 * r < g || 1.5 * g < r) {
        return false;
    }

    return r > YELLOW_THRESHOLD && g > YELLOW_THRESHOLD;
}

bool isGreen(uint16_t r, uint16_t g, uint16_t b) {
    if (g < r * 1.5 || g < b * 1.3) {
        return false;
    }
    return g > GREEN_THRESHOLD;
}


void loop()
{
    Timer_Loop();
    
    if (digitalRead(TOUCH) == HIGH && currentState == IDLE) {
        Serial.println("Обнаружено касание");
        currentState = WAITING_RED;
        digitalWrite(LED_CONTROL, HIGH);
        Set_Backlight(80);
        startCountdown("Красный", lv_color_make(255, 0, 0));
    }

    if (currentState != IDLE && currentState != SHOWING_CODE) {
        uint16_t r, g, b, c;
        tcs.getRawData(&r, &g, &b, &c);
        Serial.printf("Цвет: R=%d, G=%d, B=%d\n", r, g, b);
        
        unsigned long currentTime = millis();
        if (currentTime - lastUpdateTime >= 1000) {
            remainingSeconds--;
            lastUpdateTime = currentTime;
            lv_arc_set_value(uic_arc, (remainingSeconds * 100) / COUNTDOWN_SECONDS);
            Serial.printf("Осталось секунд: %d\n", remainingSeconds);
            
            if (remainingSeconds <= 0) {
                Serial.println("Время истекло, возврат в исходное состояние");
                resetToIdle();
            }
        }

        switch (currentState) {
            case WAITING_RED:
                if (isRed(r, g, b)) {
                    Serial.println("Обнаружен красный цвет");
                    currentState = WAITING_YELLOW;
                    startCountdown("Желтый", lv_color_make(255, 255, 0));
                }
                break;
                
            case WAITING_YELLOW:
                if (isYellow(r, g, b)) {
                    Serial.println("Обнаружен желтый цвет");
                    currentState = WAITING_GREEN;
                    startCountdown("Зеленый", lv_color_make(0, 255, 0));
                }
                break;
                
            case WAITING_GREEN:
                if (isGreen(r, g, b)) {
                    Serial.println("Обнаружен зеленый цвет");
                    currentState = SHOWING_CODE;
                    lv_obj_add_flag(uic_arc, LV_OBJ_FLAG_HIDDEN);
                    lv_label_set_text(uic_message, ("Кодовое слово:\n" + code).c_str());
                    lv_obj_set_style_text_color(uic_message, lv_color_make(255, 0, 0), 0);
                    lastUpdateTime = currentTime;
                }
                break;
        }
    } else if (currentState == SHOWING_CODE) {
        if (millis() - lastUpdateTime >= MESSAGE_DISPLAY_TIME) {
            Serial.println("Завершение показа кодового слова");
            resetToIdle();
        }
    }

    delay(5);
}
