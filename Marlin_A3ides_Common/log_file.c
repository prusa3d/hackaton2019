#include "ff.h"
#include "log_file.h"
#include "dbg.h"
#include "logger.h"

static FIL logFile;

int log_open()
{
    if (f_open(&logFile, (const TCHAR*)"log.txt", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
        return 0;
    }

    return 1;
}

int log_read(char* buffer, int count)
{
    FRESULT res;
    uint32_t bytesRead = 0;

    if (f_eof(&logFile)) {
        return -1;
    }

    res = f_read(&logFile, buffer, count, (void*)&bytesRead);

    if (res != FR_OK) {
        return 0;
    }

    return bytesRead;
}

void log_msg_to_json(log_message_t *message, char* json, uint32_t* json_len)
{
	char text[MAX_JSON_LEN];
	char level[MAX_LEVEL_TEXT_LEN];
	char module[MAX_MODUL_TEXT_LEN];

	switch(message->level)
	{
		case LOGLEVEL_UNDEFINED:
			snprintf(level, MAX_LEVEL_TEXT_LEN, "Undefined");
			break;
		case LOGLEVEL_DEBUG:
			snprintf(level, MAX_LEVEL_TEXT_LEN, "Debug");
			break;
		case LOGLEVEL_INFO:
			snprintf(level, MAX_LEVEL_TEXT_LEN, "Info");
			break;
		case LOGLEVEL_WARNING:
			snprintf(level, MAX_LEVEL_TEXT_LEN, "Warning");
			break;
		case LOGLEVEL_ERROR:
			snprintf(level, MAX_LEVEL_TEXT_LEN, "Error");
			break;
		case LOGLEVEL_FATAL:
			snprintf(level, MAX_LEVEL_TEXT_LEN, "Fatal");
			break;
		default:
			snprintf(level, MAX_LEVEL_TEXT_LEN, "Unknown");
			break;
	}

	switch(message->module)
	{
		case LOGMODULE_FreeRTOS:
			snprintf(module, MAX_MODUL_TEXT_LEN, "FreeRTOX");
			break;
		case LOGMODULE_Marlin:
			snprintf(module, MAX_MODUL_TEXT_LEN, "Marlin");
			break;
		case LOGMODULE_GUI:
			snprintf(module, MAX_MODUL_TEXT_LEN, "GUI");
			break;
		default:
			snprintf(module, MAX_MODUL_TEXT_LEN, "Unknown");
			break;
	}

	snprintf(json, MAX_JSON_LEN, "\r{\"ms\":%d, \"type\":\"%s\", \"source\":\"%s\", \"code\":%d, \"message\":\"%s\"},", (int)message->timestamp, level, module, (int)message->code, message->message);
	*json_len = strlen(text);
}

int log_write(char* json, uint32_t json_len)
{
	//FRESULT res;
	uint32_t bytesWrite = 0;

	_dbg(json);
	//res = f_write(&logFile, &json, json_len, (void*)&bytesWrite);

	//if (res != FR_OK) {
	//    return 0;
	//}

	return bytesWrite;
}

void log_close()
{
	f_sync(&logFile);
    f_close(&logFile);
}

