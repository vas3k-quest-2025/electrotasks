#include <lvgl.h>
#include "Arduino_GFX_Library.h"
#include "lv_conf.h"
#include "demos/lv_demos.h"
#include "pin_config.h"
#include "ui.h"

#define EXAMPLE_LVGL_TICK_PERIOD_MS 2

static const int SYS_EN = 35;
static const int NOISE_PIN = 2;
static const int NOISE_TRESHOLD = 3500;
static const unsigned long NOISE_DISPLAY_DURATION = 5000; // 5 секунд в миллисекундах
static const String CODE_WORD = "АПГРЕЙД";
static const int BATTERY_PIN = 34; // Пин для считывания напряжения батареи

static const uint16_t screenWidth = 240;
static const uint16_t screenHeight = 280;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 10];

const int voltageDividerPin = 1;
float vRef = 3.3;
float R1 = 200000.0;
float R2 = 100000.0;

Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI);

Arduino_GFX *gfx = new Arduino_ST7789(bus, LCD_RST /* RST */,
                                      0 /* rotation */, true /* IPS */, LCD_WIDTH, LCD_HEIGHT, 0, 20, 0, 0);

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

  lv_disp_flush_ready(disp);
}

void example_increase_lvgl_tick(void *arg) {
  /* Tell LVGL how many milliseconds has elapsed */
  lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

static uint8_t count = 0;
static unsigned long lastNoiseTime = 0;
static int maxNoiseLevel = 0;

void example_increase_reboot(void *arg) {
  count++;
  if (count == 30) {
    esp_restart();
  }
}

void setup() {
  pinMode(SYS_EN, OUTPUT);
  digitalWrite(SYS_EN, HIGH);
  pinMode(voltageDividerPin, INPUT);
  
  lastNoiseTime = millis() - NOISE_DISPLAY_DURATION;

  // Дальше копипаст от разработчиков — инициализация экрана
  lv_init();
  gfx->begin();
  gfx->Display_Brightness(127);
  pinMode(15, OUTPUT);
  digitalWrite(15, LOW);
  // todo — было бы круто чуть снижать яркость экрана в режиме простоя. Пока не знаю, что не так с этой установкой
  // ledcAttach(LCD_BL, 5000, 12); 
  // ledcWrite(LCD_BL, 2048);
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * screenHeight / 10);

  /* Initialize the display */
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  /* Change the following line to your display resolution */
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  const esp_timer_create_args_t lvgl_tick_timer_args = {
    .callback = &example_increase_lvgl_tick,
    .name = "lvgl_tick"
  };

  const esp_timer_create_args_t reboot_timer_args = {
    .callback = &example_increase_reboot,
    .name = "reboot"
  };

  esp_timer_handle_t lvgl_tick_timer = NULL;
  esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
  esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000);

  // инициализация нашего интерфейса из SquareLine
  ui_init();
}

void loop() {
  // не погружался в нюансы lvgl, но нужно
  lv_timer_handler();

  // Напряжение на батарее
  int adcValue = analogRead(voltageDividerPin);
  float voltage = (float)adcValue * (vRef / 4095.0);
  float actualVoltage = voltage * ((R1 + R2) / R2);
  String voltageStr = String(actualVoltage) + " v";
  lv_label_set_text(uic_label_battery, voltageStr.c_str());

  // непосредственно код задания
  static String feeling;
  static String code;
  const int noiseValue = analogRead(NOISE_PIN);
  unsigned long currentTime = millis();

  if (noiseValue > NOISE_TRESHOLD || currentTime - lastNoiseTime < NOISE_DISPLAY_DURATION) {
    if (noiseValue > maxNoiseLevel) {
      maxNoiseLevel = noiseValue;
    }
    feeling = "Блин! Зачем\nтак орать?!";
    code = "Код:\n" + CODE_WORD + "\n" + String(maxNoiseLevel);
    if (noiseValue > NOISE_TRESHOLD) {
      lastNoiseTime = currentTime;
    }
  } else if (noiseValue > NOISE_TRESHOLD / 2) {
    feeling = "Ещё чуть-чуть...";
    code = "";
    maxNoiseLevel = 0;
  } else {
    feeling = "Приготовься,\nсейчас будет\nочень больно";
    code = "";
    maxNoiseLevel = 0;
  }

  lv_label_set_text(uic_label_feel, feeling.c_str());
  lv_label_set_text(uic_label_code, code.c_str());

  // по доке lvgl, надо ставить задержку 5мс
  delay(5);
}
