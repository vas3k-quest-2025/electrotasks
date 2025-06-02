#include "Display_ST7789.h"
#include "LVGL_Driver.h"
#include "ui.h"

const static int SENSOR_IN = 2;
const static String code = "ПЛАЗМА";
const static int SENSOR_THRESHOLD_LOW = 180; // Порог для предупреждения
const static int SENSOR_THRESHOLD_HIGH = 500;  // Порог для показа кода

// Константы для таймера
const static int MESSAGE_DISPLAY_TIME = 5000; // Время показа кодового слова в миллисекундах

// Константы для подсветки
const static int BACKLIGHT_LOW = 50;
const static int BACKLIGHT_HIGH = 80;

// Переменные состояния
enum State {
    WAITING,    // Ожидание
    WARNING,    // Предупреждение
    SHOWING_CODE // Показ кода
};

State currentState = WAITING;
unsigned long codeDisplayStartTime = 0;

void setup()
{       
    Serial.begin(115200);
    LCD_Init();
    Lvgl_Init();
    ui_init();

    pinMode(SENSOR_IN, INPUT);
    
    lv_obj_add_flag(uic_arc, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(uic_message, "Хочется поближе\nк свету и теплу");
    Set_Backlight(BACKLIGHT_LOW);
    lv_obj_set_style_text_color(uic_message, lv_color_make(255, 255, 255), 0);
}

void loop()
{
    Timer_Loop();
    
    int sensorValue = analogRead(SENSOR_IN);
    
    // Выводим значение датчика в консоль
    Serial.print("Sensor value: ");
    Serial.println(sensorValue);
    
    switch (currentState) {
        case WAITING:
            if (sensorValue < SENSOR_THRESHOLD_LOW) {
                currentState = WARNING;
                codeDisplayStartTime = millis();
                lv_label_set_text(uic_message, "Эй, не так близко!");
                Set_Backlight(BACKLIGHT_HIGH);
            }
            break;
            
        case WARNING:
            if (sensorValue >= SENSOR_THRESHOLD_HIGH && millis() - codeDisplayStartTime >= 500) {
                currentState = SHOWING_CODE;
                codeDisplayStartTime = millis();
                String message = "кодовое слово:\n" + code;
                lv_label_set_text(uic_message, message.c_str());
                lv_obj_set_style_text_color(uic_message, lv_color_make(255, 0, 0), 0);
            }
            break;
            
        case SHOWING_CODE:
            if (millis() - codeDisplayStartTime >= MESSAGE_DISPLAY_TIME) {
                currentState = WAITING;
                lv_label_set_text(uic_message, "Хочется поближе\nк свету и теплу");
                lv_obj_set_style_text_color(uic_message, lv_color_make(255, 255, 255), 0);
                Set_Backlight(BACKLIGHT_LOW);
            }
            break;
    }

    delay(5);
}
