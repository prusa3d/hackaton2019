// eeprom.h

#ifndef _EEPROM_H
#define _EEPROM_H

#include "variant8.h"


#define EEVAR_VERSION         0x00
#define EEVAR_FILAMENT_TYPE   0x01
#define EEVAR_FILAMENT_COLOR  0x02


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


// initialize eeprom
extern uint8_t eeprom_init(void);

// get variable value as variant8
extern variant8_t eeprom_get_var(uint8_t id);

// set variable value as variant8
extern void eeprom_set_var(uint8_t id, variant8_t var);

// fill range 0x0000..0x0800 with 0xff
extern void eeprom_clear(void);

int8_t eeprom_test_PUT(const unsigned int);

#ifdef __cplusplus
}
#endif //__cplusplus


#endif //_EEPROM_H
