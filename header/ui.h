//
// Created by evanmd25 on 2025-12-08.
//

#ifndef PROJETLP_UI_H
#define PROJETLP_UI_H
#ifndef UI_H
#define UI_H

#include <sys/types.h>
#include "process.h"

/* Actions utilisateur */
typedef enum {
    UI_ACTION_NONE = 0,
    UI_ACTION_HELP,      // F1
    UI_ACTION_SEARCH,    // F4
    UI_ACTION_PAUSE,     // F5
    UI_ACTION_STOP,      // F6
    UI_ACTION_KILL,      // F7
    UI_ACTION_RESTART,   // F8
    UI_ACTION_QUIT
    } ui_action_t;

/* Cycle de vie UI */
void ui_init(void);
void ui_cleanup(void);

/* Affichage */
void ui_draw_header(void);
void ui_draw_processes(process_info_t *list, int count);

/* Entrées utilisateur */
ui_action_t ui_get_action(void);
int ui_get_selected_index(void);

/* Fenêtres */
void ui_show_help(void);
void ui_show_search(char *buffer, int maxlen);

#endif
#endif //PROJETLP_UI_H