// window_dlg_unload.c
// does not have header
// window_dlg_loadunload.h used instead

#include "window_dlg_loadunload.h"
#include "marlin_client.h"
#include "menu_vars.h"
#include "stm32f4xx_hal.h"
#include <limits.h>
#include "window_dlg_preheat.h"
#include "window_dlg_loadunload_shared.h"

static const _cl_dlg cl_unload;

ld_result_t _gui_dlg_unload(void)
{
    _dlg_ld_vars ld_vars;
    memset(&ld_vars, '\0', sizeof(ld_vars));
    ld_vars.z_min_extr_pos = 10;
    return _gui_dlg(&cl_unload, &ld_vars, 300000); //5min
}

ld_result_t gui_dlg_unload(void)
{
    //todo must be called inside _gui_dlg, but nested dialogs are not supported now

    marlin_vars_t* p_vars = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_TTEM_NOZ));
    if (p_vars->target_nozzle < 190) {
        if (gui_dlg_preheat(NULL) < 1)
            return LD_ABORTED; //0 is return
    }
    return _gui_dlg_unload();
}

ld_result_t gui_dlg_unload_forced(void)
{
    //todo must be called inside _gui_dlg, but nested dialogs are not supported now
    if (gui_dlg_preheat_forced("PREHEAT for UNLOAD") < 0)
        return LD_ABORTED; //LD_ABORTED should not happen
    return _gui_dlg_unload();
}

/*****************************************************************************/
//UNLOAD

static int f_UL_GCODE(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    marlin_gcode("M702 Z0");
    p_vars->phase++;
    p_vars->flags |= LD_CH_CMD; //kill screen after M702 ends
    return 0;
}

//ram sequence .. kinda glitchy
// 0 ... -8  ... 8 ... -392
static int f_UL_WAIT_E_POS__RAM_RETRACTING(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    // 0 ... -8
    if ( //todo unprecise
        (p_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] < -7.9F) || (p_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] > 0.0F) //this part should not happen, just to be safe
    )
        p_vars->phase++;

    float ret = 100.0F * (-p_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E]) / 8;
    if (ret >= 100)
        return 100;
    if (ret <= 0)
        return 0;
    return ret;
}

static int f_UL_WAIT_E_POS__RAMMING(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    // -8  ... 8
    if ( //todo unprecise
        p_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] > 4.0F
        //(p_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] > 7.9F) ||
        //(p_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] < -8.1F) //this part should not happen, just to be safe
    )
        p_vars->phase++;

    float ret = 100.0F * (8.0F + p_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E]) / (8 + 8);

    if (ret >= 100)
        return 100;
    if (ret <= 0)
        return 0;
    return ret;
}

static int f_UL_WAIT_E_POS__UNLOADING(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars)
{
    // 8 ... -392
    if (p_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] < -391.9F)
        p_vars->phase++;
    return 100 * (8.0F - p_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E]) / (392 + 8);
}

/*****************************************************************************/
//UNLOAD
//name of func serves as comment
static const _dlg_state unload_states[] = {
    { 0, "Parking", &bt_stop_ena, (dlg_state_func)f_SH_INIT },
    { 0, "Parking", &bt_stop_ena, (dlg_state_func)f_SH_MOVE_INITIAL_Z },
    { 0, "Parking", &bt_stop_ena, (dlg_state_func)f_SH_WAIT_INITIAL_Z_MOTION },
    { 1000, "Parking", &bt_stop_ena, (dlg_state_func)f_SH_WAIT_INITIAL_Z_STOPPED },
    { 10000, "Waiting for temp.", &bt_stop_ena, (dlg_state_func)f_SH_WAIT_TEMP },
    { 0, "Waiting for temp.", &bt_stop_dis, (dlg_state_func)f_UL_GCODE },

    { 1500, "Preparing to ram", &bt_stop_dis, (dlg_state_func)f_UL_WAIT_E_POS__RAM_RETRACTING },
    { 1500, "Ramming", &bt_stop_dis, (dlg_state_func)f_UL_WAIT_E_POS__RAMMING },
    { 10000, "Unloading", &bt_stop_dis, (dlg_state_func)f_UL_WAIT_E_POS__UNLOADING },

    { 0, "Unloading", &bt_stop_dis, (dlg_state_func)f_SH_WAIT_E_STOPPED },
    { 0, "Finished", &bt_cont_ena, (dlg_state_func)f_SH_WAIT_READY }
};

static const _cl_dlg cl_unload = {
    "Unloading filament",
    unload_states,
    sizeof(unload_states) / sizeof(unload_states[0]),
    NULL, //on_timeout
    NULL, //on_done
};
