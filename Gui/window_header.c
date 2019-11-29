/*
 * window_header.c
 *
 *  Created on: 19. 7. 2019
 *      Author: mcbig
 */

#include <stdbool.h>
#include "window_header.h"
#include "config.h"
#include "marlin_client.h"

extern bool media_is_inserted();

void window_frame_draw(window_frame_t* window);

int16_t WINDOW_CLS_HEADER = 0;

void window_header_init(window_header_t* window)
{
    window->color_back = gui_defaults.color_back;
    window->color_text = gui_defaults.color_text;
    window->font = gui_defaults.font;
    window->padding = gui_defaults.padding;
    window->alignment = ALIGN_LEFT_CENTER;
    window->id_res = IDR_NULL;
    window->states = 0;
    window->label = NULL;

    if (media_is_inserted()) {
        p_window_header_enable_state(window, HEADER_USB);
    }

#if 0 // check if LAN is connected
	p_window_header_enable_state(window, HEADER_LAN);
#endif
#if 0 // check if WIFI is connected / enabled
	p_window_header_enable_state(window, HEADER_WIFI);
#endif
}

void window_header_done(window_header_t* window)
{
}

void window_header_draw(window_header_t* window)
{
    if (!((window->win.flg & (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE)) == (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE))) {
        return;
    }

    rect_ui16_t rc = {
        window->win.rect.x + 10, window->win.rect.y,
        window->win.rect.h, window->win.rect.h
    };

    if (window->id_res) { // first icon
        render_icon_align(rc, window->id_res,
            window->color_back, RENDER_FLG(ALIGN_CENTER, 0));
    } else {
        display->fill_rect(rc, window->color_back);
    }

    uint16_t icons_width = 10 + 36;
    rc = rect_ui16( // usb icon is showed always
        window->win.rect.x + window->win.rect.w - 10 - 34, window->win.rect.y,
        36, window->win.rect.h);
    uint8_t ropfn = (window->states & HEADER_USB) ? 0 : ROPFN_DISABLE;
    render_icon_align(rc, IDR_PNG_header_icon_usb,
        window->color_back, RENDER_FLG(ALIGN_CENTER, ropfn));

    for (int i = 1; i < 3; i++) {
        if (window->states & (1 << i)) {
            icons_width += 20;
            rc = rect_ui16(
                window->win.rect.x + window->win.rect.w - icons_width, window->win.rect.y,
                20, window->win.rect.h);
            uint16_t id_res = 0;
            switch (1 << i) {
            case HEADER_LAN:
                id_res = IDR_PNG_header_icon_lan;
                break;
            case HEADER_WIFI:
                id_res = IDR_PNG_header_icon_wifi;
                break;
            }
            render_icon_align(rc, id_res,
                window->color_back, RENDER_FLG(ALIGN_CENTER, 0));
        }
    }

    rc = window->win.rect;
    rc.x += 10 + window->win.rect.h;
    rc.w -= (icons_width + 10 + window->win.rect.h);

    if (window->label) { // label
        render_text_align(rc, window->label, window->font,
            window->color_back, window->color_text,
            window->padding, window->alignment);
    }
}

void p_window_header_set_icon(window_header_t* window, uint16_t id_res)
{
    window->id_res = id_res;
    _window_invalidate((window_t*)window);
}

void p_window_header_enable_state(window_header_t* window, header_states_t state)
{
    window->states |= state;
    _window_invalidate((window_t*)window);
}

void p_window_header_disable_state(window_header_t* window, header_states_t state)
{
    window->states &= ~state;
    _window_invalidate((window_t*)window);
}

uint8_t p_window_header_get_state(window_header_t* window, header_states_t state)
{
    return (uint8_t)(window->states & state);
}

void p_window_header_set_text(window_header_t* window, const char* text)
{
    window->label = text;
    _window_invalidate((window_t*)window);
}

int p_window_header_event_clr(window_header_t* window, uint8_t evt_id)
{
    if (marlin_event_clr(evt_id)) {
        switch (evt_id) {
        case MARLIN_EVT_MediaInserted:
            p_window_header_enable_state(window, HEADER_USB);
            break;
        case MARLIN_EVT_MediaRemoved:
            p_window_header_disable_state(window, HEADER_USB);
            break;
        }
        return 1;
    }
    return 0;
}

const window_class_header_t window_class_header = {
    {
        WINDOW_CLS_USER,
        sizeof(window_header_t),
        (window_init_t*)window_header_init,
        (window_done_t*)window_header_done,
        (window_draw_t*)window_header_draw,
        0,
    },
};
