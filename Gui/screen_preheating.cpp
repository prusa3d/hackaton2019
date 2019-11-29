/*
 * screen_load_filament.cpp
 *
 *  Created on: 19. 7. 2019
 *      Author: mcbig
 */


#include "gui.h"
#include "window_header.h"
#include "status_footer.h"
#include "filament.h"
#include "config.h"
#include "dbg.h"
#include "marlin_client.h"

#include "../Marlin/src/module/temperature.h"
#include "../Marlin/src/gcode/queue.h"
#include "../Marlin/src/lcd/extensible_ui/ui_api.h"

extern uint8_t menu_preheat_type;

#define BUTTON_CANCEL	1

#pragma pack(push)
#pragma pack(1)

typedef struct
{
	window_frame_t root;
	window_header_t header;

	window_text_t w_progress_label;
	window_progress_t w_progress;
	window_text_t w_cancel;

	uint32_t last_timer_repaint;
	status_footer_t footer;

	uint8_t exit;
} screen_preheating_data_t;

#pragma pack(pop)

#define pd ((screen_preheating_data_t*)screen->pdata)

uint8_t screen_preheating_timer(screen_t* screen, uint32_t mseconds);
uint8_t screen_preheating_update_progress(screen_t* screen);

void screen_preheating_init(screen_t* screen)
{
	pd->exit = 0;
	int16_t id;

	int16_t root = window_create_ptr(WINDOW_CLS_FRAME, -1,
			rect_ui16(0,0,0,0), &(pd->root));

	id = window_create_ptr(WINDOW_CLS_HEADER, root,
			rect_ui16(0,0,240,31), &(pd->header));
	p_window_header_set_icon(&(pd->header), IDR_PNG_status_icon_home);
	p_window_header_set_text(&(pd->header), "PREHEATING");

	id = window_create_ptr(WINDOW_CLS_TEXT, root,
			rect_ui16(10, 33, 220, 29),
			&(pd->w_progress_label));
	pd->w_progress_label.font = resource_font(IDR_FNT_BIG);
	window_set_padding(id, padding_ui8(0,0,0,0));
	window_set_alignment(id, ALIGN_CENTER_BOTTOM);
	window_set_text(id, "Preheating ...");

	id = window_create_ptr(WINDOW_CLS_PROGRESS, root,
			rect_ui16(10, 70, 220, 50),
			&(pd->w_progress));
	pd->w_progress.color_progress = COLOR_ORANGE;
	pd->w_progress.font=resource_font(IDR_FNT_BIG);
	pd->w_progress.height_progress = 14;

	id = window_create_ptr(
			WINDOW_CLS_TEXT, root,
			rect_ui16(10, 227, 220, 30),
			&(pd->w_cancel));
	pd->w_cancel.font = resource_font(IDR_FNT_NORMAL);
	window_set_alignment(id, ALIGN_CENTER);
	window_set_text(id, "CANCEL");
	window_enable(id);
	window_set_tag(id, BUTTON_CANCEL);

	status_footer_init(&(pd->footer), root);
}

void screen_preheating_done(screen_t* screen)
{
	window_destroy(pd->root.win.id);
}

void screen_preheating_draw(screen_t* screen)
{
}

int screen_preheating_event(screen_t* screen, window_t* window,
		uint8_t event, void* param)
{
	if (screen_preheating_timer(screen, (HAL_GetTick()/50)*50)){
		return 1;
	}
	status_footer_event(&(pd->footer), window, event, param);

	if (event == WINDOW_EVENT_CLICK &&
			(int)param == BUTTON_CANCEL &&
			(pd->w_cancel.win.flg & WINDOW_FLG_VISIBLE))
	{
		thermalManager.setTargetHotend(0, 0);
		thermalManager.setTargetBed(0);
		screen_close();
		return 1;
	}

	return 0;
}

uint8_t screen_preheating_timer(screen_t* screen, uint32_t mseconds)
{
	uint8_t rv = 0;
	if ( ( (mseconds - pd->last_timer_repaint) >= 1000 ) && (pd->w_cancel.win.flg & WINDOW_FLG_VISIBLE)){
		if (screen_preheating_update_progress(screen)){
			window_disable(pd->w_cancel.win.id);
			window_hide(pd->w_cancel.win.id);
			window_hide(pd->w_progress.win.id);

			if (menu_preheat_type == 1){
				if (gui_load_filament(&(pd->w_progress), &(pd->w_progress_label), 1)){
					screen_close();		// return to home screen
				}
			} else if (menu_preheat_type == 2){
				gui_unload_filament(&(pd->w_progress), &(pd->w_progress_label),1);
				set_filament(FILAMENT_NONE);
			}

			screen_close();		// filament was (un)loaded
			rv = 1;
		}
		pd->last_timer_repaint = mseconds;
	}

	/*
	if (mseconds % 500 == 0 && wait_for_user){
		_dbg3("wait_for_user is true ...");
		// sometimes marlin wait for user whern thermal is heating...
		wait_for_user = false;
		return 0;	// skip next test
	}
	*/
	return rv;
}

uint8_t screen_preheating_update_progress(screen_t* screen)
{
	float percent = thermalManager.degHotend(0)/thermalManager.degTargetHotend(0)*100;
	window_set_value(pd->w_progress.win.id, percent);
	return (percent >= 95) ? 1 : 0;
}

screen_t screen_preheating =
{
		0,
		0,
		screen_preheating_init,
		screen_preheating_done,
		screen_preheating_draw,
		screen_preheating_event,
		sizeof(screen_preheating_data_t), //data_size
		0, //pdata
};

const screen_t* pscreen_preheating = &screen_preheating;
