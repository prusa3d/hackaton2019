// sdl_helper.h

#ifndef _SDL_HELPER_H
#define _SDL_HELPER_H

#include <inttypes.h>
#include "SDL/SDL.h"
#include "sdl_font.h"


extern void sdl_set_pixel(SDL_Surface* ps, int x, int y, uint32_t c);
extern uint32_t sdl_get_pixel(SDL_Surface* ps, int x, int y);
extern void sdl_draw_line(SDL_Surface* ps, int x0, int y0, int x1, int y1, uint32_t c);
extern void sdl_draw_rect(SDL_Surface* ps, int x, int y, int w, int h, uint32_t c);
extern void sdl_fill_rect(SDL_Surface* ps, int x, int y, int w, int h, uint32_t c);
extern void sdl_draw_char8(SDL_Surface* ps, sdl_font_t* pf, int x, int y, char chr, uint32_t c);
extern void sdl_draw_char16(SDL_Surface* ps, sdl_font_t* pf, int x, int y, char chr, uint32_t c);
extern void sdl_draw_char(SDL_Surface* ps, sdl_font_t* pf, int x, int y, char chr, uint32_t c);
extern void sdl_draw_text(SDL_Surface* ps, sdl_font_t* pf, int x, int y, char* str, uint32_t c);

extern sdl_font_t sdl_font_8x12;
extern sdl_font_t sdl_font_12x12;


#endif // _SDL_HELPER_H
