SET(SOURCES ui_Screen1.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_img_noise1_png.c
    ui_img_noise2_png.c
    ui_img_noise3_png.c
    ui_img_clear_png.c)

add_library(ui ${SOURCES})
