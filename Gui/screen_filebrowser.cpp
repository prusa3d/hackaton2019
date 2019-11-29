/*
 * screen_menu_filebrowser.cpp
 *
 *  Created on: 23. 7. 2019
 *      Author: mcbig
 */


#include "gui.h"
#include "window_file_list.h"
#include "window_header.h"
#include "config.h"
#include "stdlib.h"
#include "usb_host.h"
#include "cmsis_os.h"
#include "marlin_client.h"
#include "screen_print_preview.h"
#include "print_utils.h"

#include "../Marlin/src/sd/cardreader.h"
#include "../Marlin/src/gcode/queue.h"

//-//
#include "../Marlin/src/gcode/lcd/M73_PE.h"

extern "C" {
void window_set_capture(int16_t id);
void window_file_list_draw(window_file_list_t* window);
}

extern osThreadId displayTaskHandle;
extern ApplicationTypeDef Appli_state;
ApplicationTypeDef Old_state = APPLICATION_IDLE;

#pragma pack(push)
#pragma pack(1)

typedef struct
{
	window_frame_t root;

	window_header_t header;
	window_file_list_t  w_filelist;

	char filename[128];

} screen_filebrowser_data_t;

#define pd ((screen_filebrowser_data_t*)screen->pdata)

#pragma pack(pop)

//TODO: remove this externs
extern screen_t* pscreen_printing;
extern char screen_printing_file_path[_MAX_LFN];
extern char screen_printing_file_name[_MAX_LFN];

// Default value could be rewrite from eeprom settings
static WF_Sort_t screen_filebrowser_sort = WF_SORT_BY_TIME;

const char * filters[] = {"*.gcode","*.gco"};
const size_t filt_cnt = sizeof(filters)/sizeof(const char *);
void screen_filebrowser_init(screen_t* screen)
{
	// TODO: load screen_filebrowser_sort from eeprom
	// FIXME: this could crash with very fast insert and eject, status_header will fix this
	marlin_event_clr(MARLIN_EVT_MediaRemoved);		// when screen is open, USB must be inserted

	int16_t id;
	int16_t root = window_create_ptr(WINDOW_CLS_FRAME, -1,
			rect_ui16(0,0,0,0),
			&(pd->root));
	window_disable(root);								// hack for do not change capture

	id = window_create_ptr(WINDOW_CLS_HEADER, root,
			rect_ui16(0,0,240,31), &(pd->header));
	p_window_header_set_icon(&(pd->header), IDR_PNG_filescreen_icon_folder);
	p_window_header_set_text(&(pd->header), "SELECT FILE");

	id = window_create_ptr(WINDOW_CLS_FILE_LIST, root,
			rect_ui16(10, 32, 220, 278),
			&(pd->w_filelist));
	window_file_list_load(&(pd->w_filelist), filters, filt_cnt, screen_filebrowser_sort);
	window_file_set_item_index(&(pd->w_filelist), 1);
	window_set_capture(id);								// hack for do not change capture
	window_set_focus(id);								// hack for do not change capture
}

void screen_filebrowser_done(_screen_t* screen)
{
	window_destroy(pd->root.win.id);
}

void screen_filebrowser_draw(screen_t* screen)
{}

static void on_print_preview_action(print_preview_action_t action)
{
	if (action == PRINT_PREVIEW_ACTION_BACK) {
		screen_close(); // close the print preview
	} else if (action == PRINT_PREVIEW_ACTION_PRINT) {
		screen_close(); // close the print preview
		screen_close(); // close the file browser
        print_begin(screen_print_preview_get_gcode_filepath());
        screen_open(pscreen_printing->id);
	}
}

int screen_filebrowser_event(screen_t* screen, window_t* window,
		uint8_t event, void* param)
{
	if (marlin_event_clr(MARLIN_EVT_MediaRemoved)){			// close screen when media removed
		screen_close();
		return 1;
	}

	window_header_events(&(pd->header));

	window_file_list_t * filelist = &(pd->w_filelist);

	if (event != WINDOW_EVENT_CLICK){
		return 0;
	}

	const FILINFO* file_item = &(filelist->file_items[(int)param]);
	//there must be fname, not altname
	if (!strcmp(file_item->fname, "..") && !strcmp(filelist->altpath, "/")){
		screen_close();
		return 1;
	}

	if (file_item->fattrib & AM_DIR){						// directory selected
		if (strcmp(file_item->fname, "..")){		// not same -> not ..
			//if ((strlen(filelist->path)+strlen(file_item->fname)+1) >= MAXPATHNAMELENGTH){
			if ((strlen(filelist->altpath)+strlen(file_item->altname)+1) >= MAXPATHNAMELENGTH){
				// TODO: notify too much depth
				return 0;
			}

			size_t len = strlen(filelist->altpath);
			if (filelist->altpath[len-1] != '/'){
				filelist->altpath[len++] = '/';
			}
			//strcpy(filelist->path+len, file_item->fname);
			strcpy(filelist->altpath+len, file_item->altname);
			window_file_list_load(filelist, filters, filt_cnt, screen_filebrowser_sort);
			window_set_text(pd->header.win.id, strrchr(filelist->altpath, '/'));
		} else {
			char * last = strrchr(filelist->altpath, '/');
			if (last == filelist->altpath){
				strcpy(last, "/");
			} else {
				last[0] = '\0';		// stop the string :D
			}
			window_file_list_load(filelist, filters, filt_cnt, screen_filebrowser_sort);
			window_set_text(pd->header.win.id, strrchr(filelist->altpath, '/'));
		}
	} else {									// print the file
		//if ((strlen(filelist->path)+strlen(file_item->fname)+1) >= MAXPATHNAMELENGTH){
		if ((strlen(filelist->altpath)+strlen(file_item->altname)+1) >= MAXPATHNAMELENGTH){
			// TODO: notify too much depth
			return 0;
		}

		char file_path[MAXPATHNAMELENGTH];

		if (!strcmp(filelist->altpath, "/"))
		{
			//sprintf(file_path, "/%s", file_item->fname);
			sprintf(file_path, "/%s", file_item->altname);
		}
		else
		{
			//sprintf(file_path, "%s/%s", filelist->path, file_item->fname);
			sprintf(file_path, "%s/%s", filelist->altpath, file_item->altname);
		}


		strcpy(screen_printing_file_path, file_path);
		strcpy(screen_printing_file_name, file_item->fname);

		screen_print_preview_set_on_action(on_print_preview_action);
		screen_print_preview_set_gcode_filename(screen_printing_file_name);
		screen_print_preview_set_gcode_filepath(screen_printing_file_path);
		screen_open(pscreen_print_preview->id);
		return 1;
	}

	return 0;
}

screen_t screen_filebrowser =
{
		0,
		0,
		screen_filebrowser_init,
		screen_filebrowser_done,
		screen_filebrowser_draw,
		screen_filebrowser_event,
		sizeof(screen_filebrowser_data_t),
		0
};

const screen_t* pscreen_filebrowser = &screen_filebrowser;

