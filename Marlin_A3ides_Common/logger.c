#include "logger.h"
#include "w25x.h"

#define FLASH_MEMORY_START			0
#define FLASH_MEMORY_SIZE			0x800000	//8MB
#define PAGE_SIZE					0x1000		//4KB
#define LOGGER_VERSION				0x1			//increase when flash format is changed, value 0xFFFF is reserved for empty page

#define EMPTY_PAGE_MARKER			0xFFFF
#define EMPTY_MESSAGE_MARKER		0xFFFFFFFF

static char s_logger_is_initialized;
static uint32_t s_active_page_address;
static uint32_t s_active_page_number;
static uint32_t s_active_page_init_needed = 0;
static uint32_t s_next_record_address;

static uint32_t s_reading_record_address;
static uint32_t s_reading_eof = 0;

void init_page(uint32_t page_address)
{

}

int8_t init_logger()
{
	//TODO check whether HAL is already initialized
	int8_t result = w25x_init();
	w25x_wait_busy();

	log_page_header_t page_header;

	uint32_t flash_addr = FLASH_MEMORY_START;
	uint32_t latest_page_address = flash_addr;
	uint32_t latest_page_number = EMPTY_PAGE_MARKER;

	while (flash_addr < (FLASH_MEMORY_START + FLASH_MEMORY_SIZE)) {
		w25x_rd_data(flash_addr, &page_header, sizeof(log_page_header_t));
		//TODO check return value

		if (page_header.logger_version == EMPTY_PAGE_MARKER) {
			s_active_page_init_needed = 1;
			break;
		}

		if (page_header.page_number < latest_page_number) {
			break;
		}

		latest_page_address = flash_addr;
		latest_page_number = page_header.page_number;

		flash_addr += PAGE_SIZE;
	}

	s_active_page_address = latest_page_address;

	//find first empty space
	if (latest_page_number == EMPTY_PAGE_MARKER) {
		s_next_record_address = latest_page_address + sizeof(log_page_header_t);
	} else {
		flash_addr = latest_page_address;
		log_message_t log_message;

		while (flash_addr < (latest_page_address + PAGE_SIZE)) {
			w25x_rd_data(flash_addr, &log_message, sizeof(log_message));
			//TODO check return value

			if (log_message.timestamp == EMPTY_MESSAGE_MARKER) {
				break;
			}

			flash_addr += sizeof(log_message);
		}

		s_next_record_address = flash_addr;
	}

	s_logger_is_initialized = 1;
}

void log_message(log_context_t context, log_level_t level, log_module_t module, uint32_t code, const char* message)
{
	if (0 == s_logger_is_initialized) {
		if (!init_logger()) {
			return;	//logger is probably called too early
		}
	}

	//check whether there is enough room in actual page
	if (s_next_record_address + sizeof(log_message_t)
			> (s_active_page_address + PAGE_SIZE)) {
		//page is full, use new page
		s_active_page_address += PAGE_SIZE;
		s_active_page_number++;
		if (s_active_page_address >= (FLASH_MEMORY_START + FLASH_MEMORY_SIZE)) {
			s_active_page_address = FLASH_MEMORY_START;
		}

		s_next_record_address = s_active_page_address
				+ sizeof(log_page_header_t);
		s_active_page_init_needed = 1;
	}

	if (s_active_page_init_needed) {
		//erase page
		w25x_enable_wr();
		w25x_sector_erase(s_active_page_address);
		w25x_wait_busy();

		//prepare and save new page header
		log_page_header_t page_header;
		page_header.page_number = s_active_page_number;
		page_header.logger_version = LOGGER_VERSION;
		w25x_enable_wr();
		w25x_page_program(s_active_page_address, &page_header, sizeof(log_page_header_t));
		w25x_wait_busy();

		s_active_page_init_needed = 0;
	}

	//save new message
	log_message_t message_record;

	uint32_t timestamp = HAL_GetTick();
	if (timestamp == EMPTY_PAGE_MARKER)
		timestamp++;
	message_record.timestamp = timestamp;
	message_record.level = (uint8_t) level;
	message_record.module = module;
	message_record.code = code;
	message_record.message_length = 0;

	w25x_enable_wr();
	w25x_page_program(s_next_record_address, &message_record, sizeof(log_message_t));
	w25x_wait_busy();

	s_next_record_address += sizeof(log_message_t);

}

void init_logger_reading()
{
	//find first free page or oldest page
	log_page_header_t page_header;

	uint32_t flash_addr = FLASH_MEMORY_START;
	uint32_t first_page_address = 0;
	uint32_t first_page_number = 0;
	uint32_t eof = 1;

	while (flash_addr < (FLASH_MEMORY_START + FLASH_MEMORY_SIZE)) {
		w25x_rd_data(flash_addr, &page_header, sizeof(log_page_header_t));
		//TODO check return value

		if ( flash_addr ==  FLASH_MEMORY_START )
		{
			first_page_address = 0;
		    first_page_number = page_header.page_number;
		}

		if (page_header.logger_version == EMPTY_PAGE_MARKER) {
			break;
		}

		if (page_header.page_number < first_page_number) {
			first_page_address = flash_addr;
		    first_page_number = page_header.page_number;
			break;
		}

		flash_addr += PAGE_SIZE;
	}

	s_reading_record_address = first_page_address + sizeof(page_header);


}

uint32_t get_next_logged_message( uint32_t*	pTimestamp, log_level_t* pLevel, log_module_t* pModule, uint32_t* pCode, char* pMessage )
{
	if ( s_reading_eof )
	{
		return 0;
	}

	log_message_t log_message;

	w25x_rd_data(s_reading_record_address, &log_message, sizeof(log_message));

	if (log_message.timestamp == EMPTY_MESSAGE_MARKER)
	{
		s_reading_eof = 0;
		return 0;
	}

	*pTimestamp = log_message.timestamp;
	*pLevel = log_message.level;
	*pModule = log_message.module;
	*pCode = log_message.code;

	uint32_t page_start = (s_reading_record_address / PAGE_SIZE) * PAGE_SIZE;
	//TODO jump over page end

	//TODO detect last page

	s_reading_record_address += sizeof(log_message);

	return 1;
}

static tested = 0;

void test_logger() {
	if (tested)
	{
		return;
	}
	tested = 1;

	init_logger();

	w25x_chip_erase();

	log_message( LOGCONTEXT_DelayTolerant, LOGLEVEL_INFO, LOGMODULE_FreeRTOS, 0, "Idle" );

	init_logger_reading();


	uint32_t timestamp;
	log_level_t level;
	log_module_t module;
	uint32_t code;
	char  message[8];

	uint32_t result = get_next_logged_message( &timestamp, &level, &module, &code, &message );

	result = get_next_logged_message( &timestamp, &level, &module, &code, &message );

	//	check_flash();
//	log_message( LOGCONTEXT_DelayTolerant, LOGLEVEL_INFO, LOGMODULE_FreeRTOS, 1, "Idle" );
//	check_flash();
//	log_message( LOGCONTEXT_DelayTolerant, LOGLEVEL_INFO, LOGMODULE_FreeRTOS, 2, "Idle" );
//	check_flash();

//	int8_t init_logger();

}


