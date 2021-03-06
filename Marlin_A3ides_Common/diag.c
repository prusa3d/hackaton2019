// diag.c

#include "diag.h"
#include "otp.h"
#include "gpio.h"
#include "stm32f4xx_hal.h"
#include "w25x.h"
#include "st25dv64k.h"
#include "usb_host.h"

int diag_fastboot = 0;

int diag_error = 0;

extern ApplicationTypeDef Appli_state;

void diag_delay(int delay)
{
    volatile int i;
    for (i = 0; i < delay; i++) {
    }
}

void diag_check_fastboot(void)
{
    if (otp_lock_sector0) //not locked
    {
        __HAL_RCC_GPIOC_CLK_ENABLE();
        gpio_init(PC7, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_VERY_HIGH);
        diag_delay(100000);
        int i;
        for (i = 0; i < 10; i++) {
            if (gpio_get(PC7))
                break;
            diag_delay(10000);
        }
        diag_fastboot = ((i == 10) && !gpio_get(PC7)) ? 1 : 0;
    }
}

int diag_eeprom()
{
    int ee_adress = 0x00;
    int ee_data = 0x56;
    int diag_eeprom;
    st25dv64k_init();
    st25dv64k_user_write(ee_adress, ee_data);
    diag_eeprom = (st25dv64k_user_read(ee_adress) == ee_data) ? 1 : 0;
    return diag_eeprom;
}

void diag_test(void)
{
    diag_error = 0;
    //SPI DIAG
    if (diag_error == 0)
        if (!w25x_init())
            diag_error = DIAG_ERR_SPIFLASH;

    //I2C EEPROM
    if (diag_error == 0)
        if (!diag_eeprom())
            diag_error = DIAG_ERR_I2CEEPROM;

    //USBA
    if (diag_error == 0)
        if (!(Appli_state == APPLICATION_READY))
            diag_error = DIAG_ERR_USBA;
}
