//
// Created by evanmd25 on 2025-12-08.
//

#include "ui.h"
#include <ncurses.h>
#include <string.h>

static int selected_index = 0;

/* =============================
 * Initialisation / nettoyage
 * ============================= */
void ui_init(void) {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    refresh();
}

void ui_cleanup(void) {
    endwin();
}

/* =============================
 * Affichage
 * ============================= */
void ui_draw_header(void) {
    attron(A_REVERSE);
    mvprintw(0, 0,
    " Localhost | F1 Help | F4 Search | F5 Pause | F6 Stop | F7 Kill | F8 Restart | Q Quit ");
    attroff(A_REVERSE);
    clrtoeol();
}

void ui_draw_processes(process_info_t *list, int count) {
    clear();
    ui_draw_header();

    mvprintw(2, 0, "PID     NAME                CPU     MEM     TIME");
    mvprintw(3, 0, "--------------------------------------------------");

    for (int i = 0; i < count; i++) {
        if (i == selected_index) {
            attron(A_REVERSE);
        }

        mvprintw(4 + i, 0, "%-7d %-18s %5.1f%% %5.1f%% %ld",
        list[i].pid,
        list[i].name,
        list[i].cpu,
        list[i].memory,
        list[i].time);

        if (i == selected_index) {
            attroff(A_REVERSE);
        }
    }

    refresh();
}

/* =============================
 * Gestion clavier
 * ============================= */
ui_action_t ui_get_action(void) {
    int ch = getch();

    switch (ch) {
        case KEY_F(1): return UI_ACTION_HELP;
        case KEY_F(4): return UI_ACTION_SEARCH;
        case KEY_F(5): return UI_ACTION_PAUSE;
        case KEY_F(6): return UI_ACTION_STOP;
        case KEY_F(7): return UI_ACTION_KILL;
        case KEY_F(8): return UI_ACTION_RESTART;
        case 'q':
        case 'Q': return UI_ACTION_QUIT;

        case KEY_UP:
            if (selected_index > 0)
                selected_index--;
            break;

        case KEY_DOWN:
            selected_index++;
            break;

        default:
            break;
    }

    return UI_ACTION_NONE;
}

int ui_get_selected_index(void) {
    return selected_index;
}

/* =============================
 * Fenêtres secondaires
 * ============================= */
void ui_show_help(void) {
    clear();
    mvprintw(2, 2, "Aide - Raccourcis clavier");
    mvprintw(4, 4, "F1  : Afficher l'aide");
    mvprintw(5, 4, "F4  : Rechercher un processus");
    mvprintw(6, 4, "F5  : Mettre en pause un processus");
    mvprintw(7, 4, "F6  : Arreter un processus");
    mvprintw(8, 4, "F7  : Tuer un processus");
    mvprintw(9, 4, "F8  : Redemarrer un processus");
    mvprintw(11,4, "Q   : Quitter");

    mvprintw(13,4, "Appuyez sur une touche pour revenir...");
    refresh();
    getch();
}

void ui_show_search(char *buffer, int maxlen) {
    echo();
    curs_set(1);

    mvprintw(LINES - 2, 0, "Recherche : ");
    clrtoeol();
    getnstr(buffer, maxlen);

    noecho();
    curs_set(0);
}