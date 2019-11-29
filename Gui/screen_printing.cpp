/*
 * screen_prusa.c
 *
 *  Created on: 16. 7. 2019
 *      Author: mcbig
 */

#include "dbg.h"
#include "gui.h"
#include "config.h"
#include "window_header.h"
#include "status_footer.h"
#include "marlin_client.h"
#include "filament.h"

#include "ffconf.h"

#include "../Marlin/src/libs/duration_t.h"

#include "../Marlin/src/gcode/lcd/M73_PE.h"


extern screen_t* pscreen_home;
extern screen_t* pscreen_menu_tune;

#define COLOR_VALUE_VALID COLOR_WHITE
//#define COLOR_VALUE_INVALID COLOR_YELLOW
#define COLOR_VALUE_INVALID COLOR_WHITE

#define BUTTON_TUNE			0
#define BUTTON_PAUSE		1
#define BUTTON_STOP			2

typedef enum {
	P_PRINTING=1,
	P_PAUSED=3,
	P_PRINTED=4
} printing_state_t;

#define HEATING_DIFFERENCE 1

const uint16_t printing_icons[6] = {
		IDR_PNG_menu_icon_settings,
		IDR_PNG_menu_icon_pause,
		IDR_PNG_menu_icon_stop,
		IDR_PNG_menu_icon_resume,
		IDR_PNG_menu_icon_reprint,
		IDR_PNG_menu_icon_home,
};

const char * printing_labels[6] = {
		"Tune",
		"Pause",
		"Stop",
		"Resume",
		"Reprint",
		"Home"
};

#pragma pack(push)
#pragma pack(1)

typedef struct
{
	window_frame_t root;

	window_header_t header;
	window_text_t w_filename;
	window_progress_t w_progress;
	window_text_t w_time_label;
	window_text_t w_time_value;
	window_text_t w_etime_label;
	window_text_t w_etime_value;
	// window_text_t w_filament_label;
	// window_text_t w_filament_value;

	window_icon_t w_buttons[3];
	window_text_t w_labels[3];

	printing_state_t state;
	uint32_t last_timer_repaint;

	status_footer_t footer;

	char text_time[13]; 				// 999d 23h 30m\0
	char text_etime[9]; 				// 999X 23Y\0
	char text_filament[5]; 				// 999m\0 | 1.2m\0

} screen_printing_data_t;


#pragma pack(pop)

void screen_printing_init(screen_t* screen);
void screen_printing_done(screen_t* screen);
void screen_printing_draw(screen_t* screen);
int screen_printing_event(screen_t* screen, window_t* window, uint8_t event, void* param);
void screen_printing_timer(screen_t* screen, uint32_t seconds);
void screen_printing_update_progress(screen_t* screen);
void screen_printing_pause_print(screen_t* screen);
void screen_printing_resume_print(screen_t* screen);
void screen_printing_reprint(screen_t* screen);
void screen_printing_printed(screen_t* screen);
void screen_mesh_err_stop_print(screen_t* screen);

screen_t screen_printing =
{
		0,
		0,
		screen_printing_init,
		screen_printing_done,
		screen_printing_draw,
		screen_printing_event,
		sizeof(screen_printing_data_t), //data_size
		0, //pdata
};
const screen_t* pscreen_printing = &screen_printing;

//TODO: rework this, save memory
char screen_printing_file_name[_MAX_LFN] = {'\0'};
char screen_printing_file_path[_MAX_LFN] = {'\0'};

#define pw ((screen_printing_data_t*)screen->pdata)


struct pduration_t: duration_t {
	pduration_t(): pduration_t(0) {};

	pduration_t(uint32_t const &seconds):duration_t(seconds) {}

	void to_string(char *buffer) const {
	    int d = this->day(),
	        h = this->hour() % 24,
	        m = this->minute() % 60,
	        s = this->second() % 60;

	    if (d) {
	    	sprintf(buffer, "%i3d %2ih %2im", d, h, m);
	    } else if (h){
	    	sprintf(buffer, "     %2ih %2im", h, m);
	    } else if (m){
	    	sprintf(buffer, "     %2im %2is", m, s);
	    } else {
	    	sprintf(buffer, "         %2is", s);
	    }
	  }
};



void screen_printing_init(screen_t* screen)
{
	marlin_error_clr(MARLIN_ERR_ProbingFailed);
	int16_t id;

	strcpy(pw->text_time, "0m");
	strcpy(pw->text_filament, "999m");

	pw->state = P_PAUSED;

	int16_t root = window_create_ptr(WINDOW_CLS_FRAME, -1,
				rect_ui16(0,0,0,0),
				&(pw->root));

	id = window_create_ptr(WINDOW_CLS_HEADER, root,
					rect_ui16(0,0,240,31), &(pw->header));
	p_window_header_set_icon(&(pw->header), IDR_PNG_status_icon_printing);
	p_window_header_set_text(&(pw->header), "PRINTING");

	id = window_create_ptr(WINDOW_CLS_TEXT, root,
			rect_ui16(10, 33, 220, 29),
			&(pw->w_filename));
	pw->w_filename.font = resource_font(IDR_FNT_BIG);
	window_set_padding(id, padding_ui8(0,0,0,0));
	window_set_alignment(id, ALIGN_LEFT_BOTTOM);
	window_set_text(id, screen_printing_file_name);

	id = window_create_ptr(WINDOW_CLS_PROGRESS, root,
			rect_ui16(10, 70, 220, 50),
			&(pw->w_progress));
	pw->w_progress.color_progress = COLOR_ORANGE;
	pw->w_progress.font=resource_font(IDR_FNT_BIG);
	pw->w_progress.height_progress = 14;

	id = window_create_ptr(WINDOW_CLS_TEXT, root,
			rect_ui16(130, 128, 101, 20),
			&(pw->w_etime_label));
	pw->w_etime_label.font = resource_font(IDR_FNT_SMALL);
	window_set_alignment(id, ALIGN_RIGHT_BOTTOM);
	window_set_padding(id, padding_ui8(0,2,0,2));
	window_set_text(id, "Remaining Time");

	id = window_create_ptr(WINDOW_CLS_TEXT, root,
			rect_ui16(130, 148, 101, 20),
			&(pw->w_etime_value));
	pw->w_etime_value.font = resource_font(IDR_FNT_SMALL);
	window_set_alignment(id, ALIGN_RIGHT_BOTTOM);
	window_set_padding(id, padding_ui8(0,2,0,2));
	window_set_text(id, pw->text_etime);

	id = window_create_ptr(WINDOW_CLS_TEXT, root,
			rect_ui16(10, 128, 101, 20),
			&(pw->w_time_label));
	pw->w_time_label.font = resource_font(IDR_FNT_SMALL);
	window_set_alignment(id, ALIGN_RIGHT_BOTTOM);
	window_set_padding(id, padding_ui8(0,2,0,2));
	//window_set_text(id, "Remaining Time");
	window_set_text(id, "Printing Time");

	id = window_create_ptr(WINDOW_CLS_TEXT, root,
			rect_ui16(10, 148, 101, 20),
			&(pw->w_time_value));
	pw->w_time_value.font = resource_font(IDR_FNT_SMALL);
	window_set_alignment(id, ALIGN_RIGHT_BOTTOM);
	window_set_padding(id, padding_ui8(0,2,0,2));
	window_set_text(id, pw->text_time);

	for(uint8_t col=0; col<3; col++){
		id = window_create_ptr(
				WINDOW_CLS_ICON, root,
				rect_ui16(8+(15+64)*col, 185, 64, 64),
				&(pw->w_buttons[col]));
		window_set_color_back(id, COLOR_GRAY);
		window_set_icon_id(id, printing_icons[col]);
		window_set_tag(id, col+1);
		window_enable(id);

		id = window_create_ptr(
				WINDOW_CLS_TEXT, root,
				rect_ui16(80*col, 196+48+8, 80, 22),
				&(pw->w_labels[col]));
		pw->w_labels[col].font = resource_font(IDR_FNT_SMALL);
		window_set_padding(id, padding_ui8(0,0,0,0));
		window_set_alignment(id, ALIGN_CENTER);
		window_set_text(id, printing_labels[col]);
	}

	status_footer_init(&(pw->footer), root);
	screen_printing_timer(screen, 1000); // first fast value s update
}

void screen_printing_done(screen_t* screen)
{
	window_destroy(pw->root.win.id);
}

void screen_printing_draw(screen_t* screen)
{
}

int screen_printing_event(screen_t* screen, window_t* window, uint8_t event, void* param)
{
	if (marlin_error(MARLIN_ERR_ProbingFailed)){
		screen_mesh_err_stop_print(screen);
		marlin_error_clr(MARLIN_ERR_ProbingFailed);
	}

	if (p_window_header_event_clr(&(pw->header), MARLIN_EVT_MediaRemoved)){			// close screen when media removed
		screen_printing_pause_print(screen);
	}
	window_header_events(&(pw->header));

	screen_printing_timer(screen, (HAL_GetTick()/50)*50);

	if (status_footer_event(&(pw->footer), window, event, param)){
		return 1;
	}

	if (event != WINDOW_EVENT_CLICK){
		return 0;
	}

	switch ((int)param)
	{
		case BUTTON_TUNE+1:
			screen_open(pscreen_menu_tune->id);
			return 1;
			break;
		case BUTTON_PAUSE+1:
		{
			switch(pw->state){
				case P_PRINTING:
					screen_printing_pause_print(screen);
					break;
				case P_PAUSED:
					screen_printing_resume_print(screen);
					break;
				case P_PRINTED:
					screen_printing_reprint(screen);
					break;
			}
			break;
		}
		case BUTTON_STOP+1:
			if(pw->state == P_PRINTED){
				screen_close();
				return 1;
			}
			else if (gui_msgbox("Are you sure to stop this printing?",
					MSGBOX_BTN_YESNO | MSGBOX_ICO_WARNING | MSGBOX_DEF_BUTTON1) == MSGBOX_RES_YES)
			{
				marlin_print_abort();
				while (marlin_vars()->sd_printing)
				{
					gui_loop();
				}
				marlin_park_head();
				screen_close();
				return 1;
			}
			break;
	}
	return 0;
}

void screen_printing_timer(screen_t* screen, uint32_t mseconds){
	if ( (mseconds - pw->last_timer_repaint) >= 1000){
		screen_printing_update_progress(screen);
		pw->last_timer_repaint = mseconds;
	}
}
void screen_printing_disable_tune_button(screen_t * screen){
	pw->w_buttons[BUTTON_TUNE].win.f_disabled = 1;
	pw->w_buttons[BUTTON_TUNE].win.f_enabled = 0;		// cant't be focused

	// move to reprint when tune is focused
	if (window_is_focused(pw->w_buttons[BUTTON_TUNE].win.id)){
		window_set_focus(pw->w_buttons[BUTTON_PAUSE].win.id);
	}
}

void screen_printing_enable_tune_button(screen_t * screen){

	pw->w_buttons[BUTTON_TUNE].win.f_disabled = 0;
	pw->w_buttons[BUTTON_TUNE].win.f_enabled = 1;		// can be focused

}

void screen_printing_update_progress(screen_t* screen)

{
uint8_t nPercent;

	if (pw->state == P_PRINTING){
		if (!marlin_vars()->sd_printing) {
			screen_printing_printed(screen);
		}
	} else if (pw->state == P_PRINTED || pw->state == P_PAUSED) {
		if (marlin_vars()->sd_printing) {
			pw->state = P_PRINTING;
			window_set_text(pw->w_labels[BUTTON_PAUSE].win.id, printing_labels[P_PRINTING]);
			window_set_icon_id(pw->w_buttons[BUTTON_PAUSE].win.id, printing_icons[P_PRINTING]);
		}
	}

	if (pw->state == P_PRINTING){

        if(oProgressData.oPercentDone.mIsActual(marlin_vars()->print_duration))
             {
             nPercent=(uint8_t)oProgressData.oPercentDone.mGetValue();
             oProgressData.oTime2End.mFormatSeconds(pw->text_etime, marlin_vars()->print_speed);
             pw->w_etime_value.color_text=((marlin_vars()->print_speed == 100)?COLOR_VALUE_VALID:COLOR_VALUE_INVALID);
             pw->w_progress.color_text=COLOR_VALUE_VALID;
//_dbg(".progress: %d\r",nPercent);
             }
        else {
             nPercent=marlin_vars()->sd_percent_done;
             strcpy_P(pw->text_etime,PSTR("N/A"));
             pw->w_etime_value.color_text=COLOR_VALUE_VALID;
             pw->w_progress.color_text=COLOR_VALUE_INVALID;
//_dbg(".progress: %d ???\r",nPercent);
             }
        window_set_value(pw->w_progress.win.id,nPercent);
        window_set_text(pw->w_etime_value.win.id,pw->text_etime);

//-//		_dbg("progress: %d", ExtUI::getProgress_percent());
		_dbg("progress: %d", nPercent);
//_dbg("##% FeedRate %d",feedrate_percentage);
	}

	//const pduration_t e_time(ExtUI::getProgress_seconds_elapsed());

	const pduration_t e_time(marlin_vars()->print_duration);
	e_time.to_string(pw->text_time);
	window_set_text(pw->w_time_value.win.id, pw->text_time);
//_dbg("#.. progress / p :: %d t0: %d ?: %d\r",oProgressData.oPercentDirectControl.mGetValue(),oProgressData.oPercentDirectControl.nTime,oProgressData.oPercentDirectControl.mIsActual(print_job_timer.duration()));
//_dbg("#.. progress / P :: %d t0: %d ?: %d\r",oProgressData.oPercentDone.mGetValue(),oProgressData.oPercentDone.nTime,oProgressData.oPercentDone.mIsActual(print_job_timer.duration()));
//_dbg("#.. progress / R :: %d t0: %d ?: %d\r",oProgressData.oTime2End.mGetValue(),oProgressData.oTime2End.nTime,oProgressData.oTime2End.mIsActual(print_job_timer.duration()));
//_dbg("#.. progress / T :: %d t0: %d ?: %d\r",oProgressData.oTime2Pause.mGetValue(),oProgressData.oTime2Pause.nTime,oProgressData.oTime2Pause.mIsActual(print_job_timer.duration()));


	/*
	sprintf(pw->text_filament, "1.2m");
	window_set_text(pw->w_filament_value.win.id, pw->text_filament);
	*/
}

void screen_printing_pause_print(screen_t* screen)
{
	pw->state = P_PAUSED;
	window_set_text(pw->w_labels[BUTTON_PAUSE].win.id, printing_labels[P_PAUSED]);
	window_set_icon_id(pw->w_buttons[BUTTON_PAUSE].win.id, printing_icons[P_PAUSED]);
	marlin_print_pause();
}

void screen_printing_resume_print(screen_t* screen)
{
	marlin_print_resume();
}

void screen_printing_reprint(screen_t* screen)
{
	marlin_gcode_printf("M23 %s", screen_printing_file_path);
	marlin_gcode("M24");
	oProgressData.mInit();
	window_set_text(pw->w_etime_label.win.id,PSTR("Remaining Time")); // !!! "screen_printing_init()" is not invoked !!!

	window_set_text(pw->w_labels[BUTTON_STOP].win.id, printing_labels[2]);
	window_set_icon_id(pw->w_buttons[BUTTON_STOP].win.id, printing_icons[2]);

	screen_printing_enable_tune_button(screen);
}

void screen_printing_printed(screen_t* screen)
{
	pw->state = P_PRINTED;
	window_set_text(pw->w_labels[BUTTON_PAUSE].win.id, printing_labels[P_PRINTED]);
	window_set_icon_id(pw->w_buttons[BUTTON_PAUSE].win.id, printing_icons[P_PRINTED]);
	window_set_value(pw->w_progress.win.id, 100);

	window_set_text(pw->w_labels[BUTTON_STOP].win.id, printing_labels[5]);
	window_set_icon_id(pw->w_buttons[BUTTON_STOP].win.id, printing_icons[5]);

	pw->w_progress.color_text=COLOR_VALUE_VALID;
	window_set_text(pw->w_etime_label.win.id,PSTR(""));
	window_set_text(pw->w_etime_value.win.id,PSTR(""));

	screen_printing_disable_tune_button(screen);
}
void screen_mesh_err_stop_print(screen_t* screen)
{
	marlin_print_abort();
}

