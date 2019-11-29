// sdl_helper.c
#include "sdl_helper.h"

void sdl_set_pixel(SDL_Surface* ps, int x, int y, uint32_t c)
{
    if ((x < 0) || (x >= ps->w) || (y < 0) || (y >= ps->h))
        return;
    uint32_t* p = (uint32_t*)(((uint8_t*)ps->pixels) + y * ps->pitch + x * ps->format->BytesPerPixel);
    *p = c;
}

uint32_t sdl_get_pixel(SDL_Surface* ps, int x, int y)
{
    if ((x < 0) || (x >= ps->w) || (y < 0) || (y >= ps->h))
        return 0;
    uint32_t* p = (uint32_t*)(((uint8_t*)ps->pixels) + y * ps->pitch + x * ps->format->BytesPerPixel);
    return *p;
}

void sdl_draw_line(SDL_Surface* ps, int x0, int y0, int x1, int y1, uint32_t c)
{
    int n;
    int x = x0;
    int y = y0;
    int dx = x1 - x0;
    int dy = y1 - y0;
    int sx = (dx >= 0) ? 1 : -1;
    int sy = (dy >= 0) ? 1 : -1;
    int cx = (sx > 0) ? dx : -dx;
    int cy = (sy > 0) ? dy : -dy;
    dx = cx;
    dy = cy;
    if (dx > dy)
        for (n = dx; n > 0; n--) {
            sdl_set_pixel(ps, x, y, c);
            if ((cx -= cy) <= 0) {
                y += sy;
                cx += dx;
            }
            x += sx;
        }
    else if (dx < dy)
        for (n = dy; n > 0; n--) {
            sdl_set_pixel(ps, x, y, c);
            if ((cy -= cx) <= 0) {
                x += sx;
                cy += dy;
            }
            y += sy;
        }
    else //dx == dy
        for (n = dx; n > 0; n--) {
            sdl_set_pixel(ps, x, y, c);
            x += sx;
            y += sy;
        }
}

void sdl_draw_rect(SDL_Surface* ps, int x, int y, int w, int h, uint32_t c)
{
    sdl_draw_line(ps, x, y, x + w - 1, y, c);
    sdl_draw_line(ps, x + w - 1, y, x + w - 1, y + h - 1, c);
    sdl_draw_line(ps, x + w - 1, y + h - 1, x, y + h - 1, c);
    sdl_draw_line(ps, x, y + h - 1, x, y, c);
}

void sdl_fill_rect(SDL_Surface* ps, int x, int y, int w, int h, uint32_t c)
{
    int i;
    int j;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            sdl_set_pixel(ps, x + i, y + j, c);
}

void sdl_draw_char8(SDL_Surface* ps, sdl_font_t* pf, int x, int y, char chr, uint32_t c)
{
    int i;
    int j;
    uint8_t* pch;
    uint8_t crd;
    uint8_t bpc = (pf->bpr * pf->h) >> 3;
    if ((chr < 32) || (chr > 127))
        return;
    pch = ((uint8_t*)pf->pcs) + ((chr - 32) * bpc);
    for (j = 0; j < pf->h; j++) {
        crd = pch[j];
        for (i = 0; i < pf->w; i++) {
            if (crd & 0x80)
                sdl_set_pixel(ps, x + i, y + j, c);
            crd <<= 1;
        }
    }
}

void sdl_draw_char16(SDL_Surface* ps, sdl_font_t* pf, int x, int y, char chr, uint32_t c)
{
    int i;
    int j;
    uint16_t* pch;
    uint16_t crd;
    uint8_t bpc = (pf->bpr * pf->h) >> 3;
    if ((chr < 32) || (chr > 127))
        return;
    pch = ((uint16_t*)pf->pcs) + ((chr - 32) * (bpc >> 1));
    for (j = 0; j < pf->h; j++) {
        crd = pch[j];
        for (i = 0; i < pf->w; i++) {
            if (crd & 0x8000)
                sdl_set_pixel(ps, x + i, y + j, c);
            crd <<= 1;
        }
    }
}

void sdl_draw_char(SDL_Surface* ps, sdl_font_t* pf, int x, int y, char chr, uint32_t c)
{
    switch (pf->bpr) {
    case 8:
        sdl_draw_char8(ps, pf, x, y, chr, c);
        break;
    case 16:
        sdl_draw_char16(ps, pf, x, y, chr, c);
        break;
    }
}

void sdl_draw_text(SDL_Surface* ps, sdl_font_t* pf, int x, int y, char* str, uint32_t c)
{
    int i;
    int len = strlen(str);
    for (i = 0; i < len; i++)
        sdl_draw_char(ps, pf, x + i * pf->w, y, str[i], c);
}
