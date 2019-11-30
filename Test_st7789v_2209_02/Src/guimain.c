//guimain.c

#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "gui.h"
#include "config.h"

#include "gpio.h"
#include "st7789v.h"
#include "jogwheel.h"
#include "hwio_a3ides.h"
#include "diag.h"
#include "sys.h"

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
    gpio_init(PE12, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_LOW);
    font_t* font = resource_font(IDR_FNT_NORMAL);

    display->fill_rect(rect_ui16(0, 0, 240, 320), COLOR_BLACK);
    render_icon_align(rect_ui16(70, 20, 100, 100), IDR_PNG_icon_pepa, COLOR_BLACK, RENDER_FLG(ALIGN_CENTER, 0));
//    display->draw_text(rect_ui16(10, 115, 240, 60), "Hi, this is your Original Prusa\nMINI.", font, COLOR_BLACK, COLOR_WHITE);
//    display->draw_text(rect_ui16(10, 160, 240, 80), "Please insert the USB drive\nthat came with your MINI and\nreset the printer to flash the\nfirmware", font, COLOR_BLACK, COLOR_WHITE);
    render_text_align(rect_ui16(5, 115, 230, 60), "Hi, this is your Original Prusa MINI.", font, COLOR_ORANGE, COLOR_WHITE, padding_ui8(2, 6, 2, 2), ALIGN_LEFT | RENDER_FLG_WORDB);
    render_text_align(rect_ui16(5, 160, 230, 80), "Please insert the USB drive that came with your MINI and reset the printer to flash the firmware", font, COLOR_ORANGE, COLOR_WHITE, padding_ui8(2, 6, 2, 2), ALIGN_LEFT | RENDER_FLG_WORDB);
    render_text_align(rect_ui16(5, 250, 230, 40), "RESET PRINTER", font, COLOR_ORANGE, COLOR_WHITE, padding_ui8(2, 6, 2, 2), ALIGN_CENTER);
    while (1) {
        if (!gpio_get(PE12)) {
            sys_reset();
        }
    }
}
