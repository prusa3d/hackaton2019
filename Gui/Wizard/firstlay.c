// firstlay.c

#include "firstlay.h"
#include "dbg.h"
#include "config.h"
#include "stm32f4xx_hal.h"
#include "marlin_client.h"
#include "wizard_config.h"
#include "wizard_ui.h"
#include "wizard_types.h"
#include "wizard_progress_bar.h"
#include <stdio.h>
#include <string.h>
#include "guitypes.h" //font_meas_text
#include "menu_vars.h"
#include "filament.h"

const char* V2_gcodes_head_PLA[];
const char* V2_gcodes_head_PETG[];
const char* V2_gcodes_head_ASA[];
const char* V2_gcodes_head_FLEX[];
const char* V2_gcodes_body[];

const size_t V2_gcodes_head_PLA_sz;
const size_t V2_gcodes_head_PETG_sz;
const size_t V2_gcodes_head_ASA_sz;
const size_t V2_gcodes_head_FLEX_sz;
const size_t V2_gcodes_body_sz;

//todo use marlin api
const size_t commands_in_queue_size = 8;
const size_t commands_in_queue_use_max = 6;
const size_t max_gcodes_in_one_run = 20; //milion of small gcodes could be done instantly but block gui

static uint32_t line_head = 0;
static uint32_t line_body = 0;

static const char** head_gcode = NULL;
static const char** body_gcode = NULL;
static size_t head_gcode_sz = -1;
static size_t body_gcode_sz = -1;
static size_t gcode_sz = -1;

int _get_progress();

void _set_gcode_first_lines();

//returns remaining lines
int _run_gcode_line(uint32_t* p_line, const char* gcodes[], size_t gcodes_count, window_term_t* term);

void _wizard_firstlay_Z_step(firstlay_screen_t* p_screen);

static const char* _wizard_firstlay_text = "Adjust the nozzle\n"
                                           "height above the\n"
                                           "heatbed by turning\n"
                                           "the knob.";
void wizard_init_screen_firstlay(int16_t id_body, firstlay_screen_t* p_screen, firstlay_data_t* p_data)
{
    //marlin_vars_t* vars        = marlin_update_vars( MARLIN_VAR_MSK(MARLIN_VAR_Z_OFFSET) );
    //p_screen->Z_offset         = vars->z_offset;
    p_screen->Z_offset_request = 0;

    point_ui16_t pt;
    int16_t id;
    window_destroy_children(id_body);
    window_show(id_body);
    window_invalidate(id_body);

    uint16_t y = 40;
    uint16_t x = WIZARD_MARGIN_LEFT;

    pt = font_meas_text(resource_font(IDR_FNT_NORMAL), _wizard_firstlay_text);
    pt.x += 5;
    pt.y += 5;
    id = window_create_ptr(WINDOW_CLS_TEXT, id_body, rect_ui16(x, y, pt.x, pt.y), &(p_screen->text_state));
    p_screen->text_state.font = resource_font(IDR_FNT_NORMAL);
    window_set_text(id, _wizard_firstlay_text);

    y += pt.y;

    id = window_create_ptr(WINDOW_CLS_TEXT, id_body, rect_ui16(x, y, 70, 22), &(p_screen->text_Z_pos));
    window_set_text(id, "Z pos.:");

    id = window_create_ptr(WINDOW_CLS_NUMB, id_body, rect_ui16(x + 70, y, 70, 22), &(p_screen->spin_baby_step));
    window_set_format(id, "%.3f");
    window_set_value(id, p_screen->Z_offset);
    p_screen->spin_baby_step.color_text = COLOR_GRAY;
    //window_set_color_text(id, COLOR_GRAY);

    id = window_create_ptr(WINDOW_CLS_TEXT, id_body, rect_ui16(x + 70 + 70, y, WIZARD_X_SPACE - x - 70 - 70, 22),
        &(p_screen->text_direction_arrow));
    window_set_text(id, "-|+");

    y += 22 + 10;

    id = window_create_ptr(WINDOW_CLS_TERM, id_body,
        rect_ui16(10, y,
            resource_font(IDR_FNT_SMALL)->w * FIRSTLAY_SCREEN_TERM_X,
            resource_font(IDR_FNT_SMALL)->h * FIRSTLAY_SCREEN_TERM_Y),
        &(p_screen->term));
    p_screen->term.font = resource_font(IDR_FNT_SMALL);
    term_init(&(p_screen->terminal), FIRSTLAY_SCREEN_TERM_X, FIRSTLAY_SCREEN_TERM_Y, p_screen->term_buff);
    p_screen->term.term = &(p_screen->terminal);

    y += 18 * FIRSTLAY_SCREEN_TERM_Y + 3;
    id = window_create_ptr(WINDOW_CLS_PROGRESS, id_body, rect_ui16(x, y, WIZARD_X_SPACE, 8), &(p_screen->progress));
}

int wizard_firstlay_print(int16_t id_body, firstlay_screen_t* p_screen, firstlay_data_t* p_data, float z_offset)
{
    if (p_data->state_print == _TEST_START) {
        p_screen->state = _FL_INIT;
        p_data->state_print = _TEST_RUN;

        body_gcode = V2_gcodes_body;
        body_gcode_sz = V2_gcodes_body_sz;
        switch (get_filament()) {
        case FILAMENT_PETG:
            head_gcode = V2_gcodes_head_PETG;
            head_gcode_sz = V2_gcodes_head_PETG_sz;
            break;
        case FILAMENT_ASA:
            head_gcode = V2_gcodes_head_ASA;
            head_gcode_sz = V2_gcodes_head_ASA_sz;
            break;
        case FILAMENT_FLEX:
            head_gcode = V2_gcodes_head_FLEX;
            head_gcode_sz = V2_gcodes_head_FLEX_sz;
            break;
        case FILAMENT_PLA:
        default:
            head_gcode = V2_gcodes_head_PLA;
            head_gcode_sz = V2_gcodes_head_PLA_sz;
            break;
        }

        gcode_sz = body_gcode_sz + head_gcode_sz;
    }

    int remaining_lines;
    switch (p_screen->state) {
    case _FL_INIT:
        p_screen->Z_offset = z_offset;
        wizard_init_screen_firstlay(id_body, p_screen, p_data);
        _set_gcode_first_lines();
        p_screen->state = _FL_GCODE_HEAD;
        break;
    case _FL_GCODE_HEAD:
        remaining_lines = _run_gcode_line(&line_head, head_gcode,
            head_gcode_sz, &p_screen->term);
        if (remaining_lines < 1) {
            p_screen->state = _FL_GCODE_BODY;
            p_screen->Z_offset_request = 0; //ignore Z_offset_request variable changes until now
            p_screen->spin_baby_step.color_text = COLOR_ORANGE;
            window_invalidate(p_screen->spin_baby_step.win.id);
        }
        break;
    case _FL_GCODE_BODY:
        _wizard_firstlay_Z_step(p_screen);
        remaining_lines = _run_gcode_line(&line_body, body_gcode,
            body_gcode_sz, &p_screen->term);
        if (remaining_lines < 1) {
            p_screen->state = _FL_GCODE_DONE;
        }
        break;
    case _FL_GCODE_DONE:
        p_data->state_print = _TEST_PASSED;
        //marlin_settings_save();
        p_screen->Z_offset_request = 0;
        return 100;
    }

    int progress = _get_progress(); //max 99

    window_set_value(p_screen->progress.win.id, (float)progress);
    return progress;
}

void wizard_firstlay_event_dn(firstlay_screen_t* p_screen)
{
    //todo term is bugged spinner can make it not showing
    window_invalidate(p_screen->term.win.id);
    p_screen->Z_offset_request -= z_offset_step;
}

void wizard_firstlay_event_up(firstlay_screen_t* p_screen)
{
    //todo term is bugged spinner can make it not showing
    window_invalidate(p_screen->term.win.id);
    p_screen->Z_offset_request += z_offset_step;
}

void _wizard_firstlay_Z_step(firstlay_screen_t* p_screen)
{
    int16_t numb_id = p_screen->spin_baby_step.win.id;
    int16_t arrow_id = p_screen->text_direction_arrow.win.id;

    //need last step to ensure correct behavior on limits
    float _step_last = p_screen->Z_offset;
    p_screen->Z_offset += p_screen->Z_offset_request;

    if (p_screen->Z_offset > z_offset_max)
        p_screen->Z_offset = z_offset_max;
    if (p_screen->Z_offset < z_offset_min)
        p_screen->Z_offset = z_offset_min;

    marlin_do_babysteps_Z(p_screen->Z_offset - _step_last);

    if (p_screen->Z_offset_request > 0) {
        window_set_value(numb_id, p_screen->Z_offset);
        window_set_text(arrow_id, "+++");
    } else if (p_screen->Z_offset_request < 0) {
        window_set_value(numb_id, p_screen->Z_offset);
        window_set_text(arrow_id, "---");
    }

    p_screen->Z_offset_request = 0;
}

#define V__GCODES_HEAD_BEGIN                \
    "M107", /*fan off */                    \
        "G90", /*use absolute coordinates*/ \
        "M83", /*extruder relative mode*/

#define V__GCODES_HEAD_END                   \
    "G28 X", /*HOME X MUST BE ONLY X*/       \
        X_home_gcode, /*Set X pos */         \
        "G28 Y", /*HOME Y MUST BE ONLY Y*/   \
        Y_home_gcode, /*Set Y pos */         \
        "G28", /*autohome*/                  \
        "G29", /*meshbed leveling*/          \
        "G21", /* set units to millimeters*/ \
        "G90", /* use absolute coordinates*/ \
        "M83", /* use relative distances for extrusion*/

//todo generate me
const char* V2_gcodes_head_PLA[] = {
    V__GCODES_HEAD_BEGIN
    "M104 S215", //nozzle target 215C
    "M140 S60", //bed target 60C
    "M190 S60", //wait for bed temp 60C
    "M109 S215", //wait for nozzle temp 215C
    V__GCODES_HEAD_END
};
const size_t V2_gcodes_head_PLA_sz = sizeof(V2_gcodes_head_PLA) / sizeof(V2_gcodes_head_PLA[0]);

const char* V2_gcodes_head_PETG[] = {
    V__GCODES_HEAD_BEGIN
    "M104 S230", //nozzle target 215C
    "M140 S85", //bed target 60C
    "M190 S85", //wait for bed temp 60C
    "M109 S230", //wait for nozzle temp 215C
    V__GCODES_HEAD_END
};
const size_t V2_gcodes_head_PETG_sz = sizeof(V2_gcodes_head_PETG) / sizeof(V2_gcodes_head_PETG[0]);

const char* V2_gcodes_head_ASA[] = {
    V__GCODES_HEAD_BEGIN
    "M104 S260", //nozzle target 215C
    "M140 S100", //bed target 60C
    "M190 S100", //wait for bed temp 60C
    "M109 S260", //wait for nozzle temp 215C
    V__GCODES_HEAD_END
};
const size_t V2_gcodes_head_ASA_sz = sizeof(V2_gcodes_head_ASA) / sizeof(V2_gcodes_head_ASA[0]);

const char* V2_gcodes_head_FLEX[] = {
    V__GCODES_HEAD_BEGIN
    "M104 S240", //nozzle target 215C
    "M140 S50", //bed target 60C
    "M190 S50", //wait for bed temp 60C
    "M109 S240", //wait for nozzle temp 215C
    V__GCODES_HEAD_END
};
const size_t V2_gcodes_head_FLEX_sz = sizeof(V2_gcodes_head_FLEX) / sizeof(V2_gcodes_head_FLEX[0]);

//#define EXTRUDE_PER_MM 0.025

//todo generate me
const char* V2_gcodes_body[] = {
    "G1 Z4 F1000",
    "G1 X0 Y-2 Z0.2 F3000.0",
    "G1 E6 F2000",
    "G1 X60 E9 F1000.0",
    "G1 X100 E12.5 F1000.0",
    "G1 Z2 E-6 F2100.00000",

    "G1 X10 Y150 Z0.2 F3000",
    "G1 E6 F2000"

    "G1 F1000",
    "G1 X170 Y150 E4", //160 * 0.025 = 4
    "G1 X170 Y130 E0.5", //20 * 0.025 = 0.5
    "G1 X10  Y130 E4",
    "G1 X10  Y110 E0.5",
    "G1 X170 Y110 E4",
    "G1 X170 Y90  E0.5",
    "G1 X10  Y90  E4",
    "G1 X10  Y70  E0.5",
    "G1 X170 Y70  E4",
    "G1 X170 Y50  E0.5",
    "G1 X10  Y50  E4",
    "G1 X10  Y30  E0.5",

    "G1 F1000",
    "G1 X30  Y30.0  E0.5", //20 * 0.025 = 0.5
    "G1 X30  Y29.5  E0.0125", //0.5 * 0.025 = 0.0125
    "G1 X10  Y29.5  E0.5",
    "G1 X10  Y29.0  E0.0125",
    "G1 X30  Y29.0  E0.5",
    "G1 X30  Y28.5  E0.0125",
    "G1 X10  Y28.5  E0.5",
    "G1 X10  Y28.0  E0.0125",
    "G1 X30  Y28.0  E0.5",
    "G1 X30  Y27.5  E0.0125",
    "G1 X10  Y27.5  E0.5",
    "G1 X10  Y27.0  E0.0125",
    "G1 X30  Y27.0  E0.5",
    "G1 X30  Y26.5  E0.0125",
    "G1 X10  Y26.5  E0.5",
    "G1 X10  Y26.0  E0.0125",
    "G1 X30  Y26.0  E0.5",
    "G1 X30  Y25.5  E0.0125",
    "G1 X10  Y25.5  E0.5",
    "G1 X10  Y25.0  E0.0125",
    "G1 X30  Y25.0  E0.5",
    "G1 X30  Y24.5  E0.0125",
    "G1 X10  Y24.5  E0.5",
    "G1 X10  Y24.0  E0.0125",
    "G1 X30  Y24.0  E0.5",
    "G1 X30  Y23.5  E0.0125",
    "G1 X10  Y23.5  E0.5",
    "G1 X10  Y23.0  E0.0125",
    "G1 X30  Y23.0  E0.5",
    "G1 X30  Y22.5  E0.0125",
    "G1 X10  Y22.5  E0.5",
    "G1 X10  Y22.0  E0.0125",
    "G1 X30  Y22.0  E0.5",
    "G1 X30  Y21.5  E0.0125",
    "G1 X10  Y21.5  E0.5",
    "G1 X10  Y21.0  E0.0125",
    "G1 X30  Y21.0  E0.5",
    "G1 X30  Y20.5  E0.0125",
    "G1 X10  Y20.5  E0.5",
    "G1 X10  Y20.0  E0.0125",
    "G1 X30  Y20.0  E0.5",
    "G1 X30  Y19.5  E0.0125",
    "G1 X10  Y19.5  E0.5",
    "G1 X10  Y19.0  E0.0125",
    "G1 X30  Y19.0  E0.5",
    "G1 X30  Y18.5  E0.0125",
    "G1 X10  Y18.5  E0.5",
    "G1 X10  Y18.0  E0.0125",
    "G1 X30  Y18.0  E0.5",
    "G1 X30  Y17.5  E0.0125",

    "G1 Z2 E-6 F2100",
    "G1 X180 Y0 Z10 F3000",

    "G4",

    "M107",
    "M104 S0", // turn off temperature
    "M140 S0", // turn off heatbed
    "M84" // disable motors
};
const size_t V2_gcodes_body_sz = sizeof(V2_gcodes_body) / sizeof(V2_gcodes_body[0]);

int _get_progress()
{
    //if ( _is_gcode_end_line() ) return 100;
    int ret = 100 * (line_head + 1 + line_body + 1) / gcode_sz;
    if (ret > 99)
        return 99;
    return ret;
}

//returns progress
void _set_gcode_first_lines()
{
    line_head = 0;
    line_body = 0;
}

int _run_gcode_line(uint32_t* p_line, const char* gcodes[], size_t gcodes_count, window_term_t* term)
{
    size_t gcodes_in_this_run = 0;

    //todo "while" does not work ...why?, something with  commands_in_queue?
    //while(commands_in_queue < commands_in_queue_use_max){
    if (marlin_get_gqueue() < 1) {
        if ((*p_line) < gcodes_count) {
            ++gcodes_in_this_run;
            marlin_gcode(gcodes[*p_line]);
            term_printf(term->term, "%s\n", gcodes[*p_line]);
            ++(*p_line);
            window_invalidate(term->win.id);
        }
    }

    //commands_in_queue does not reflect commands added in this run
    return gcodes_count - (*p_line) + marlin_get_gqueue() + gcodes_in_this_run;
}
