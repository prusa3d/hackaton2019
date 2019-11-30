#ifndef LOGGER_H_
#define LOGGER_H_

#include "stm32f4xx_hal.h"


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


typedef enum
{
	LOGLEVEL_UNDEFINED = 0,
	LOGLEVEL_DEBUG,
	LOGLEVEL_INFO,
	LOGLEVEL_WARNING,
	LOGLEVEL_ERROR,
	LOGLEVEL_FATAL,
} log_level_t;


typedef enum
{
	LOGMODULE_FreeRTOS,
	LOGMODULE_Marlin,
	LOGMODULE_GUI,
} log_module_t;

typedef enum
{
	LOGCONTEXT_DelayTolerant,
	LOGCONTEXT_TimeCritical,
} log_context_t;

#pragma pack(push, 1)

typedef struct
{
	uint32_t	timestamp;
	uint8_t		level;
	uint16_t	module;
	uint32_t	code;
	uint8_t		message_length;
	char		message[128];
} log_message_t;

#pragma pack(pop)


void log_message( log_context_t context, log_level_t level, log_module_t module, uint32_t code, const char* message );


#ifdef __cplusplus
}
#endif //__cplusplus


#endif /* LOGGER_H_ */
