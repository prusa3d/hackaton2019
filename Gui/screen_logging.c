#include "gui.h"
#include "config.h"
#include "window_logo.h"
#include "stm32f4xx_hal.h"
#include "log_file.h"
#include "logger.h"

#include "dbg.h"
#include "sys.h"
#include "../Middlewares/ST/Utilites/CPU/cpu_utils.h"

#pragma pack(push)
#pragma pack(1)

typedef struct
{
    window_frame_t frame;
    window_text_t textMenuName;
    window_text_t textSave;
    window_text_t textErrase;
    window_text_t textExit;
} screen_logging_data_t;

#pragma pack(pop)

#define pda ((screen_logging_data_t*)screen->pdata)
/******************************************************************************************************/
//variables

/******************************************************************************************************/
//methods

/******************************************************************************************************/
//column specifications
enum { col_0 = 2,
    col_1 = 96 };
enum { col_0_w = col_1 - col_0,
    col_1_w = 240 - col_1 - col_0 };
enum { col_2_w = 38 };
#define RECT_MACRO(col) rect_ui16(col_##col, row2draw, col_##col##_w, row_h)

enum {
    TAG_QUIT = 10,
	TAG_SAVE = 20,
	TAG_ERASE = 30

};

void screen_logging_init(screen_t* screen)
{
    int16_t row2draw = 0;
    int16_t id;

    int16_t id0 = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), &(pda->frame));

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(0, 0, display->w, 22), &(pda->textMenuName));
    pda->textMenuName.font = resource_font(IDR_FNT_BIG);
    window_set_text(id, (const char*)"Logging screen");

    row2draw += 25;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(col_0, 40, 150, 22), &(pda->textSave));
    pda->textExit.font = resource_font(IDR_FNT_BIG);
    window_set_text(id, (const char*)"SAVE TO FLASH");
    window_enable(id);
    window_set_tag(id, TAG_SAVE);

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(col_0, 65, 120, 22), &(pda->textErrase));
    pda->textExit.font = resource_font(IDR_FNT_BIG);
    window_set_text(id, (const char*)"ERASE LOG");
    window_enable(id);
    window_set_tag(id, TAG_ERASE);

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(col_0, 290, 60, 22), &(pda->textExit));
    pda->textExit.font = resource_font(IDR_FNT_BIG);
    window_set_text(id, (const char*)"EXIT");
    window_enable(id);
    window_set_tag(id, TAG_QUIT);
}

void screen_logging_done(screen_t* screen)
{
    window_destroy(pda->frame.win.id);
}

void screen_logging_draw(screen_t* screen)
{
}

#include "ff.h"
static FIL logFile;

int screen_logging_event(screen_t* screen, window_t* window, uint8_t event, void* param)
{
	FRESULT ret;
	uint32_t wb;

	uint32_t next_message = 0;
	log_message_t message;

	char json[MAX_JSON_LEN];
	uint32_t json_len;

    if (event == WINDOW_EVENT_CLICK)
        switch ((int)param) {
        case TAG_QUIT:
            screen_close();
            return 1;

        case TAG_SAVE:
        	ret = f_open(&logFile, (const TCHAR*)"log1.txt", FA_CREATE_NEW | FA_WRITE | FA_READ);

        	init_logger_reading();
        	next_message = get_next_logged_message(&message.timestamp, &message.level, &message.module, &message.code, &message.message, 117);
        	while (next_message == 1)
        	{
        		log_msg_to_json(&message, json, &json_len);

        		if (ret == FR_OK) ret = f_write(&logFile, json, json_len, (void*)&wb);
        		if (ret != FR_OK) _dbg(json);
        		next_message = get_next_logged_message(&message.timestamp, &message.level, &message.module, &message.code, &message.message, 117);
        	}

        	ret = f_sync(&logFile);
        	ret = f_close(&logFile);

        	if (ret == FR_OK)
        	{
        		_new_dbg(LOGLEVEL_DEBUG, LOGMODULE_GUI, 1000, "Save log to file on USB ok.");
        	}
        	else
        	{
        		_new_dbg(LOGLEVEL_ERROR, LOGMODULE_GUI, 1001, "Save log to file on USB failed.");
			}
            return 1;

        case TAG_ERASE:
            //test_logger();
            //init_logger();
        	_new_dbg(LOGLEVEL_DEBUG, LOGMODULE_GUI, 1002, "Erase log memory.");
            return 1;
        }

    if (event == WINDOW_EVENT_LOOP) {

    }

    return 0;
}

screen_t screen_logging = {
    0,
    0,
    screen_logging_init,
    screen_logging_done,
    screen_logging_draw,
    screen_logging_event,
    sizeof(screen_logging_data_t), //data_size
    0, //pdata
};

const screen_t* pscreen_logging = &screen_logging;
