// screen_menu_settings.c

#include "gui.h"
#include "app.h"
#include "marlin_client.h"
#include "screen_menu.h"
#include "cmsis_os.h"

extern screen_t screen_menu_temperature;
extern screen_t screen_menu_move;
#ifdef _DEBUG
extern screen_t screen_menu_service;
extern screen_t screen_test;
#endif //_DEBUG
extern osThreadId webServerTaskHandle;

const char* settings_opt_enable_disable[] = { "Off", "On", NULL };

typedef enum {
    MI_RETURN,
    MI_TEMPERATURE,
    MI_MOVE_AXIS,
    MI_DISABLE_STEP,
    MI_FACTORY_DEFAULTS,
#ifdef _DEBUG
    MI_SERVICE,
    MI_TEST,
#endif //_DEBUG
    MI_TIMEOUT,
#ifdef _DEBUG
    MI_ETHSTART,
#endif //_DEBUG
} MI_t;

const menu_item_t _menu_settings_items[] = {
    { { "Temperature", 0, WI_LABEL }, &screen_menu_temperature },
    { { "Move Axis", 0, WI_LABEL }, &screen_menu_move },
    { { "Disable Steppers", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
    { { "Factory Defaults", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
#ifdef _DEBUG
    { { "Service", 0, WI_LABEL }, &screen_menu_service },
    { { "Test", 0, WI_LABEL }, &screen_test },
#endif //_DEBUG
    { { "Timeout", 0, WI_SWITCH, .wi_switch_select = { 0, settings_opt_enable_disable } }, SCREEN_MENU_NO_SCREEN },
#ifdef _DEBUG
    { { "Eth. start", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
#endif //_DEBUG
};

void screen_menu_settings_init(screen_t* screen)
{
    int count = sizeof(_menu_settings_items) / sizeof(menu_item_t);
    screen_menu_init(screen, "SETTINGS", count + 1, 1, 0);
    psmd->items[MI_RETURN] = menu_item_return;
    memcpy(psmd->items + 1, _menu_settings_items, count * sizeof(menu_item_t));
    psmd->items[MI_TIMEOUT].item.wi_switch_select.index = menu_timeout_enabled; //st25dv64k_user_read(MENU_TIMEOUT_FLAG_ADDRESS)
}

int screen_menu_settings_event(screen_t* screen, window_t* window, uint8_t event, void* param)
{
    if (screen_menu_event(screen, window, event, param))
        return 1;
    if (event == WINDOW_EVENT_CLICK) {
        switch ((int)param) {
        case MI_DISABLE_STEP:
            marlin_gcode("M18");
            break;
        case MI_FACTORY_DEFAULTS:
            if (gui_msgbox("This operation can't be undone, current configuration will be lost! Are you really sure to reset printer to factory defaults?", MSGBOX_BTN_YESNO | MSGBOX_ICO_WARNING) == MSGBOX_RES_YES) {
                marlin_event_clr(MARLIN_EVT_FactoryReset);
                marlin_gcode("M502");
                while (!marlin_event_clr(MARLIN_EVT_FactoryReset)) {
                    gui_loop();
                }
                marlin_event_clr(MARLIN_EVT_StoreSettings);
                marlin_gcode("M500");
                while (!marlin_event_clr(MARLIN_EVT_StoreSettings)) {
                    gui_loop();
                }
                gui_msgbox("Factory defaults loaded.", MSGBOX_BTN_OK | MSGBOX_ICO_INFO);
            }
            break;
        case MI_TIMEOUT:
            if (menu_timeout_enabled == 1) {
                menu_timeout_enabled = 0;
                gui_timer_delete(gui_get_menu_timeout_id());
                //st25dv64k_user_write((uint16_t)MENU_TIMEOUT_FLAG_ADDRESS, (uint8_t)0);
            } else {
                menu_timeout_enabled = 1;
                //st25dv64k_user_write((uint16_t)MENU_TIMEOUT_FLAG_ADDRESS, (uint8_t)1);
            }
            break;
#ifdef _DEBUG
        case MI_ETHSTART:
            osThreadResume(webServerTaskHandle);
            break;
#endif //_DEBUG
        }
    }
    return 0;
}

screen_t screen_menu_settings = {
    0,
    0,
    screen_menu_settings_init,
    screen_menu_done,
    screen_menu_draw,
    screen_menu_settings_event,
    sizeof(screen_menu_data_t), //data_size
    0, //pdata
};

const screen_t* pscreen_menu_settings = &screen_menu_settings;
