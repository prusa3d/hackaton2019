// guimain.c

#include <stdio.h>
#include "gui.h"
#include "config.h"

#include "gpio.h"
#include "st7789v.h"
#include "jogwheel.h"

extern SPI_HandleTypeDef hspi2;

const st7789v_config_t st7789v_cfg = {
    &hspi2, // spi handle pointer
    ST7789V_PIN_CS, // CS pin
    ST7789V_PIN_RS, // RS pin
    ST7789V_PIN_RST, // RST pin
    ST7789V_FLG_DMA, // flags (DMA, MISO)
    ST7789V_DEF_COLMOD, // interface pixel format (5-6-5, hi-color)
    ST7789V_DEF_MADCTL, // memory data access control (no mirror XY)
};

const jogwheel_config_t jogwheel_cfg = {
    JOGWHEEL_PIN_EN1, // encoder phase1
    JOGWHEEL_PIN_EN2, // encoder phase2
    JOGWHEEL_PIN_ENC, // button
    JOGWHEEL_DEF_FLG, // flags
};

void gui_run(void)
{
    st7789v_config = st7789v_cfg;
    jogwheel_config = jogwheel_cfg;
    gui_init();
    gui_defaults.font = resource_font(IDR_FNT_NORMAL);
    gui_defaults.font_big = resource_font(IDR_FNT_BIG);
    window_msgbox_id_icon[0] = 0; //IDR_PNG_icon_pepa;
    window_msgbox_id_icon[1] = IDR_PNG_msgbox_icon_error;
    window_msgbox_id_icon[2] = IDR_PNG_msgbox_icon_question;
    window_msgbox_id_icon[3] = IDR_PNG_msgbox_icon_warning;
    window_msgbox_id_icon[4] = IDR_PNG_msgbox_icon_info;
    while (1) {
        gui_loop();
    }
}
