// window_dlg_loadunload.c
#include "window_dlg_loadunload.h"
#include "window_dlg_loadunload_shared.h" //button flags
#include "display_helper.h"
#include "gui.h"
#include "marlin_client.h"
#include "dbg.h"
#include "menu_vars.h"
#include "stm32f4xx_hal.h"
#include <limits.h>

//choose 1
//#define DBG_LOAD_PROGRESS 1
#define DBG_LOAD_PROGRESS 0

//#define FRAME_ENA 1
#define FRAME_ENA 0

//dialog flags bitmasks
#define DLG_MSK_MOD 0x0003 // mode mask
#define DLG_MSK_CHG 0xc000 // change flag mask

//dialog flags bitshift
#define DLG_SHI_MOD 4 // mode shift
#define DLG_SHI_CHG 14 // change flag shift

#if FRAME_ENA == 1
#define DLG_DRA_FR 0x2000 // draw frame
#else
#define DLG_DRA_FR 0x0000 // draw frame
#endif
#define DLG_PHA_CH 0x4000 // phase changed
#define DLG_PRO_CH 0x8000 // progress changed

int16_t WINDOW_CLS_DLG_LOADUNLOAD = 0;

extern window_t* window_1; //current popup window

static uint32_t _phase_time_total(int phase, const _cl_dlg* _ths);

static void clr_logs();
static void is_part_log(int part_progress);
static void progress_changed_log(int progress);
static void phase_changed_log(int phase, int base_progress);
static void print_log();

void window_dlg_loadunload_init(window_dlg_loadunload_t* window)
{
    if (rect_empty_ui16(window->win.rect)) //use display rect if current rect is empty
        window->win.rect = rect_ui16(0, 0, display->w, display->h);
    window->win.flg |= WINDOW_FLG_ENABLED; //enabled by default
    window->color_back = gui_defaults.color_back;
    window->color_text = gui_defaults.color_text;
    window->font = gui_defaults.font;
    window->font_title = gui_defaults.font_big;
    window->padding = gui_defaults.padding;
    window->vars.phase = 0;
    window->vars.prev_phase = -1;
    window->vars.progress = 0;
    window->vars.prev_progress = 0;
    window->vars.part_progress = 0;
    window->vars.base_progress = 0;
    window->flags = 0;

    window->vars.tick_part_start = HAL_GetTick();
    window->vars.time_total = _phase_time_total(window->_ths->count - 1, window->_ths);
}

rect_ui16_t _get_dlg_loadunload_button_size(window_dlg_loadunload_t* window)
{
    rect_ui16_t rc_btn = window->win.rect;
    rc_btn.y += (rc_btn.h - 40); // 30pixels for button (+ 10 space for grey frame)
    rc_btn.h = 30;
    rc_btn.x += 6;
    rc_btn.w -= 12;
    return rc_btn;
}

extern void msgbox_draw_button(rect_ui16_t rc_btn, const char* text, font_t* pf, int is_selected);

void _window_dlg_loadunload_draw_button(window_dlg_loadunload_t* window)
{
    rect_ui16_t rc_btn = _get_dlg_loadunload_button_size(window);

    switch (window->_ths->p_states[window->vars.phase].p_button->flags & (BT_VISIBLE | BT_ENABLED)) {
    case BT_VISIBLE:
    case (BT_VISIBLE | BT_ENABLED):
        msgbox_draw_button(
            rc_btn,
            window->_ths->p_states[window->vars.phase].p_button->label,
            window->font_title,
            (window->_ths->p_states[window->vars.phase].p_button->flags & BT_ENABLED) ? 1 : 0);
        break;
    case BT_ENABLED: //enabled and not visible == do not clear
        break;
    case 0: //not enabled and not visible == clear
    default:
        display->fill_rect(rc_btn, window->color_back);
        break;
    }
}

void _window_dlg_loadunload_draw_frame(window_dlg_loadunload_t* window)
{
    rect_ui16_t rc = window->win.rect;
    display->draw_line(point_ui16(rc.x, rc.y), point_ui16(239, rc.y), COLOR_GRAY);
    display->draw_line(point_ui16(rc.x, rc.y), point_ui16(rc.x, 320 - 67), COLOR_GRAY);
    display->draw_line(point_ui16(239, rc.y), point_ui16(239, 320 - 67), COLOR_GRAY);
    display->draw_line(point_ui16(rc.x, 320 - 67), point_ui16(239, 320 - 67), COLOR_GRAY);
}

void _window_dlg_loadunload_draw_progress(window_dlg_loadunload_t* window, uint8_t progress)
{
    rect_ui16_t rc_pro = window->win.rect;
    char text[16];
    rc_pro.x += 10;
    rc_pro.w -= 20;
    rc_pro.h = 16;
    rc_pro.y += 30;
    uint16_t w = rc_pro.w;
    rc_pro.w = w * progress / 100;
    display->fill_rect(rc_pro, COLOR_ORANGE);
    rc_pro.x += rc_pro.w;
    rc_pro.w = w - rc_pro.w;
    display->fill_rect(rc_pro, COLOR_GRAY);
    rc_pro.y += rc_pro.h;
    rc_pro.w = window->win.rect.w - 120;
    rc_pro.x = window->win.rect.x + 60;
    rc_pro.h = 30;
    sprintf(text, "%d%%", progress);
    render_text_align(rc_pro, text, window->font_title, window->color_back, window->color_text, window->padding, ALIGN_CENTER);
}

void _window_dlg_loadunload_draw_phase_text(window_dlg_loadunload_t* window)
{
    rect_ui16_t rc_sta = window->win.rect;
    rc_sta.h = 30;
    rc_sta.y += (30 + 46);
    rc_sta.x += 2;
    rc_sta.w -= 4;
    render_text_align(rc_sta, window->_ths->p_states[window->vars.phase].text, window->font_title,
        window->color_back, window->color_text, window->padding, ALIGN_CENTER);
}

void window_dlg_loadunload_draw(window_dlg_loadunload_t* window)
{
    if ((window->win.f_visible) && ((size_t)(window->vars.phase) < window->_ths->count)) {
        rect_ui16_t rc = window->win.rect;

        if (window->win.f_invalid) {
            display->fill_rect(rc, window->color_back);
            rect_ui16_t rc_tit = rc;
            rc_tit.h = 30; // 30pixels for title
            // TODO: - icon
            //			rc_tit.w -= 30;
            //			rc_tit.x += 30;
            render_text_align(rc_tit, window->_ths->title, window->font_title,
                window->color_back, window->color_text, window->padding, ALIGN_CENTER);
            _window_dlg_loadunload_draw_button(window);

            window->win.f_invalid = 0;
            window->flags |= DLG_DRA_FR | DLG_PHA_CH | DLG_PRO_CH;
        }
        if (window->flags & DLG_PHA_CH) //phase changed
        {
            window->flags &= ~DLG_PHA_CH;
            _window_dlg_loadunload_draw_phase_text(window);
            _window_dlg_loadunload_draw_button(window);
        }
        if (window->flags & DLG_PRO_CH && (!(window->flags & LD_DI_PRS))) //progress changed and enabled
        {
            window->flags &= ~DLG_PRO_CH;
            _window_dlg_loadunload_draw_progress(window, window->vars.progress);
        }
        if (window->flags & DLG_DRA_FR) { //draw frame
            window->flags &= ~DLG_DRA_FR;
            _window_dlg_loadunload_draw_frame(window);
        }

        if (window->vars.draw_cb != NULL)
            window->vars.draw_cb(window);
    }
}

void window_dlg_loadunload_event(window_dlg_loadunload_t* window, uint8_t event, void* param)
{
    if (window->vars.event_cb != NULL)
        window->vars.event_cb(window, &event, param);
    switch (event) {
    case WINDOW_EVENT_BTN_DN:
    //case WINDOW_EVENT_BTN_UP:
    case WINDOW_EVENT_CLICK:
        window->vars.flags |= LD_BT_FLG;
        return;
    }
}

const window_class_dlg_loadunload_t window_class_dlg_loadunload = {
    {
        WINDOW_CLS_USER,
        sizeof(window_dlg_loadunload_t),
        (window_init_t*)window_dlg_loadunload_init,
        0,
        (window_draw_t*)window_dlg_loadunload_draw,
        (window_event_t*)window_dlg_loadunload_event,
    },
};

ld_result_t _gui_dlg(const _cl_dlg* _ths, void* p_additional_vars, int32_t ttl)
{
    clr_logs();

    window_dlg_loadunload_t dlg;
    memset(&dlg, 0, sizeof(dlg));
    dlg._ths = _ths;
    int16_t id_capture = window_capture();
    int16_t id = window_create_ptr(WINDOW_CLS_DLG_LOADUNLOAD, 0, gui_defaults.msg_box_sz, &dlg);
    window_1 = (window_t*)&dlg;
    gui_reset_jogwheel();
    gui_invalidate();
    window_set_capture(id);

    uint32_t start_tick = HAL_GetTick();
    marlin_event_clr(MARLIN_EVT_CommandEnd);
    while ((size_t)(dlg.vars.phase) < _ths->count) //negative number will force exit
    {
        //after M70X is sent i must check marlin_command()
        //0 == M70X ended - dialog has no reason to exist anymore
        if ((dlg.vars.flags & LD_CH_CMD) && (marlin_event_clr(MARLIN_EVT_CommandEnd))) {
            break;
        }

        //time to live reached?
        if ((uint32_t)(HAL_GetTick() - start_tick) >= (uint32_t)ttl) {
            if (_ths->on_timeout != NULL)
                _ths->on_timeout();
            break;
        }

        dlg.vars.p_marlin_vars = marlin_update_vars(
            MARLIN_VAR_MSK(MARLIN_VAR_MOTION) | MARLIN_VAR_MSK(MARLIN_VAR_POS_E) | MARLIN_VAR_MSK(MARLIN_VAR_TEMP_NOZ) | MARLIN_VAR_MSK(MARLIN_VAR_TTEM_NOZ));

        //button auto exit flag must be handled here
        //state could change phase
        if (
            (dlg.vars.flags & LD_BT_FLG) && //button clicked
            (_ths->p_states[dlg.vars.phase].p_button->flags & BT_AUTOEXIT) //autoexit flag on current button
        ) {
            break;
        }

        int part_progress = _ths->p_states[dlg.vars.phase].state_fnc(&(dlg.vars), p_additional_vars); //state machine

        //forced exit
        //intended to be used as handling  user clicked exit button
        //any state can do that
        if ((size_t)(dlg.vars.phase) >= _ths->count)
            break;

        dlg.vars.flags &= ~LD_BT_FLG; //disable button clicked

        is_part_log(part_progress); //log raw value obtained from state

        if (part_progress > dlg.vars.part_progress)
            dlg.vars.part_progress = (uint8_t)part_progress; //progress can only rise
        if (part_progress > 100)
            dlg.vars.part_progress = 100;
        if (part_progress < 0)
            dlg.vars.part_progress = 0;

        if (dlg.vars.prev_phase != dlg.vars.phase) {
            dlg.vars.part_progress = 0;
            dlg.flags |= DLG_PHA_CH;
            gui_invalidate();
            dlg.vars.base_progress = 100 * _phase_time_total(dlg.vars.prev_phase, _ths) / dlg.vars.time_total;
            dlg.vars.prev_phase = dlg.vars.phase;
            dlg.vars.tick_part_start = HAL_GetTick();

            phase_changed_log(dlg.vars.phase, dlg.vars.base_progress);
        }

        //dlg.vars.part_progress MUST remain unscaled
        //dlg.vars.base_progress is scaled
        part_progress = (int)(dlg.vars.part_progress) * (_ths->p_states[dlg.vars.phase].time) / dlg.vars.time_total;
        dlg.vars.progress = dlg.vars.base_progress + part_progress;

        if (dlg.vars.prev_progress != dlg.vars.progress) {
            //dlg.progress = progress;
            dlg.flags |= DLG_PRO_CH;
            gui_invalidate();
            progress_changed_log(dlg.vars.progress);
            dlg.vars.prev_progress = dlg.vars.progress;
        }

        print_log();
        gui_loop();
    }
    if (_ths->on_done != NULL)
        _ths->on_done();
    window_destroy(id);
    window_set_capture(id_capture);
    window_invalidate(0);
    return 0;
}

static uint32_t _phase_time_total(int phase, const _cl_dlg* _ths)
{
    uint32_t time = 0;
    while (phase >= 0)
        time += _ths->p_states[phase--].time;
    return time;
}

/*****************************************************************************/
//DEBUG PRINT LOAD PROGRESS
#if DBG_LOAD_PROGRESS == 1

static int log_flag;
static int base_progress_log_min;
static int base_progress_log_max;
static int part_progress_log_min;
static int part_progress_log_max;
static int progress_log;
static int phase_log;
static marlin_vars_t* p_marlin_vars;

static void clr_logs()
{
    p_marlin_vars = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_MOTION) | MARLIN_VAR_MSK(MARLIN_VAR_POS_E));
    log_flag = 0;
    base_progress_log_min = INT_MAX;
    base_progress_log_max = INT_MIN;
    part_progress_log_min = INT_MAX;
    part_progress_log_max = INT_MIN;
    progress_log = -1;
    phase_log = -1;
}

static void is_part_log(int part_progress)
{
    if (part_progress_log_min > part_progress) {
        part_progress_log_min = part_progress;
        log_flag = 1;
        //_dbg("part_progress_min[%d] = %d",phase, part_progress);
    }
    if (part_progress_log_max < part_progress) {
        part_progress_log_max = part_progress;
        log_flag = 1;
        //_dbg("part_progress_max[%d] = %d",phase, part_progress);
    }
}

static void progress_changed_log(int progress)
{
    log_flag = 1;
    progress_log = progress;
}

static void phase_changed_log(int phase, int base_progress)
{
    int _progress_log = progress_log; //save progress
    clr_logs();
    progress_log = _progress_log; //restore
    log_flag = 1;
    phase_log = phase;

    if (base_progress_log_min > base_progress) {
        base_progress_log_min = base_progress;
        log_flag = 1;
    }
    if (base_progress_log_max < base_progress) {
        base_progress_log_max = base_progress;
        log_flag = 1;
    }
}

static void print_log()
{
    if (log_flag) {
        log_flag = 0;
        _dbg("phase = %d, progress = %d", phase_log, progress_log);
        _dbg("part_progress_min = %d", part_progress_log_min);
        _dbg("part_progress_max = %d", part_progress_log_max);
        _dbg("base_progress_min = %d", base_progress_log_min);
        _dbg("base_progress_max = %d", base_progress_log_max);
        _dbg("E pos = %f", (double)p_marlin_vars->pos[MARLIN_VAR_INDEX_E]);
        _dbg("Z pos = %f", (double)p_marlin_vars->pos[MARLIN_VAR_INDEX_Z]);
    }
}

#else //DBG_LOAD_PROGRESS
static void clr_logs()
{
}

static void is_part_log(int part_progress) {}

static void progress_changed_log(int progress) {}

static void phase_changed_log(int phase, int base_progress) {}

static void print_log() {}

#endif //DBG_LOAD_PROGRESS
