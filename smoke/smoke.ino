#include "Display_ST7789.h"
#include "LVGL_Driver.h"
#include "ui.h"

const static int SENSOR_IN = 2;
const static String code = "ПОТОЛОК";
const static int SENSOR_THRESHOLD_HIGH = 320; // Порог для перехода в SHOWING_CODE
const static int SENSOR_THRESHOLD_LOW = 310;  // Порог для перехода в WAITING

// Константы для таймера
const static int MESSAGE_DISPLAY_TIME = 5000; // Время показа кодового слова в миллисекундах

// Переменные состояния
enum State {
    WARMING,    // Состояние разогрева
    WAITING,    // Ожидание
    SHOWING_CODE // Показ кода
};

State currentState = WARMING;
unsigned long codeDisplayStartTime = 0;
bool lastSensorState = true;

void setup()
{       
    Serial.begin(115200);
    LCD_Init();
    Lvgl_Init();
    ui_init();

    pinMode(SENSOR_IN, INPUT);
    
    lv_obj_add_flag(uic_arc, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(uic_message, "Погоди,\nне спеши");
    Set_Backlight(30);
    lv_obj_set_style_text_color(uic_message, lv_color_make(255, 255, 255), 0);
}

void loop()
{
    Timer_Loop();
    
    int sensorValue = analogRead(SENSOR_IN);
    bool currentSensorState = sensorValue > SENSOR_THRESHOLD_HIGH;
    
    // Выводим значение датчика в консоль
    Serial.print("Sensor value: ");
    Serial.println(sensorValue);
    
    switch (currentState) {
        case WARMING:
            if (sensorValue < SENSOR_THRESHOLD_LOW) {
                currentState = WAITING;
                lv_label_set_text(uic_message, "Эх, щас бы\nсижку...");
            }
            break;
            
        case WAITING:
            if (sensorValue > SENSOR_THRESHOLD_HIGH) {
                currentState = SHOWING_CODE;
                codeDisplayStartTime = millis();
                String message = "Код задания:\n" + code;
                lv_label_set_text(uic_message, message.c_str());
                lv_obj_set_style_text_color(uic_message, lv_color_make(255, 0, 0), 0);
                Set_Backlight(80);
            }
            break;
            
        case SHOWING_CODE:
            if (millis() - codeDisplayStartTime >= MESSAGE_DISPLAY_TIME) {
                if (sensorValue > SENSOR_THRESHOLD_HIGH) {
                    currentState = WARMING;
                    lv_label_set_text(uic_message, "Погоди,\nне спеши");
                } else if (sensorValue < SENSOR_THRESHOLD_LOW) {
                    currentState = WAITING;
                    lv_label_set_text(uic_message, "Эх, щас бы\nсижку...");
                }
                lv_obj_set_style_text_color(uic_message, lv_color_make(255, 255, 255), 0);
                Set_Backlight(30);
            }
            break;
    }

    delay(5);
}
