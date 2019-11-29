/*
 * filament.h
 *
 *  Created on: 19. 7. 2019
 *      Author: mcbig
 */

#ifndef FILAMENT_H_
#define FILAMENT_H_

#include "gui.h"

#pragma pack(push)
#pragma pack(1)

typedef struct {
	const char * name;
	const char * long_name;
	uint16_t nozzle;
	uint16_t heatbed;
} filament_t;

#pragma pack(pop)

typedef enum {
	FILAMENT_NONE = 0,
	FILAMENT_PLA,
	FILAMENT_PETG,
	FILAMENT_ASA,
	FILAMENT_FLEX,
	FILAMENTS_END
}FILAMENT_t;

//#define FILAMENT_COUNT ((uint32_t)FILAMENTS_END-1)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern const filament_t filaments[FILAMENTS_END];

void set_filament(FILAMENT_t filament);

FILAMENT_t get_filament();

void retract_filament_gcode(int length);

uint32_t unload_filament_gcode();

uint32_t load_filament_gcode(uint8_t up_z);

void flush_filamnet_gcode();

uint8_t gui_load_filament(window_progress_t * w_progress, window_text_t * w_text, uint8_t up_z);

void gui_unload_filament(window_progress_t * w_progress, window_text_t * w_text, uint8_t ask);

void gui_change_filament(window_progress_t * w_progress, window_text_t * w_text);

uint8_t get_relative(uint8_t axis);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* FILAMENT_H_ */
