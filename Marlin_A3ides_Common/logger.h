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

typedef struct		//size of this structure shall be integer fraction of PAGE_SIZE defined in logger.c
{
	uint32_t	timestamp;		//cannot be 0xFFFFFFFF
	uint8_t		level;
	uint16_t	module;
	uint32_t	code;
	char		message[117];	//round structure to 128 bytes
} log_message_t;

#pragma pack(pop)


void log_message( log_level_t level, log_module_t module, uint32_t code, const char* message );

void test_logger();

void init_logger_reading();
uint32_t get_next_logged_message( uint32_t*	pTimestamp, log_level_t* pLevel, log_module_t* pModule, uint32_t* pCode, char* pMessage, uint32_t messegeBufferSize );


#ifdef __cplusplus
}
#endif //__cplusplus


#endif /* LOGGER_H_ */
