#include "logger.h"
#include "w25x.h"

#define FLASH_MEMORY_START			0
#define FLASH_MEMORY_SIZE			0x800000	//8MB
#define PAGE_SIZE					0x1000		//4KB
#define LOGGER_VERSION				0x1			//increase when flash format is changed, value 0xFFFF is reserved for empty page

#define EMPTY_PAGE_MARKER			0xFFFF
#define EMPTY_MESSAGE_MARKER		0xFFFFFFFF

static char s_logger_is_initialized;
static uint32_t s_next_free_record = 0;

static uint32_t s_first_reading_record_address;
static uint32_t s_reading_record_address;
static uint32_t s_reading_eof = 0;

void init_logger()
{
	if ( s_logger_is_initialized )
	{
		return;
	}

	//TODO check whether HAL is already initialized
	int8_t result = w25x_init();
	w25x_wait_busy();

	uint32_t flash_addr = FLASH_MEMORY_START;
	uint32_t timestamp;

	while (flash_addr < (FLASH_MEMORY_START + FLASH_MEMORY_SIZE))
	{
		w25x_rd_data(flash_addr, &timestamp, sizeof(timestamp));
		//TODO check return value

		if (timestamp == EMPTY_MESSAGE_MARKER)
		{
			break;
		}

		flash_addr += sizeof(log_message_t);
	}

	if (flash_addr >= (FLASH_MEMORY_START + FLASH_MEMORY_SIZE))
	{
		flash_addr = FLASH_MEMORY_START;
	}

	s_next_free_record = flash_addr;
	s_logger_is_initialized = 1;
}

void log_message(log_level_t level, log_module_t module, uint32_t code, const char* message)
{
	//init logger if not initialized
	if (0 == s_logger_is_initialized) {
		init_logger();
	}

	//save new record
	log_message_t message_record;
	uint32_t timestamp = HAL_GetTick();
	if (timestamp == EMPTY_PAGE_MARKER)
		timestamp++;
	message_record.timestamp = timestamp;
	message_record.level = (uint8_t) level;
	message_record.module = module;
	message_record.code = code;
	strncpy(message_record.message, message, sizeof(message_record.message));

	w25x_enable_wr();
	w25x_page_program(s_next_free_record, &message_record, sizeof(log_message_t));
	w25x_wait_busy();

	//assure that there is enough free room for next record
	uint32_t page_address_1 = (s_next_free_record / PAGE_SIZE) * PAGE_SIZE;
	uint32_t page_address_2 = ((s_next_free_record + sizeof(log_message_t)) / PAGE_SIZE) * PAGE_SIZE;

	s_next_free_record += sizeof(log_message_t);


	if(page_address_2 > page_address_1)
	{
		//roll over memory space if necessary
		if(page_address_2 >= (FLASH_MEMORY_START + FLASH_MEMORY_SIZE))
		{
			page_address_2 = FLASH_MEMORY_START;
			s_next_free_record = FLASH_MEMORY_START;
		}

		//clear page after this record
		w25x_enable_wr();
		w25x_sector_erase(page_address_2);
		w25x_wait_busy();
	}

}

void init_logger_reading()
{
	if (0 == s_logger_is_initialized) {
		init_logger();
	}

	uint32_t flash_addr = FLASH_MEMORY_START;
	uint32_t timestamp;

	while (flash_addr < (FLASH_MEMORY_START + FLASH_MEMORY_SIZE))
	{
		w25x_rd_data(flash_addr, &timestamp, sizeof(timestamp));
		//TODO check return value

		if (timestamp == EMPTY_MESSAGE_MARKER)
		{
			break;
		}

		flash_addr += sizeof(log_message_t);
	}

	if (flash_addr >= (FLASH_MEMORY_START + FLASH_MEMORY_SIZE))
	{
		flash_addr = 0;
	}
	else
	{
		while (flash_addr < (FLASH_MEMORY_START + FLASH_MEMORY_SIZE))
		{
			w25x_rd_data(flash_addr, &timestamp, sizeof(timestamp));
			//TODO check return value

			if (timestamp != EMPTY_MESSAGE_MARKER)
			{
				break;
			}

			flash_addr += sizeof(log_message_t);
		}

	}

	s_first_reading_record_address = flash_addr;
	s_reading_record_address = flash_addr;
}

uint32_t get_next_logged_message( uint32_t*	pTimestamp, log_level_t* pLevel, log_module_t* pModule, uint32_t* pCode, char* pMessage, uint32_t messegeBufferSize )
{
	if (0 == s_logger_is_initialized) {
		init_logger();
	}

	if ( s_reading_eof )
	{
		return 0;
	}

	//read message
	log_message_t log_message;
	w25x_rd_data(s_reading_record_address, &log_message, sizeof(log_message));

	//detect first empty message
	if (log_message.timestamp == EMPTY_MESSAGE_MARKER)
	{
		s_reading_eof = 1;
		return 0;
	}

	*pTimestamp = log_message.timestamp;
	*pLevel = log_message.level;
	*pModule = log_message.module;
	*pCode = log_message.code;
	strncpy(pMessage, log_message.message, messegeBufferSize);

	s_reading_record_address += sizeof(log_message);

	if (s_reading_record_address >= (FLASH_MEMORY_START + FLASH_MEMORY_SIZE))
	{
		s_reading_record_address = FLASH_MEMORY_START;
	}

	return 1;
}

static tested = 0;

void assert( uint32_t arg)
{
	if(!arg)
	{
		for(;;);
	}
}

void read_and_test_message( log_level_t expectedLevel, log_module_t expectedModule, uint32_t expectedCode, char* expectedMessage  )
{
	uint32_t timestamp;
	log_level_t level;
	log_module_t module;
	uint32_t code;
	char message[128];

	uint32_t result = get_next_logged_message( &timestamp, &level, &module, &code, message, sizeof(message) );

	assert( result == 1 );
	assert( level == expectedLevel );
	assert( module == expectedModule );
	assert( code == expectedCode );
	assert( strncmp(message, expectedMessage, sizeof(message) ) == 0);
}


void read_and_test_eof_message( )
{
	uint32_t timestamp;
	log_level_t level;
	log_module_t module;
	uint32_t code;
	char* message[128];

	uint32_t result = get_next_logged_message( &timestamp, &level, &module, &code, &message, sizeof(message) );

	assert( result == 0 );
}

void test_logger() {
	if (tested)
	{
		return;
	}
	tested = 1;

	w25x_init();
	w25x_enable_wr();
	w25x_chip_erase();
	w25x_wait_busy();

	uint32_t codeCounter = 0;
	for(int i = 0; i < 33; i++)		//one page is 32 resords
	{
		log_message( LOGLEVEL_INFO, LOGMODULE_FreeRTOS, codeCounter++, "Idle" );
		log_message( LOGLEVEL_DEBUG, LOGMODULE_Marlin, codeCounter++, "Marlin run" );
		log_message( LOGLEVEL_WARNING, LOGMODULE_GUI, codeCounter++, "Refresh" );
		log_message( LOGLEVEL_ERROR, LOGMODULE_GUI, codeCounter++, "Dialog" );
	}

	init_logger_reading();

	codeCounter = 0;
	for(int i = 0; i < 33; i++)		//one page is 32 resords
	{
		read_and_test_message( LOGLEVEL_INFO, LOGMODULE_FreeRTOS, codeCounter++, "Idle" );
		read_and_test_message( LOGLEVEL_DEBUG, LOGMODULE_Marlin, codeCounter++, "Marlin run" );
		read_and_test_message( LOGLEVEL_WARNING, LOGMODULE_GUI, codeCounter++, "Refresh" );
		read_and_test_message( LOGLEVEL_ERROR, LOGMODULE_GUI, codeCounter++, "Dialog" );
	}
	read_and_test_eof_message();

	w25x_enable_wr();
	w25x_chip_erase();
	w25x_wait_busy();

}


