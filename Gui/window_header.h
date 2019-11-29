/*
 * window_header.h
 *
 *  Created on: 19. 7. 2019
 *      Author: mcbig
 */

#ifndef WINDOW_HEADER_H_
#define WINDOW_HEADER_H_

#include "gui.h"
#include "marlin_events.h"

typedef enum {
	HEADER_USB = 1,
	HEADER_LAN = 2,
	HEADER_WIFI = 4
} header_states_t;

#pragma pack(push)
#pragma pack(1)

typedef struct _window_class_header_t
{
	window_class_t cls;
} window_class_header_t;

typedef struct _window_header_t
{
	window_t win;

	color_t color_back;
	color_t color_text;
	font_t* font;
	padding_ui8_t padding;
	uint8_t alignment;
	uint16_t id_res;
	uint8_t states;
	const char * label;

	// char time[10];
} window_header_t;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern int16_t WINDOW_CLS_HEADER;

extern const window_class_header_t window_class_header;

void p_window_header_set_icon(window_header_t* window, uint16_t id_res);
void p_window_header_enable_state(window_header_t* window, header_states_t state);
void p_window_header_disable_state(window_header_t* window, header_states_t state);
uint8_t p_window_header_get_state(window_header_t* window, header_states_t state);
void p_window_header_set_text(window_header_t* window, const char* text);
int p_window_header_event_clr(window_header_t* window, uint8_t evt_id);

#define window_header_events(window) \
		p_window_header_event_clr(window, MARLIN_EVT_MediaInserted); \
		p_window_header_event_clr(window, MARLIN_EVT_MediaRemoved); \
		p_window_header_event_clr(window, MARLIN_EVT_MediaError);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* WINDOW_HEADER_H_ */
