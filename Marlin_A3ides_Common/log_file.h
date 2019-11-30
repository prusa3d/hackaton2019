#ifndef LOG_FILE_H_
#define LOG_FILE_H_

#include "logger.h"

#define MAX_JSON_LEN 200
#define MAX_LEVEL_TEXT_LEN 20
#define MAX_MODUL_TEXT_LEN 20

int log_open();
int log_read(char* buffer, int count);
void log_msg_to_json(log_message_t *message, char* json, uint32_t* json_len);
int log_write(char* json, uint32_t json_len);
void log_close();

#endif /* LOG_FILE_H_ */
