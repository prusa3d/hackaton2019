//guiconfig.h - guiapi configuration file
#ifndef _GUICONFIG_H
#define _GUICONFIG_H


//--------------------------------------
//GUI configuration
#define GUI_USE_RTOS
#define GUI_JOGWHEEL_SUPPORT
#define GUI_WINDOW_SUPPORT


#ifdef GUI_USE_RTOS
#define GUI_SIG_REDRAW      0x0001  //display redraw request
#define GUI_SIG_JOGWHEEL    0x0002  //jogwheel change
#define GUI_SIG_TIMER       0x0004  //timer event
#endif //GUI_USE_RTOS


//--------------------------------------
//ST7789v configuration
#define ST7789V_USE_RTOS GUI_USE_RTOS
//#define ST7789V_PNG_SUPPORT


#endif //_GUICONFIG_H
