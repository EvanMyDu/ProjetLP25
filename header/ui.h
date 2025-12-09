#ifndef UI_H
#define UI_H

#include "process.h"

void ui_draw_help(void);

/* Actions utilisateur */
typedef enum {
    UI_ACTION_NONE = 0,
    UI_ACTION_HELP,
    UI_ACTION_SEARCH,
    UI_ACTION_PAUSE,
    UI_ACTION_RESUME,
    UI_ACTION_KILL,
    UI_ACTION_RESTART,
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