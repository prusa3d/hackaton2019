// sim_st7789v.c

#include "sim_st7789v.h"

//control and rgb color format
uint8_t sim_st7789v_colmod = ST7789_CTL_FMT_565 | ST7789_RGB_FMT_65k;

uint32_t sim_st7789v_pixels[ST7789_COLS * ST7789_ROWS]; //pixel array (32bit)

uint32_t sim_st7789v_pixels_changed = 0; //number of pixel changed

uint32_t* sim_st7789v_ppix = 0; // current pixel pointer (32bit)

uint16_t sim_st7789v_x = 0; // current col address (x-pos)

uint16_t sim_st7789v_y = 0; // current row address (y-pos)

uint16_t sim_st7789v_cx = 0; // current window width (x-size)

uint16_t sim_st7789v_cy = 0; // current window height (y-size)

int sim_st7789_pin_cs = 0; // chip-select pin state (0/1)
int sim_st7789_pin_rs = 0; // register-select pin state (0/)

//increment address
void sim_st7789v_inc_addr(void)
{
    if (++sim_st7789v_x >= ST7789_COLS) {
        sim_st7789v_x = 0;
        if (++sim_st7789v_y >= ST7789_ROWS)
            sim_st7789v_y = 0;
    }
    sim_st7789v_ppix = sim_st7789v_pixels + (sim_st7789v_x + sim_st7789v_y * ST7789_COLS);
}

uint16_t sim_st7789v_rd_565(void)
{
    uint32_t pix = sim_st7789v_pixels[sim_st7789v_x + sim_st7789v_y * ST7789_COLS];
    uint16_t r = (pix & 0x00f80000) >> (16 + 3);
    uint16_t g = (pix & 0x0000fc00) >> (8 + 2);
    uint16_t b = (pix & 0x000000f8) >> 3;
    sim_st7789v_inc_addr();
    return (r << 11) | (g << 5) | b;
}

void sim_st7789v_wr_565(uint16_t pix)
{
    uint32_t* p = sim_st7789v_pixels + (sim_st7789v_x + sim_st7789v_y * ST7789_COLS);
    uint32_t r = (pix & 0xf800) >> (11 - 3);
    uint32_t g = (pix & 0x07e0) >> (5 - 2);
    uint32_t b = (pix & 0x001f) << 3;
    *p = (r << 16) | (g << 8) | b;
    sim_st7789v_pixels_changed++;
    sim_st7789v_inc_addr();
}

uint8_t sim_st7789v_spi_rw_cmd(uint8_t ch)
{
    return 0xff;
}

uint8_t sim_st7789v_spi_rw_pix(uint8_t ch)
{
    switch (sim_st7789v_colmod & ST7789_CTL_FMT_MSK) {
    case ST7789_CTL_FMT_444:
        return 0xff;
    case ST7789_CTL_FMT_565:
        return 0xff;
    case ST7789_CTL_FMT_666:
        return 0xff;
    case ST7789_CTL_FMT_888:
        return 0xff;
    }
    return 0xff;
}

uint8_t sim_st7789v_spi_rw(uint8_t ch)
{
    if (sim_st7789_pin_cs)
        return 0xff;
    if (sim_st7789_pin_rs)
        return sim_st7789v_spi_rw_cmd(ch);
    return sim_st7789v_spi_rw_pix(ch);
}
