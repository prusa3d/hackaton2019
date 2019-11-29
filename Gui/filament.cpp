/*
 * filament.c
 *
 *  Created on: 19. 7. 2019
 *      Author: mcbig
 */
#include "new_eeprom.h"
#include "st25dv64k.h"
#include "assert.h"
#include "dbg.h"
#include "marlin_client.h"
#include "filament.h"
#include "gui.h"

#include "../Marlin/src/gcode/gcode.h"
#include "../Marlin/src/module/planner.h"
#include "../Marlin/src/lcd/extensible_ui/ui_api.h"

//fixme generating long names, takes too long
const filament_t filaments[FILAMENTS_END] = {
		{"---"  ,"---", 0, 0},
		{"PLA"  ,"PLA      215/ 60", 215, 60},
		{"PET-G","PET-G    230/ 85", 230, 85},
		{"ASA"  ,"ASA      260/100", 260, 100},
		{"FLEX" ,"FLEX     240/ 50", 240, 50},
};

#define FILAMENT_ADDRESS 	0x400

static FILAMENT_t filament_selected = FILAMENTS_END;

void set_filament(FILAMENT_t filament){
	assert(filament < FILAMENTS_END);
	if (filament == filament_selected){
		return;
	}
	filament_selected = filament;
	st25dv64k_user_write(FILAMENT_ADDRESS, filament);
}

FILAMENT_t get_filament()
{
	if (filament_selected == FILAMENTS_END){
		uint8_t fil = st25dv64k_user_read(FILAMENT_ADDRESS);
		if (fil >= FILAMENTS_END) fil = 0;
		filament_selected = (FILAMENT_t)fil;
	}
	return filament_selected;
}

uint8_t get_relative(uint8_t axis){
    return GcodeSuite::axis_is_relative((AxisEnum)axis);
}

void retract_filament_gcode(int length)
{
    bool relative = GcodeSuite::axis_is_relative(E_AXIS);
	if (relative){
		marlin_gcode("M82");
	}
	marlin_gcode_printf("G0 E%d F500", -length);
	marlin_gcode("G92 E0");
	if (relative){
		marlin_gcode("M83");
	}
}

uint32_t unload_filament_gcode()
{
	bool relative = GcodeSuite::axis_is_relative(E_AXIS);
	if (relative){
		marlin_gcode("M82");
	}
	marlin_gcode("G0 E-2 F1000");			// 2/1000
	marlin_gcode("G0 E-1 F500");			// 1/500
	marlin_gcode("G0 E-10 F200");			// 9/200
	marlin_gcode("G0 E-30 F500");			// 20/500
	marlin_gcode("G0 E-400 F2000");			// 370/1000
	marlin_gcode("G92 E0");
	if (relative){
		marlin_gcode("M83");
	}

	return 27500;		// time of this operation
}

uint32_t load_filament_gcode(uint8_t up_z)
{
	bool relative = GcodeSuite::axis_is_relative(E_AXIS);
	if (relative){
		marlin_gcode("M82");
	}
	marlin_gcode("G0 E0");					// after marlin start, this fix the state
	marlin_gcode("G0 E40 F200");			// 40/200
	marlin_gcode("G0 E320 F2000");			// 280/1000
	marlin_gcode("G0 E340 F200");			// 20/200
	if (up_z){
		marlin_gcode("G1 Z20 E360 F200");	// 20/200
	}
	marlin_gcode("G0 E400 F200");			// 40/200
	marlin_gcode("G92 E0");
	if (relative){
		marlin_gcode("M83");
	}
	return 53700;		// time of this operation
}

void flush_filamnet_gcode()
{
	marlin_gcode("G0 E15 F300");
	marlin_gcode("G92 E0");
}

void flush_bowden_gcode()
{
	marlin_gcode("G0 E-50 F1000");
	marlin_gcode("G92 E0");
}

uint8_t gui_load_filament(window_progress_t * w_progress, window_text_t * w_text, uint8_t up_z)
{
	const char* next_btn = "NEXT";

	gui_msgbox_ex(0,
			"Press knob to load filament.",
			MSGBOX_BTN_CUSTOM1,
			rect_ui16(0, 32, 240, 320-96),
			0,
			&next_btn);

	if (w_text) {
		window_set_text(w_text->win.id, "Loading ...");
		window_show(w_text->win.id);
	}
	gui_loop();

	uint32_t time = load_filament_gcode(up_z);	// nozzle is parked - no up_z needed

	osDelay(200);
	if (w_progress)
	{
		uint32_t start = HAL_GetTick();
		w_progress->min = 0;
		w_progress->max = 100;
		window_set_value(w_progress->win.id, 0);
		window_show(w_progress->win.id);
		gui_loop();

		while (ExtUI::isMoving()){
			osDelay(100);
			_dbg("time from start: %d", (double)((HAL_GetTick()-start)/(float)time*100));
			window_set_value(w_progress->win.id, (HAL_GetTick()-start)/(float)time*100);
			gui_loop();		// FIXME: footbar is refreshed in bad time
		}

		window_hide(w_progress->win.id);
	} else {
		while(ExtUI::isMoving()) {		// Wait for load
			osDelay(100);
		}
	}

	if (w_text) {
		window_hide(w_text->win.id);
	}
	gui_loop();

	int ret = MSGBOX_RES_CUSTOM1;
	while (ret != MSGBOX_RES_CUSTOM0)
	{
		const char * btns[3] = {"YES", "NO", "UNLOAD"};
		ret = gui_msgbox_ex(0,
				"The filament extruding "
				"And with the correct color?",
				MSGBOX_BTN_CUSTOM3,
				rect_ui16(0, 32, 240, 320-96),
				0,
				btns);

		_dbg("msgbox result: %d", ret);
		if (ret == MSGBOX_RES_CUSTOM1) {
			flush_filamnet_gcode();		// flush and ask again
		} else if (ret == MSGBOX_RES_CUSTOM2) {
			gui_unload_filament(w_progress, w_text, 0);
			return	0;					// filament is unload
		}
	}
	return 1;	// filament is loaded
}

void gui_unload_filament(window_progress_t * w_progress, window_text_t * w_text, uint8_t ask)
{
	if (ask){
		const char* unload_btn = "UNLOAD";
		gui_msgbox_ex(0,
				"Press knob to unload filament.",
				MSGBOX_BTN_CUSTOM1,
				rect_ui16(0, 32, 240, 320-96),
				0,
				&unload_btn);
	}

	if (w_text) {
		window_set_text(w_text->win.id, "Unloading ...");
		window_show(w_text->win.id);
	}
	gui_loop();

	uint32_t time = unload_filament_gcode();

	osDelay(200);
	if (w_progress)							// not work now :-(
	{
		uint32_t start = HAL_GetTick();
		w_progress->min = 0;
		w_progress->max = 100;
		window_set_value(w_progress->win.id, 0);
		window_show(w_progress->win.id);
		gui_loop();

		while (ExtUI::isMoving()){
			osDelay(100);
			_dbg("time from start: %d", (double)((HAL_GetTick()-start)/(float)time*100));
			window_set_value(w_progress->win.id, (HAL_GetTick()-start)/(float)time*100);
			gui_loop();		// FIXME: footbar is refreshed in bad time
		}

		window_hide(w_progress->win.id);
	} else {
		while(ExtUI::isMoving()) {		// Wait for load
			osDelay(100);
		}
	}

	if (w_text) {
		window_hide(w_text->win.id);
	}
	gui_loop();

	int ret = MSGBOX_RES_NO;
	while (ret != MSGBOX_RES_YES)
	{
		ret = gui_msgbox_ex(0,
				"Is filament fully unloaded?",
				MSGBOX_BTN_YESNO,
				rect_ui16(0, 32, 240, 320-96),
				0,
				0);

		if (ret == MSGBOX_RES_NO) {
			flush_bowden_gcode();		// flush and ask again
		}
	}
}

void gui_change_filament(window_progress_t * w_progress, window_text_t * w_text)
{
	gui_unload_filament(w_progress, w_text, 1);

	while (gui_load_filament(w_progress, w_text, 0) != 1)
	{}
}
