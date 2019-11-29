// wizard_load_unload.c

#include "wizard_load_unload.h"
#include "wizard_ui.h"
#include "window_dlg_loadunload.h"
#include "window_dlg_preheat.h"

LD_UNLD_STATE_t wizard_load_unload(LD_UNLD_STATE_t state)
{
    switch (state) {
    case LD_UNLD_INIT:
        return LD_UNLD_MSG_DECIDE_CONTINUE_LOAD_UNLOAD;

    case LD_UNLD_MSG_DECIDE_CONTINUE_LOAD_UNLOAD: {
        //cannot use CONTINUE button, string is too long
        const char* btns[3] = { "NEXT", "LOAD", "UNLOAD" };
        switch (wizard_msgbox_btns(
            "If a filament is loaded correctly press NEXT.\n"
            "If a filament is not loaded press LOAD.\n"
            "If a filament is not loaded correctly press UNLOAD.",
            MSGBOX_BTN_CUSTOM3, 0, btns)) {
        case MSGBOX_RES_CUSTOM0:
            return LD_UNLD_DIALOG_PREHEAT;
        case MSGBOX_RES_CUSTOM1:
            return LD_UNLD_DIALOG_LOAD;
        case MSGBOX_RES_CUSTOM2:
            return LD_UNLD_DIALOG_UNLOAD;
        default:
            //should not happen
            return LD_UNLD_DIALOG_PREHEAT;
        }
    }

    case LD_UNLD_DIALOG_PREHEAT:
        gui_dlg_preheat_forced("SELECT FILAMENT TYPE");
        return LD_UNLD_DONE;

    case LD_UNLD_DIALOG_LOAD:
        switch (gui_dlg_load_forced()) {
        case LD_OK:
            return LD_UNLD_DONE;
        case LD_ABORTED:
            return LD_UNLD_MSG_DECIDE_CONTINUE_LOAD_UNLOAD;
        }

    case LD_UNLD_DIALOG_UNLOAD:
        switch (gui_dlg_unload_forced()) {
        case LD_OK:
            return LD_UNLD_DIALOG_LOAD;
        case LD_ABORTED:
            return LD_UNLD_MSG_DECIDE_CONTINUE_LOAD_UNLOAD;
        }

    case LD_UNLD_DONE:
    default:
        return state;
    }

    //dlg_load
}
