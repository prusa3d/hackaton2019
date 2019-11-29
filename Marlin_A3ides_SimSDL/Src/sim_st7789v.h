#ifndef _SIM_ST7789V_H
#define _SIM_ST7789V_H

#include <inttypes.h>

#define ST7789_COLS            240
#define ST7789_ROWS            320

#define ST7789_CTL_FMT_444     0x03
#define ST7789_CTL_FMT_565     0x05
#define ST7789_CTL_FMT_666     0x06
#define ST7789_CTL_FMT_888     0x07
#define ST7789_CTL_FMT_MSK     0x07

#define ST7789_RGB_FMT_65k     0x50
#define ST7789_RGB_FMT_262k    0x60


extern uint32_t sim_st7789v_pixels[ST7789_COLS * ST7789_ROWS]; //pixel array (32bit)

extern uint32_t sim_st7789v_pixels_changed; //number of pixel changed

extern uint16_t  sim_st7789v_x; //current col address (x-pos)

extern uint16_t  sim_st7789v_y; //current row address (y-pos)

extern uint16_t  sim_st7789v_cx; //current col address (x-pos)

extern uint16_t  sim_st7789v_cy; //current row address (y-pos)


extern void sim_st7789v_wr_cmd(uint16_t pix);

extern void sim_st7789v_wr_565(uint16_t pix);


#endif //_SIM_ST7789V_H
