// window_dlg_loadunload_shared.h

#ifndef _WINDOW_DLG_LOADUNLOAD_SHARED_H
#define _WINDOW_DLG_LOADUNLOAD_SHARED_H

extern ld_result_t _gui_dlg(const _cl_dlg * _ths, void* p_additional_vars, int32_t ttl);

/*****************************************************************************/
//shared for LOAD and UNLOAD

//button flags
//combination of enabled and not visible  == do not clear
#define BT_ENABLED  ( (uint8_t) (1 << 0) )
#define BT_VISIBLE  ( (uint8_t) (1 << 1) )
#define BT_AUTOEXIT ( (uint8_t) (1 << 2) )

#define LD_BT_DONE       LD_DI_US0 //continue button for marlin
#define LD_BT_PURG       LD_DI_US1 //resume   button for marlin
#define LD_BT_DONE_NPURG LD_DI_US2 //active button is done, inactive button is purge

extern const _dlg_button_t bt_stop_ena;
extern const _dlg_button_t bt_stop_dis;
extern const _dlg_button_t bt_cont_ena;
extern const _dlg_button_t bt_cont_dis;
extern const _dlg_button_t bt_none;
extern const _dlg_button_t bt_dont_clr;

extern int f_SH_INIT(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars);
extern int f_SH_MOVE_INITIAL_Z(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars);
extern int f_SH_WAIT_INITIAL_Z_MOTION(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars);
extern int f_SH_WAIT_INITIAL_Z_STOPPED(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars);
extern int f_SH_WAIT_E_MOTION(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars);
extern int f_SH_WAIT_E_STOPPED(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars);
extern int f_SH_WAIT_TEMP(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars);

extern int f_SH_WAIT_READY(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars);

extern int f_LD_REINIT_TO_PURGE(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars);
extern int f_LD_REINIT_FROM_PURGE(_dlg_vars* p_vars, _dlg_ld_vars* additional_vars);

#endif //_WINDOW_DLG_LOADUNLOAD_SHARED_H
