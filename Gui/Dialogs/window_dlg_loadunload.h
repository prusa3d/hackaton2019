// window_dlg_loadunload.h

#ifndef _WINDOW_DLG_LOADUNLOAD_H
#define _WINDOW_DLG_LOADUNLOAD_H

#include "window.h"
#include "marlin_client.h"//marlin_vars_t

typedef struct _window_dlg_loadunload_t window_dlg_loadunload_t;

extern int16_t WINDOW_CLS_DLG_LOADUNLOAD;

typedef enum
{
	LD_OK,
	LD_ABORTED//,
	//LD_ERROR
}
ld_result_t;

#define LD_BT_FLG ((uint8_t)(1 << 0))//button flag
#define LD_CH_CMD ((uint8_t)(1 << 1))//check marlin_command()
#define LD_DI_PRS ((uint8_t)(1 << 2))//disable progress

//flags for draw_cb function (user callback)
#define LD_DI_US0 ((uint8_t)(1 << 4))//user flag 0
#define LD_DI_US1 ((uint8_t)(1 << 5))//user flag 1
#define LD_DI_US2 ((uint8_t)(1 << 6))//user flag 2
#define LD_DI_US3 ((uint8_t)(1 << 7))//user flag 3



typedef void (window_draw_dlg_cb_t)(window_dlg_loadunload_t* window);
//this type does not match to window_event_t .. p_event is pointer
typedef void (window_event_dlg_cb_t)(window_dlg_loadunload_t* window, uint8_t *p_event, void* param);

#pragma pack(push)
#pragma pack(1)

//load unload specified vars
typedef struct
{
	float z_min_extr_pos;//minimal z position for extruding
	float initial_move;
	float z_start;
	float e_last;//todo use me more
}_dlg_ld_vars;


//universal vars
typedef struct
{
	uint8_t flags;
	int8_t phase;
	int8_t prev_phase;
	uint8_t progress;
	uint8_t prev_progress;
	uint8_t part_progress;
	uint8_t base_progress;
	uint32_t tick_part_start;
	uint32_t time_total;
	marlin_vars_t* p_marlin_vars;
	window_draw_dlg_cb_t*  draw_cb;    // called at the end of draw (if not NULL)
	window_event_dlg_cb_t* event_cb;   // called at the begin of event (if not NULL)
}_dlg_vars;

typedef int (*dlg_state_func)(_dlg_vars* p_vars, void* p_additional_vars);
typedef void (*dlg_cb_t)(void);//dialog callcack

typedef struct
{
	const char*    label;
	uint8_t        flags;
}_dlg_button_t;

typedef struct
{
	uint32_t             time;
	const char*          text;
	const _dlg_button_t* p_button;
	dlg_state_func       state_fnc;
}_dlg_state;


typedef struct
{
	const char*        title;
	const _dlg_state*  p_states;
	const size_t	   count;
	const dlg_cb_t     on_timeout;
	const dlg_cb_t     on_done;
}_cl_dlg;

typedef struct _window_dlg_loadunload_t
{
	window_t win;
	color_t color_back;
	color_t color_text;
	font_t* font;
	font_t* font_title;
	padding_ui8_t padding;
	uint16_t flags;

	const _cl_dlg * _ths;
	_dlg_vars vars;
} window_dlg_loadunload_t;

typedef struct _window_class_dlg_loadunload_t
{
	window_class_t cls;
} window_class_dlg_loadunload_t;


#pragma pack(pop)


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


extern const window_class_dlg_loadunload_t window_class_dlg_loadunload;


extern ld_result_t gui_dlg_load(void);

extern ld_result_t gui_dlg_load_forced(void);//no return option

extern ld_result_t gui_dlg_purge(void);

extern ld_result_t gui_dlg_unload(void);

extern ld_result_t gui_dlg_unload_forced(void);//no return option + no skipping preheat

#ifdef __cplusplus
}
#endif //__cplusplus


#endif //_WINDOW_DLG_LOADUNLOAD_H
