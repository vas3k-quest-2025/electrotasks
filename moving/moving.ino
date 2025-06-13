#include "Display_ST7789.h"
#include "LVGL_Driver.h"
#include "ui.h"
#include <FastLED.h>

const static int LED_CTRL = 8;
const static int LED_COUNT = 8;
const static int BRIGHTNESS = 80;

const static int PIR_IN = 1;
const static String code = "ФИГУРКА";

// Константы для таймера
const static int COUNTDOWN_SECONDS = 60; // Количество секунд для обратного отсчета
const static int MESSAGE_DISPLAY_TIME = 5000; // Время показа кодового слова в миллисекундах

// Переменные состояния
enum State {
    IDLE,               // Начальное состояние
    WAITING_FOR_MOTION, // Ожидание движения
    COUNTING_DOWN,      // Обратный отсчет
    SHOWING_CODE        // Показ кодового слова
};

State currentState = IDLE;
unsigned long lastUpdateTime = 0;
int remainingSeconds = COUNTDOWN_SECONDS;

CRGB leds[LED_COUNT];

void setup()
{       
    LCD_Init();
    Lvgl_Init();
    ui_init();

    pinMode(PIR_IN, INPUT);
    FastLED.addLeds<WS2812B, LED_CTRL, RGB>(leds, LED_COUNT);
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.clear();
    FastLED.show();

    lv_obj_add_flag(uic_arc, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(uic_message, "Ждём людей...");
    Set_Backlight(40);
    lv_obj_set_style_text_color(uic_message, lv_color_make(255, 255, 255), 0);
}

void loop()
{
    Timer_Loop();
    unsigned long currentTime = millis();
    bool motionDetected = (digitalRead(PIR_IN) == HIGH);

    // Обновляем состояние светодиодов
    if (motionDetected) {
        fill_solid(leds, LED_COUNT, CRGB::Red);
    } else {
        FastLED.clear();
    }
    FastLED.show();

    // Обработка состояний
    switch (currentState) {
        case IDLE:
            if (motionDetected) {
                currentState = WAITING_FOR_MOTION;
                lv_obj_set_style_text_color(uic_message, lv_color_make(255, 255, 255), 0); // Белый цвет
                lv_label_set_text(uic_message, "Стой, не шевелись");
                Set_Backlight(80);
            }
            break;

        case WAITING_FOR_MOTION:
            if (!motionDetected) {
                currentState = COUNTING_DOWN;
                remainingSeconds = COUNTDOWN_SECONDS;
                lastUpdateTime = currentTime;
            } else {
                break;
            }
            

        case COUNTING_DOWN:
            if (motionDetected) {
                currentState = WAITING_FOR_MOTION;
                lv_obj_add_flag(uic_arc, LV_OBJ_FLAG_HIDDEN);
                lv_label_set_text(uic_message, "Стой, не шевелись");
                lv_obj_set_style_text_color(uic_message, lv_color_make(255, 255, 255), 0); // Белый цвет
            } else {
                // Обновляем прогресс арки и текст каждую секунду
                if (currentTime - lastUpdateTime >= 1000) {
                    remainingSeconds--;
                    if (remainingSeconds < 0) {
                        // Завершаем отсчет
                        currentState = SHOWING_CODE;
                        lv_obj_add_flag(uic_arc, LV_OBJ_FLAG_HIDDEN);
                        char buf[64];
                        snprintf(buf, sizeof(buf), "Кодовое слово:\n%s", code.c_str());
                        lv_label_set_text(uic_message, buf);
                        lv_obj_set_style_text_color(uic_message, lv_color_make(255, 0, 0), 0); // Красный цвет
                        lastUpdateTime = currentTime;
                    } else {
                        // Обновляем прогресс
                        lv_obj_clear_flag(uic_arc, LV_OBJ_FLAG_HIDDEN);
                        lv_arc_set_value(uic_arc, (remainingSeconds * 100) / COUNTDOWN_SECONDS);
                        char buf[32];
                        snprintf(buf, sizeof(buf), "%d", remainingSeconds);
                        lv_label_set_text(uic_message, buf);
                        lastUpdateTime = currentTime;
                    }
                }
            }
            break;

        case SHOWING_CODE:
            if (motionDetected) {
                // Если движение возобновилось, возвращаемся к началу
                currentState = WAITING_FOR_MOTION;
                lv_label_set_text(uic_message, "Стой, не шевелись");
                lv_obj_set_style_text_color(uic_message, lv_color_make(255, 255, 255), 0); // Белый цвет
            } else if (currentTime - lastUpdateTime >= MESSAGE_DISPLAY_TIME) {
                lv_label_set_text(uic_message, "Ждём людей...");
                lv_obj_set_style_text_color(uic_message, lv_color_make(255, 255, 255), 0);
                currentState = IDLE;
                Set_Backlight(40);
            }
            break;
    }

    delay(5);
}
