// window_dlg_loadunload.c
#include "window_dlg_loadunload.h"
#include "window_dlg_loadunload_shared.h"
#include "display_helper.h"
#include "gui.h"
#include "marlin_client.h"
#include "dbg.h"
#include "menu_vars.h"
#include "stm32f4xx_hal.h"
#include <limits.h>
/*****************************************************************************/
//shared for LOAD and UNLOAD
const _dlg_button_t bt_stop_ena = { "STOP", BT_VISIBLE | BT_ENABLED | BT_AUTOEXIT };
const _dlg_button_t bt_stop_dis = { "STOP", BT_VISIBLE };
const _dlg_button_t bt_cont_ena = { "CONTINUE", BT_VISIBLE | BT_ENABLED };
const _dlg_button_t bt_cont_dis = { "CONTINUE", BT_VISIBLE };
const _dlg_button_t bt_none = { "", 0 };
const _dlg_button_t bt_dont_clr = { "", BT_ENABLED }; //enabled but not visible == do not clear

int f_SH_INIT(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    marlin_gcode("G92 E0"); //saves calculation
    additional_vars->z_start = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_POS_Z))->pos[2];
    p_vars->phase++;
    return 0;
}

int _was_move(_dlg_ld_vars* additional_vars)
{
    return additional_vars->initial_move > z_offset_step;
}

int f_SH_MOVE_INITIAL_Z(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    additional_vars->initial_move = additional_vars->z_min_extr_pos - additional_vars->z_start;

    if (_was_move(additional_vars)) {
        marlin_gcode_printf("G0 Z%f", (double)additional_vars->z_min_extr_pos); //move to start pos
        additional_vars->z_start = additional_vars->z_min_extr_pos; //set new start pos
    }
    p_vars->phase++;
    return 0;
}

int f_SH_WAIT_INITIAL_Z_MOTION(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    if (_was_move(additional_vars)) {
        //did move
        //wait Z motion
        if (p_vars->p_marlin_vars->motion & MARLIN_VAR_MOTION_MSK_Z)
            p_vars->phase++;
    } else {
        //no move, skip this part
        p_vars->phase++;
    }
    return 0;
}

int f_SH_WAIT_INITIAL_Z_STOPPED(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    if (_was_move(additional_vars)) {
        //did move, wait Z stopped
        if ((p_vars->p_marlin_vars->motion & MARLIN_VAR_MOTION_MSK_Z) == 0)
            p_vars->phase++;

        return 100.F * (p_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_Z] - additional_vars->z_start) / additional_vars->initial_move;
    } else {
        //no move, skip this part
        p_vars->phase++;
    }
    return 0;
}

int f_SH_WAIT_E_MOTION(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    //wait E motion
    if (p_vars->p_marlin_vars->motion & MARLIN_VAR_MOTION_MSK_E)
        p_vars->phase++;
    return 0;
}

int f_SH_WAIT_E_STOPPED(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    //wait Z stopped
    if ((p_vars->p_marlin_vars->motion & MARLIN_VAR_MOTION_MSK_E) == 0)
        p_vars->phase++;
    return 0;
}

int f_SH_WAIT_TEMP(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    //wait E temp
    float min_temp = p_vars->p_marlin_vars->target_nozzle - 10.0F;
    float diff_temp = p_vars->p_marlin_vars->temp_nozzle - min_temp;
    if (diff_temp > 0) {
        p_vars->phase++;
        return 100;
    } else {
        //200 degrrees diff == 0%
        diff_temp = 100.0F + diff_temp / 2;
        if (diff_temp > 99.0F)
            return 99;
        if (diff_temp < 0.0F)
            return 0;
        return (int)diff_temp;
    }
}

int f_SH_WAIT_E_MOTION__WAIT_TEMP(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    //wait E motion
    if (p_vars->p_marlin_vars->motion & MARLIN_VAR_MOTION_MSK_E)
        p_vars->phase++;
    return 100 * (HAL_GetTick() - p_vars->tick_part_start) / 10000;
}

int f_SH_WAIT_READY(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    //wait ready
    if (!marlin_busy())
        p_vars->phase = -1;
    return 0;
}
