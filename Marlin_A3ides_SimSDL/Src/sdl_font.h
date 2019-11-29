#ifndef _FONT_H
#define _FONT_H

#include <inttypes.h>


typedef struct _sdl_font_t
{
	uint8_t w;     //char width [pixels]
	uint8_t h;     //char height [pixels]
	uint8_t bpr;   //bits per row
	void* pcs;     //charset data pointer
	char asc_min;  //min ascii code (first character)
	char asc_max;  //max ascii code (last character)
} sdl_font_t;


#endif //_FONT_H
