// window_dlg_load.c
// does not have header
// window_dlg_loadunload.h used instead

#include "window_dlg_loadunload.h"
#include "marlin_client.h"
#include "menu_vars.h"
#include "stm32f4xx_hal.h"
#include <limits.h>
#include "window_dlg_preheat.h"
#include "window_dlg_loadunload_shared.h"
#include "gui.h" //gui_defaults

static const _cl_dlg cl_load;

const float ld_purge_amount = 40.0F; //todo is this correct?

extern void _window_dlg_loadunload_draw_progress(window_dlg_loadunload_t* window, uint8_t progress);
extern rect_ui16_t _get_dlg_loadunload_button_size(window_dlg_loadunload_t* window);
extern void msgbox_draw_button(rect_ui16_t rc_btn, const char* text, font_t* pf, int is_selected);

void window_dlg_load_draw_buttons(window_dlg_loadunload_t* window)
{
    rect_ui16_t rc_btn = _get_dlg_loadunload_button_size(window);
    //todo draw only when needed

    //todo rewrite condition
    switch (window->_ths->p_states[window->vars.phase].p_button->flags & (BT_VISIBLE | BT_ENABLED)) {
    case 0: //not enabled and not visible == clear
        display->fill_rect(rc_btn, window->color_back);
        break;
    default: {
        int16_t btn_width = rc_btn.w / 2 - gui_defaults.btn_spacing;

        rc_btn.w = btn_width;
        msgbox_draw_button(
            rc_btn,
            "DONE",
            window->font_title,
            (window->vars.flags & LD_BT_DONE_NPURG) ? 1 : 0);

        //more difficult calculations of coords to avoid round errors

        //space between buttons
        rc_btn.x += btn_width;
        rc_btn.w = _get_dlg_loadunload_button_size(window).w - rc_btn.w * 2;
        display->fill_rect(rc_btn, window->color_back);

        //distance of both buttons from screen sides is same
        rc_btn.x += rc_btn.w;
        rc_btn.w = btn_width;
        msgbox_draw_button(
            rc_btn,
            "PURGE",
            window->font_title,
            (window->vars.flags & LD_BT_DONE_NPURG) ? 0 : 1);
    } break;
    }
}

void window_dlg_load_draw_cb(window_dlg_loadunload_t* window)
{
    //window->vars.part_progress - show current progress of purge
    _window_dlg_loadunload_draw_progress(window, window->vars.part_progress);

    //draw buttons - like msg box
    window_dlg_load_draw_buttons(window);
}

//have to clear handled events
void window_dlg_load_event_cb(window_dlg_loadunload_t* window, uint8_t* p_event, void* param)
{
    uint8_t* p_flags = &window->vars.flags;
    switch (*p_event) {
    case WINDOW_EVENT_BTN_DN:
    case WINDOW_EVENT_CLICK:
        if ((*p_flags) & LD_BT_DONE_NPURG)
            window->vars.flags |= LD_BT_DONE; //set done  button
        else
            window->vars.flags |= LD_BT_PURG; //set purge button
        *p_event = 0;
        return;
    case WINDOW_EVENT_ENC_DN:
    case WINDOW_EVENT_ENC_UP:
        (*p_flags) ^= LD_BT_DONE_NPURG; //invert LD_BT_DONE_NPURG flag
        *p_event = 0;
        return;
    }
}

ld_result_t _gui_dlg_load(void)
{
    _dlg_ld_vars ld_vars;
    memset(&ld_vars, '\0', sizeof(ld_vars));
    ld_vars.z_min_extr_pos = 30;
    return _gui_dlg(&cl_load, &ld_vars, 600000); //10min
}

ld_result_t gui_dlg_load(void)
{
    //todo must be called inside _gui_dlg, but nested dialogs are not supported now
    if (gui_dlg_preheat(NULL) < 1)
        return LD_ABORTED; //0 is return
    return _gui_dlg_load();
}

ld_result_t gui_dlg_load_forced(void)
{
    //todo must be called inside _gui_dlg, but nested dialogs are not supported now
    if (gui_dlg_preheat_forced("PREHEAT for LOAD") < 0)
        return LD_ABORTED; //LD_ABORTED should not happen
    return _gui_dlg_load();
}

/*****************************************************************************/
//LOAD
static int f_LD_GCODE(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    marlin_gcode("M701 Z0");
    p_vars->phase++;
    p_vars->flags |= LD_CH_CMD; //kill screen after M701 ends
    return 0;
}

static int f_LD_INSERT_FILAMENT(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    if (p_vars->flags & LD_BT_FLG) {
        p_vars->flags &= ~LD_BT_FLG;
        p_vars->phase++;
    }
    return 0;
}

static int f_LD_WAIT_E_POS__INSERTING(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    //wait E pos >= 40
    if (p_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] >= 40)
        p_vars->phase++;
    return 100 * (p_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E]) / 40;
}

static int f_LD_WAIT_E_POS__LOADING_TO_NOZ(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    //wait E pos >= 360
    if (p_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] >= 360)
        p_vars->phase++;
    float ret = 100 * (p_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] - 40) / (360 - 40);
    return ret;
}

static int f_LD_WAIT_E_POS__PURGING(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    //wait E pos >= 400
    if (p_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] >= 400)
        p_vars->phase++;
    return 100 * (p_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] - 360) / (400 - 360);
}

static int f_LD_CHECK_MARLIN_EVENT(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    if (marlin_event_clr(MARLIN_EVT_HostPrompt))
        p_vars->phase++;
    return 0;
}

static int f_LD_PURGE_USER_INTERACTION(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    switch (p_vars->flags & (LD_BT_PURG | LD_BT_DONE)) {
    case LD_BT_PURG:
        marlin_host_button_click(HOST_PROMPT_BTN_PurgeMore);
        additional_vars->e_last = p_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E]; //needed in next phase
        p_vars->phase++; //f_LD_PURGE_SHOW_PROGRESS
        break;
    case LD_BT_DONE:
    case (LD_BT_DONE | LD_BT_PURG): //if both buttons are clicked DONE has priority, but should not happen
        marlin_host_button_click(HOST_PROMPT_BTN_Continue);
        p_vars->phase += 2; //f_LD_REINIT_FROM_PURGE
        break;
    }
    return 100; //progressbar MUST be 100
}

static int f_LD_PURGE_SHOW_PROGRESS(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{

    if (marlin_event_clr(MARLIN_EVT_HostPrompt)) {
        p_vars->phase--; //jump back to f_LD_PURGE_USER_INTERACTION
        p_vars->flags &= (~(LD_BT_PURG | LD_BT_DONE | LD_BT_DONE_NPURG)); //clr buttons
        return 0;
    }

    float ret = 100.0F * (p_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] - additional_vars->e_last)
        / ld_purge_amount;
    if (ret > 99.0F)
        return 99;
    return (int)ret;
}

//externed in "window_dlg_loadunload_shared.h"
int f_LD_REINIT_TO_PURGE(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    p_vars->flags |= LD_DI_PRS; // disable progress (draw_cb will handle it)
    p_vars->draw_cb = window_dlg_load_draw_cb; // called at the end of draw (if not NULL)
    p_vars->event_cb = window_dlg_load_event_cb; // called at the begin of event (if not NULL)
    p_vars->phase++;
    return 0;
}

int f_LD_REINIT_FROM_PURGE(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    p_vars->flags &= ~LD_DI_PRS; // disable progress (draw_cb will handle it)
    p_vars->draw_cb = NULL; // called at the end of draw (if not NULL)
    p_vars->event_cb = NULL; // called at the begin of event (if not NULL)
    p_vars->phase++;
    return 0;
}

/*****************************************************************************/
//LOAD
//name of func serves as comment
static const _dlg_state load_states[] = {
    { 0, "Parking", &bt_stop_ena, (dlg_state_func)f_SH_INIT },
    { 0, "Parking", &bt_stop_ena, (dlg_state_func)f_SH_MOVE_INITIAL_Z },
    { 0, "Parking", &bt_stop_ena, (dlg_state_func)f_SH_WAIT_INITIAL_Z_MOTION },
    { 3000, "Parking", &bt_stop_ena, (dlg_state_func)f_SH_WAIT_INITIAL_Z_STOPPED },
    { 10000, "Waiting for temp.", &bt_stop_ena, (dlg_state_func)f_SH_WAIT_TEMP },
    { 0, "Insert filament.", &bt_cont_ena, (dlg_state_func)f_LD_INSERT_FILAMENT },
    { 0, "Inserting", &bt_stop_dis, (dlg_state_func)f_LD_GCODE },
    { 6000, "Inserting", &bt_stop_dis, (dlg_state_func)f_LD_WAIT_E_POS__INSERTING },
    { 10000, "Loading to nozzle", &bt_stop_dis, (dlg_state_func)f_LD_WAIT_E_POS__LOADING_TO_NOZ },
    { 10000, "Purging", &bt_stop_dis, (dlg_state_func)f_LD_WAIT_E_POS__PURGING },
    //from now all times must be zero, draw_cb handles progress bar differently
    { 0, "Purging", &bt_none, (dlg_state_func)f_LD_CHECK_MARLIN_EVENT },
    { 0, "Purging", &bt_none, (dlg_state_func)f_LD_REINIT_TO_PURGE },
    { 0, "Purging", &bt_dont_clr, (dlg_state_func)f_LD_PURGE_USER_INTERACTION }, //can end (state += 2)
    { 0, "Purging", &bt_none, (dlg_state_func)f_LD_PURGE_SHOW_PROGRESS }, //can jump back (state --)
    { 0, "Finished", &bt_none, (dlg_state_func)f_LD_REINIT_FROM_PURGE },
    { 0, "Finished", &bt_none, (dlg_state_func)f_SH_WAIT_READY }
};

static void load_timeout_cb(void)
{
    marlin_host_button_click(HOST_PROMPT_BTN_Continue);
}

static const _cl_dlg cl_load = {
    "Loading filament",
    load_states,
    sizeof(load_states) / sizeof(load_states[0]),
    load_timeout_cb,
    NULL, //on_done
};
