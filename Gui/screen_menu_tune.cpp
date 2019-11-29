// menu_tune.cpp

#include "gui.h"
#include "screen_menu.h"
#include "marlin_client.h"
#include "filament.h"
#include "menu_vars.h"


extern screen_t screen_test;
extern screen_t screen_menu_info;
extern screen_t screen_messages;


enum
{
	MI_RETURN,
	MI_SPEED,
	MI_NOZZLE,
	MI_HEATBED,
	MI_PRINTFAN,
	MI_FLOWFACT,
	MI_BABYSTEP,
	MI_FILAMENT,
	MI_INFO,
	MI_TEST,
	MI_MESSAGES,
};

//cannot use .wi_spin = { 0, feedrate_range } ...
//sorry, unimplemented: non-trivial designated initializers not supported
const menu_item_t _menu_tune_items[] =
{
	{{"Speed",           0, WI_SPIN   }, SCREEN_MENU_NO_SCREEN},//set later
	{{"Nozzle",          0, WI_SPIN   }, SCREEN_MENU_NO_SCREEN},//set later
	{{"HeatBed",         0, WI_SPIN   }, SCREEN_MENU_NO_SCREEN},//set later
	{{"Fan Speed",       0, WI_SPIN   }, SCREEN_MENU_NO_SCREEN},//set later
	{{"Flow Factor",     0, WI_SPIN   }, SCREEN_MENU_NO_SCREEN},//set later
	{{"Live Adjust Z",   0, WI_SPIN_FL}, SCREEN_MENU_NO_SCREEN},//set later
	{{"Change Filament", 0, WI_LABEL  }, SCREEN_MENU_NO_SCREEN},
	{{"Info",            0, WI_LABEL  }, &screen_menu_info},
	{{"Test",            0, WI_LABEL  }, &screen_test},
	{{"Messages",        0, WI_LABEL  }, &screen_messages},
};


void screen_menu_tune_timer(screen_t* screen, uint32_t mseconds);
void screen_menu_tune_chanege_filament(screen_t * screen);

void screen_menu_tune_init(screen_t* screen)
{
	marlin_vars_t* vars;
	int count = sizeof(_menu_tune_items) / sizeof(menu_item_t);
	screen_menu_init(screen, "TUNE", count + 1, 1, 0);
	psmd->items[MI_RETURN] = menu_item_return;
	memcpy(psmd->items + 1, _menu_tune_items, count * sizeof(menu_item_t));

	vars = marlin_update_vars(
			MARLIN_VAR_MSK_TEMP_TARG |
			MARLIN_VAR_MSK(MARLIN_VAR_Z_OFFSET) |
			MARLIN_VAR_MSK(MARLIN_VAR_FANSPEED) |
			MARLIN_VAR_MSK(MARLIN_VAR_PRNSPEED) |
			MARLIN_VAR_MSK(MARLIN_VAR_FLOWFACT)
		);

	psmd->items[MI_SPEED].item.wi_spin.value = (int32_t)(vars->print_speed * 1000);
	psmd->items[MI_SPEED].item.wi_spin.range = feedrate_range;

	psmd->items[MI_NOZZLE].item.wi_spin.value = (int32_t)(vars->target_nozzle * 1000);
	psmd->items[MI_NOZZLE].item.wi_spin.range = nozzle_range;

	psmd->items[MI_HEATBED].item.wi_spin.value = (int32_t)(vars->target_bed * 1000);
	psmd->items[MI_HEATBED].item.wi_spin.range = heatbed_range;

	psmd->items[MI_PRINTFAN].item.wi_spin.value = (int32_t)(vars->fan_speed * 1000);
	psmd->items[MI_PRINTFAN].item.wi_spin.range = printfan_range;

	psmd->items[MI_FLOWFACT].item.wi_spin.value = (int32_t)(vars->flow_factor * 1000);
	psmd->items[MI_FLOWFACT].item.wi_spin.range = flowfact_range;

	psmd->items[MI_BABYSTEP].item.wi_spin_fl.value = vars->z_offset;
	psmd->items[MI_BABYSTEP].item.wi_spin_fl.range = zoffset_fl_range;
	psmd->items[MI_BABYSTEP].item.wi_spin_fl.prt_format = zoffset_fl_format;
}

int screen_menu_tune_event(screen_t* screen, window_t* window,
		uint8_t event, void* param)
{
	static float z_offs = 0;

	if (screen_menu_event(screen, window, event, param)) return 1;

	if (event == WINDOW_EVENT_CHANGING)
	{
		switch ((int)param)
		{
		/*case MI_SPEED:
			marlin_set_print_speed((uint8_t)(psmd->items[MI_SPEED].item.value / 1000));
			break;
		case MI_NOZZLE:
			marlin_set_target_nozzle((float)(psmd->items[MI_NOZZLE].item.value) / 1000);
			break;
		case MI_HEATBED:
			marlin_set_target_bed((float)(psmd->items[MI_HEATBED].item.value) / 1000);
			break;
		case MI_PRINTFAN:
			marlin_set_fan_speed((uint8_t)(psmd->items[MI_PRINTFAN].item.value / 1000));
			break;
		case MI_FLOWFACT:
			marlin_set_flow_factor((uint16_t)(psmd->items[MI_FLOWFACT].item.value / 1000));
			break;*/
		case MI_BABYSTEP:
			marlin_do_babysteps_Z(psmd->items[MI_BABYSTEP].item.wi_spin_fl.value - z_offs);
			z_offs = psmd->items[MI_BABYSTEP].item.wi_spin_fl.value;
			break;
		}
	}
	else if (event == WINDOW_EVENT_CHANGE)
	{
		switch((int)param)
		{
		case MI_SPEED:
			marlin_set_print_speed((uint8_t)(psmd->items[MI_SPEED].item.wi_spin.value / 1000));
			break;
		case MI_NOZZLE:
			marlin_set_target_nozzle((float)(psmd->items[MI_NOZZLE].item.wi_spin.value) / 1000);
			break;
		case MI_HEATBED:
			marlin_set_target_bed((float)(psmd->items[MI_HEATBED].item.wi_spin.value) / 1000);
			break;
		case MI_PRINTFAN:
			marlin_set_fan_speed((uint8_t)(psmd->items[MI_PRINTFAN].item.wi_spin.value / 1000));
			break;
		case MI_FLOWFACT:
			marlin_set_flow_factor((uint16_t)(psmd->items[MI_FLOWFACT].item.wi_spin.value / 1000));
			break;
		case MI_BABYSTEP:
			marlin_set_z_offset(psmd->items[MI_BABYSTEP].item.wi_spin_fl.value);
			marlin_settings_save();
			break;
		}
	}
	else if (event == WINDOW_EVENT_CLICK)
	{
		switch ((int)param)
		{
		case MI_FILAMENT:
			screen_menu_tune_chanege_filament(screen);
			break;
		case MI_BABYSTEP:
			z_offs = psmd->items[MI_BABYSTEP].item.wi_spin_fl.value;
			break;
		case MI_MESSAGES:
			screen_open(psmd->items[(int)param].screen->id);
			break;
		}
	}

	return 0;
}

void screen_menu_tune_timer(screen_t* screen, uint32_t mseconds)
{
	static uint32_t last_timer_repaint = 0;
	// if (psmd->pfooter->last_timer == mseconds) return; // WTF is footer timer doing here?
	//if (mseconds % 500 == 0) // is this really necessary? That's why the tune menu behaves like a Ferrari off-road (i.e. sitting stuck :) )
	if( ( mseconds - last_timer_repaint ) >= 500 ){
		marlin_vars_t* vars = marlin_vars();
		bool editing = psmd->menu.mode > 0;
		int index = psmd->menu.index;
		if (!editing || index != MI_SPEED)
		{
			psmd->items[MI_SPEED].item.wi_spin.value = vars->print_speed * 1000;
			psmd->menu.win.flg |= WINDOW_FLG_INVALID;
		}
		if (!editing || index != MI_NOZZLE)
		{
			psmd->items[MI_NOZZLE].item.wi_spin.value = vars->target_nozzle * 1000;
			psmd->menu.win.flg |= WINDOW_FLG_INVALID;
		}
		if (!editing || index != MI_HEATBED)
		{
			psmd->items[MI_HEATBED].item.wi_spin.value = vars->target_bed * 1000;
			psmd->menu.win.flg |= WINDOW_FLG_INVALID;
		}
		if (!editing || index != MI_PRINTFAN)
		{
			psmd->items[MI_PRINTFAN].item.wi_spin.value = vars->fan_speed * 1000;
			psmd->menu.win.flg |= WINDOW_FLG_INVALID;
		}
		if (!editing || index != MI_FLOWFACT)
		{
			psmd->items[MI_FLOWFACT].item.wi_spin.value = vars->flow_factor * 1000;
			psmd->menu.win.flg |= WINDOW_FLG_INVALID;
		}
		last_timer_repaint = mseconds;
	}
}

//TODO: remove dependency and following code
#include "../Marlin/src/libs/nozzle.h"
#include "../Marlin/src/module/planner.h"
#include "../Marlin/src/sd/cardreader.h"
#include "../Marlin/src/module/temperature.h"
#include "../Marlin/src/lcd/extensible_ui/ui_api.h"

void screen_menu_tune_chanege_filament(screen_t * screen)
{
	float rescue_z_position;
	float rescue_feedrate;

	card.pauseSDPrint();					// pause the print
	rescue_feedrate = feedrate_mm_s*60;

	// print_job_timer.pause();				// print_job_timer stop gcode queue
	retract_filament_gcode(5);

	planner.synchronize();
	rescue_z_position = current_position[Z_AXIS];
	Nozzle::park(2, NOZZLE_PARK_POINT);

	retract_filament_gcode(-5);				// return filament to nozzle, to right filament arrow

	HAL_Delay(500);

	while(ExtUI::isMoving()) {		// Wait for parking
		HAL_Delay(100);
	}

	window_text_t w_progress_label;
	window_progress_t w_progress;
	window_hide(psmd->menu.win.id);
	window_enable(psmd->root.win.id);
	window_set_capture(psmd->root.win.id);
	window_set_focus(psmd->root.win.id);
	window_invalidate(psmd->root.win.id);

	int16_t id;
	id = window_create_ptr(WINDOW_CLS_TEXT, psmd->root.win.id,
			rect_ui16(10, 33, 220, 29),
			&w_progress_label);
	w_progress_label.font = resource_font(IDR_FNT_BIG);
	window_set_padding(id, padding_ui8(0,0,0,0));
	window_set_alignment(id, ALIGN_CENTER_BOTTOM);

	id = window_create_ptr(WINDOW_CLS_PROGRESS, psmd->root.win.id,
			rect_ui16(10, 70, 220, 50),
			&w_progress);
	w_progress.color_progress = COLOR_ORANGE;
	w_progress.font=resource_font(IDR_FNT_BIG);
	w_progress.height_progress = 14;

	gui_change_filament(&w_progress, &w_progress_label);
	gui_reset_jogwheel();

	window_destroy(w_progress.win.id);
	window_destroy(w_progress_label.win.id);
	window_show(psmd->menu.win.id);

	window_disable(psmd->root.win.id);
	window_set_capture(psmd->menu.win.id);
	window_set_focus(psmd->menu.win.id);
	window_invalidate(psmd->root.win.id);

	flush_filamnet_gcode();					// flush before starting print

	marlin_gcode("G1 F4000");		// fix the feedrate
	marlin_gcode("M24");			// return only on X,Y :-(
	HAL_Delay(500);

	// return to Z too
	uint8_t relative = get_relative(Z_AXIS);
	if (relative){
		marlin_gcode("G90");
	}

	marlin_gcode_printf("G0 Z%.4f", (double)rescue_z_position);
	marlin_gcode_printf("G0 F%.4f", (double)rescue_feedrate);
	if (relative){
		marlin_gcode("G91");
	}

	// card.startFileprint();				// called from M24
	// print_job_timer.start();				// not needed, because not stopped
}

screen_t screen_menu_tune =
{
		0,
		0,
		screen_menu_tune_init,
		screen_menu_done,
		screen_menu_draw,
		screen_menu_tune_event,
		sizeof(screen_menu_data_t), //data_size
		0, //pdata
};

const screen_t* pscreen_menu_tune = &screen_menu_tune;
