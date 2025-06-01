#include "Display_ST7789.h"
#include "LVGL_Driver.h"
#include "ui.h"
#include <FastLED.h>

// Константы
#define PULSE_POWER_PIN 0
#define PULSE_DATA_PIN 1
#define TOUCH_PIN 2
#define LED_CTRL 8
#define LED_COUNT 1
#define LED_BRIGHTNESS 50
#define TARGET_BPM 90             // 90 ударов в минуту — целевой пульс
#define MEASUREMENT_TIMEOUT 60000 // 60 секунд измеряем
#define DISPLAY_TIMEOUT 5000      // 5 секунд показываем код/сообщение
#define COUNTDOWN_SEC 3           // 3 секунды обратный отсчёт
#define PULSE_MIN_INTERVAL 200    // Минимальный интервал между ударами (мс)
#define MIN_RELIABLE_BEATS 20     // Минимум ударов для надёжного подсчёта
#define CODE_WORD "ПОМПА"         // Кодовое слово
#define INITIAL_BEATS 3           // Количество ударов до начала отображения пульса

// Параметры измерения пульса
#define SAMPLE_RATE 100           // Частота измерений (измерений в секунду)
#define SAMPLE_WINDOW 10          // Окно для определения подъёма (измерений)
#define PULSE_MIN_DEVIATION 11    // Минимальное отклонение для определения удара
#define PULSE_MAX_DEVIATION 100   // Максимальное допустимое отклонение
#define PULSE_ERROR_THRESHOLD 3   // Количество ошибок подряд для перехода в состояние ошибки

// Расчет размера массива измерений
#define SAMPLE_ARRAY_SIZE (SAMPLE_RATE * MEASUREMENT_TIMEOUT / 1000)

// Яркость экрана
#define BRIGHTNESS_WAITING 60     // Яркость в режиме ожидания
#define BRIGHTNESS_ACTIVE 80      // Яркость в активном режиме

// Состояния программы
enum State {
    WAITING,        // Ожидание
    PREPARING,      // Подготовка
    MEASURING,      // Измерение
    SHOWING_CODE,   // Показ кода
    TRY_AGAIN,      // Попробуй еще раз
    MEASURE_ERROR   // Ошибка измерения
};

// Глобальные переменные
State currentState = WAITING;
unsigned long stateStartTime = 0;
int countdownValue = 3;
int currentBPM = 0;
bool lastTouchState = false;
bool isHeartBeating = false;

// Переменные для измерения пульса
unsigned long lastBeatTime = 0;
unsigned long measurementStartTime = 0;
int totalBeats = 0;
int errorCount = 0;

// Массив измерений
int samples[SAMPLE_ARRAY_SIZE];
int sampleIndex = 0;
int samplesCount = 0;  // Количество реально записанных измерений
bool isRising = false;
int riseStartIndex = 0;

// Переменные для светодиода
CRGB leds[LED_COUNT];

void setup() {
    // Инициализация пинов
    pinMode(PULSE_POWER_PIN, OUTPUT);
    pinMode(PULSE_DATA_PIN, INPUT);
    pinMode(TOUCH_PIN, INPUT);
    
    // Инициализация светодиода
    FastLED.addLeds<WS2812B, LED_CTRL, RGB>(leds, LED_COUNT);
    FastLED.setBrightness(LED_BRIGHTNESS);
    FastLED.clear();
    FastLED.show();
    
    // Инициализация Serial для отладки
    Serial.begin(115200);
    Serial.println("current,average,bpm");
    
    // Инициализация дисплея
    LCD_Init();
    Lvgl_Init();
    ui_init();
    
    // Начальное состояние
    showWaitingState();
}

void loop() {
    // Обработка состояний
    switch (currentState) {
        case WAITING:
            handleWaitingState();
            break;
        case PREPARING:
            handlePreparingState();
            break;
        case MEASURING:
            handleMeasuringState();
            break;
        case SHOWING_CODE:
            handleShowingCodeState();
            break;
        case TRY_AGAIN:
            handleTryAgainState();
            break;
        case MEASURE_ERROR:
            handleMeasureErrorState();
            break;
    }
    
    // Обновление LVGL
    lv_task_handler();
    delay(5);
}

// Функции состояний
void showWaitingState() {
    lv_obj_add_flag(uic_bpm_arc, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(uic_bpm_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(uic_pulse_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(uic_codeword, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(uic_codeword, "Измерим пульс?");
    lv_obj_set_style_text_color(uic_codeword, lv_color_white(), 0);
    
    // Устанавливаем яркость для режима ожидания
    Set_Backlight(BRIGHTNESS_WAITING);
}

void handleWaitingState() {
    bool touchState = digitalRead(TOUCH_PIN);
    if (touchState && !lastTouchState) {
        currentState = PREPARING;
        stateStartTime = millis();
        countdownValue = COUNTDOWN_SEC;
        showPreparingState();
    }
    lastTouchState = touchState;
}

void showPreparingState() {
    lv_obj_clear_flag(uic_bpm_arc, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(uic_bpm_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(uic_pulse_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(uic_codeword, LV_OBJ_FLAG_HIDDEN);
    
    // Устанавливаем яркость для активного режима
    Set_Backlight(BRIGHTNESS_ACTIVE);

    lv_label_set_text(uic_pulse_label, "  Приложи\n  палец");
    lv_label_set_text(uic_bpm_label, String(COUNTDOWN_SEC).c_str());
    lv_arc_set_value(uic_bpm_arc, 100);

    digitalWrite(PULSE_POWER_PIN, HIGH);
}

void handlePreparingState() {
    unsigned long currentTime = millis();
    if (currentTime - stateStartTime >= 1000) {
        stateStartTime = currentTime;
        countdownValue--;
        lv_label_set_text(uic_bpm_label, String(countdownValue).c_str());
        int arcValue = map(countdownValue, 0, COUNTDOWN_SEC, 0, 100);
        lv_arc_set_value(uic_bpm_arc, arcValue);
        
        if (countdownValue == 0) {
            currentState = MEASURING;
            stateStartTime = currentTime;
            showMeasuringState();
        }
    }
}

void showMeasuringState() {
    lv_obj_clear_flag(uic_bpm_arc, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(uic_bpm_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(uic_pulse_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(uic_codeword, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(uic_bpm_label, "--");
    lv_label_set_text(uic_pulse_label, "Пульс\n\nуд/мин");
    
    // Сброс переменных измерения
    totalBeats = 0;
    measurementStartTime = millis();
    lastBeatTime = 0;
    currentBPM = 0;
    errorCount = 0;
    sampleIndex = 0;
    samplesCount = 0;
    isRising = false;
    riseStartIndex = 0;
    
    // Очистка массива измерений
    for (int i = 0; i < SAMPLE_ARRAY_SIZE; i++) {
        samples[i] = 0;
    }
    
    // Устанавливаем начальный цвет индикатора арки на синий
    lv_obj_set_style_arc_color(uic_bpm_arc, lv_color_make(31, 149, 246), LV_PART_INDICATOR);
    
    // Выключаем светодиод
    FastLED.clear();
    FastLED.show();
}

void handleMeasuringState() {
    static unsigned long lastSampleTime = 0;
    unsigned long currentTime = millis();
    
    // Проверяем, не превышено ли максимальное время измерения
    if (currentTime - measurementStartTime >= MEASUREMENT_TIMEOUT) {
        if (currentBPM < TARGET_BPM) {
            currentState = MEASURE_ERROR;
            stateStartTime = millis();
            showMeasureErrorState();
            return;
        }
    }
    
    // Проверяем, пора ли делать новое измерение
    if (currentTime - lastSampleTime < (1000 / SAMPLE_RATE)) {
        return;
    }
    lastSampleTime = currentTime;
    
    int pulseValue = analogRead(PULSE_DATA_PIN);
    samples[sampleIndex] = pulseValue;
    
    // Увеличиваем счетчик реальных измерений, если массив еще не заполнен
    if (samplesCount < SAMPLE_ARRAY_SIZE) {
        samplesCount++;
    }
    
    // Вычисляем среднее значение только по записанным измерениям
    long sum = 0;
    for (int i = 0; i < samplesCount; i++) {
        sum += samples[i];
    }
    int average = sum / samplesCount;
    
    // Проверка на ошибку измерения
    int deviation = abs(pulseValue - average);
    if (deviation > PULSE_MAX_DEVIATION) {
        errorCount++;
        if (errorCount >= PULSE_ERROR_THRESHOLD) {
            currentState = MEASURE_ERROR;
            stateStartTime = millis();
            showMeasureErrorState();
            return;
        }
    } else {
        errorCount = 0;
    }
    
    // Определение подъёма значения
    if (pulseValue > average + PULSE_MIN_DEVIATION) {
        if (!isRising) {
            isRising = true;
            riseStartIndex = sampleIndex;
            isHeartBeating = true;
            
            // Обновляем цвет индикатора арки на красный
            lv_obj_set_style_arc_color(uic_bpm_arc, lv_color_make(255, 0, 0), LV_PART_INDICATOR);
            
            // Включаем красный светодиод
            fill_solid(leds, LED_COUNT, CRGB::Red);
            FastLED.show();
        }
    } else {
        if (isRising) {
            // Проверяем, что подъём длился достаточно долго
            int riseDuration = (sampleIndex - riseStartIndex + SAMPLE_ARRAY_SIZE) % SAMPLE_ARRAY_SIZE;
            if (riseDuration >= SAMPLE_WINDOW) {
                // Определяем, что это новый удар
                if (currentTime - lastBeatTime >= PULSE_MIN_INTERVAL) {
                    totalBeats++;
                    lastBeatTime = currentTime;
                    
                    unsigned long measurementDuration = (currentTime - measurementStartTime) / 1000;
                    if (measurementDuration > 0) {
                        currentBPM = (totalBeats * 60) / measurementDuration;
                        
                        // Обновляем UI только после накопления INITIAL_BEATS ударов
                        if (totalBeats >= INITIAL_BEATS) {
                            lv_label_set_text(uic_bpm_label, String(currentBPM).c_str());
                            int arcValue = map(currentBPM, 0, 200, 0, 100);
                            lv_arc_set_value(uic_bpm_arc, arcValue);
                        }
                    }
                }
            }
            isRising = false;
            isHeartBeating = false;
            
            // Обновляем цвет индикатора арки на синий
            lv_obj_set_style_arc_color(uic_bpm_arc, lv_color_make(31, 149, 246), LV_PART_INDICATOR);
            
            // Выключаем светодиод
            FastLED.clear();
            FastLED.show();
        }
    }
    
    // Отладочный вывод для Serial Plotter
    Serial.print(pulseValue);
    Serial.print(",");
    Serial.print(average);
    Serial.print(",");
    Serial.println(currentBPM);
    
    // Обновляем индекс для следующего измерения
    sampleIndex = (sampleIndex + 1) % SAMPLE_ARRAY_SIZE;
    
    // Проверка условий перехода
    if (totalBeats >= MIN_RELIABLE_BEATS) {
        if (currentBPM >= TARGET_BPM) {
            currentState = SHOWING_CODE;
            stateStartTime = millis();
            showCodeState();
        } else {
            currentState = TRY_AGAIN;
            stateStartTime = millis();
            showTryAgainState();
        }
    }
}

void showCodeState() {
    digitalWrite(PULSE_POWER_PIN, LOW);
    lv_obj_add_flag(uic_bpm_arc, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(uic_bpm_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(uic_pulse_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(uic_codeword, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(uic_codeword, "Кодовое слово:\n" CODE_WORD);
    lv_obj_set_style_text_color(uic_codeword, lv_color_make(255, 0, 0), 0);

    lv_obj_set_style_arc_color(uic_bpm_arc, lv_color_make(31, 149, 246), LV_PART_INDICATOR);
    FastLED.clear();
    FastLED.show();
}

void handleShowingCodeState() {
    if (millis() - stateStartTime >= DISPLAY_TIMEOUT) {
        currentState = WAITING;
        showWaitingState();
    }
}

void showTryAgainState() {
    digitalWrite(PULSE_POWER_PIN, LOW);
    lv_obj_add_flag(uic_bpm_arc, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(uic_bpm_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(uic_pulse_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(uic_codeword, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(uic_codeword, "Слишком низкий\nпульс,\nпотренируйся ещё");
    lv_obj_set_style_text_color(uic_codeword, lv_color_white(), 0);

    lv_obj_set_style_arc_color(uic_bpm_arc, lv_color_make(31, 149, 246), LV_PART_INDICATOR);
    FastLED.clear();
    FastLED.show();
}

void handleTryAgainState() {
    if (millis() - stateStartTime >= DISPLAY_TIMEOUT) {
        currentState = WAITING;
        showWaitingState();
    }
}

void showMeasureErrorState() {
    digitalWrite(PULSE_POWER_PIN, LOW);
    lv_obj_add_flag(uic_bpm_arc, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(uic_bpm_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(uic_pulse_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(uic_codeword, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(uic_codeword, "Ошибка измерения.\nДержи палец\nнеподвижно");
    lv_obj_set_style_text_color(uic_codeword, lv_color_white(), 0);
    
    lv_obj_set_style_arc_color(uic_bpm_arc, lv_color_make(31, 149, 246), LV_PART_INDICATOR);
    FastLED.clear();
    FastLED.show();
}

void handleMeasureErrorState() {
    if (millis() - stateStartTime >= DISPLAY_TIMEOUT) {
        currentState = WAITING;
        showWaitingState();
    }
}

