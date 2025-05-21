#include "Display_ST7789.h"
#include "LVGL_Driver.h"

#include "ui.h"

void setup()
{       
  LCD_Init();
  Lvgl_Init();
  ui_init();

  int days = 15;

  lv_arc_set_value(uic_days_arc, days);
  lv_label_set_text(uic_days_label, String(days).c_str());
}

void loop()
{
  Timer_Loop();
  delay(5);
}
