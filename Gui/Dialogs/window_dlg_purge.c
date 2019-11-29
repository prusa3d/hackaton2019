// window_dlg_purge.c
// does not have header
// window_dlg_loadunload.h used instead

#include "window_dlg_loadunload.h"
#include "marlin_client.h"
#include "menu_vars.h"
#include "stm32f4xx_hal.h"
#include <limits.h>
#include "window_dlg_preheat.h"
#include "window_dlg_loadunload_shared.h"

extern const float ld_purge_amount;

static const _cl_dlg cl_purge;

ld_result_t _gui_dlg_purge(void)
{
    _dlg_ld_vars ld_vars;
    memset(&ld_vars, '\0', sizeof(ld_vars));
    ld_vars.z_min_extr_pos = 30;
    return _gui_dlg(&cl_purge, &ld_vars, 600000); //10min
}

ld_result_t gui_dlg_purge(void)
{
    //todo must be called inside
    if (gui_dlg_preheat(NULL) < 1)
        return LD_ABORTED; //0 is return
    return _gui_dlg_purge();
}

/*****************************************************************************/
//PURGE
//name of func serves as comment
static int f_PU_GCODE(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    marlin_gcode("M83"); //extruder relative mode
    marlin_gcode("G21"); //set units to millimeters
    p_vars->phase++;
    return 0;
}

static int f_PU_PURGE_USER_INTERACTION(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    switch (p_vars->flags & (LD_BT_PURG | LD_BT_DONE)) {
    case LD_BT_PURG:
        additional_vars->e_last = p_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E]; //needed in next phase
        marlin_gcode("G1 E40 F200");
        p_vars->phase++; //f_LD_PURGE_SHOW_PROGRESS
        break;
    case LD_BT_DONE:
    case (LD_BT_DONE | LD_BT_PURG): //if both buttons are clicked DONE has priority, but should not happen
        p_vars->phase += 2; //f_LD_REINIT_FROM_PURGE
        break;
    }
    return 100; //progressbar MUST be 100
}

static int f_PU_PURGE_SHOW_PROGRESS(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    float ret = 100.0F * (p_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] - additional_vars->e_last)
        / ld_purge_amount;
    if (ret > 99.5F) {
        p_vars->phase--; //jump back to f_LD_PURGE_USER_INTERACTION
        p_vars->flags &= (~(LD_BT_PURG | LD_BT_DONE | LD_BT_DONE_NPURG)); //clr buttons
        return 100;
    }
    return (int)ret;
}

static const _dlg_state purge_states[] = {
    { 0, "Parking", &bt_stop_ena, (dlg_state_func)f_SH_INIT },
    { 0, "Parking", &bt_stop_ena, (dlg_state_func)f_SH_MOVE_INITIAL_Z },
    { 0, "Parking", &bt_stop_ena, (dlg_state_func)f_SH_WAIT_INITIAL_Z_MOTION },
    { 3000, "Parking", &bt_stop_ena, (dlg_state_func)f_SH_WAIT_INITIAL_Z_STOPPED },
    { 10000, "Waiting for temp.", &bt_stop_ena, (dlg_state_func)f_SH_WAIT_TEMP },
    //from now all times must be zero, draw_cb handles progress bar differently
    { 0, "Purging", &bt_none, (dlg_state_func)f_PU_GCODE },
    { 0, "Purging", &bt_none, (dlg_state_func)f_LD_REINIT_TO_PURGE },
    { 0, "Purging", &bt_dont_clr, (dlg_state_func)f_PU_PURGE_USER_INTERACTION }, //can end (state += 2)
    { 0, "Purging", &bt_none, (dlg_state_func)f_PU_PURGE_SHOW_PROGRESS }, //can jump back (state --)
    { 0, "Finished", &bt_none, (dlg_state_func)f_LD_REINIT_FROM_PURGE },
    { 0, "Finished", &bt_none, (dlg_state_func)f_SH_WAIT_READY }
};

static void purge_done(void)
{
    marlin_gcode("M84"); // disable motors
}

static const _cl_dlg cl_purge = {
    "Purge nozzle",
    purge_states,
    sizeof(purge_states) / sizeof(purge_states[0]),
    NULL, //on_timeout
    purge_done, //on_done
};
