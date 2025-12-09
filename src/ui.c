#include "ui.h"
#include <ncurses.h>
#include <string.h>

static int selected_index = 0;
static int scroll_offset = 0;

void ui_init(void) {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(100);
    refresh();
}

void ui_cleanup(void) {
    endwin();
}

void ui_draw_header(void) {
    attron(A_REVERSE);
    mvprintw(0, 0, " Localhost | F1 Help | F4 Search | F5 Pause | F6 Stop | F7 Kill | F8 Restart | Q Quit ");
    attroff(A_REVERSE);
}

// Fonction pour lire mémoire totale
static long get_total_memory_kb(void) {
    static long total_memory = 0;
    if (total_memory == 0) {
        FILE *f = fopen("/proc/meminfo", "r");
        if (f) {
            char line[256];
            while (fgets(line, sizeof(line), f)) {
                if (strstr(line, "MemTotal:")) {
                    sscanf(line, "MemTotal: %ld kB", &total_memory);
                    break;
                }
            }
            fclose(f);
        }
    }
    return total_memory;
}

void ui_draw_processes(process_info_t *list, int count) {
    erase();
    ui_draw_header();

    long total_memory_kb = get_total_memory_kb();

    mvprintw(2, 0, "PID     NAME                CPU(percent)   MEM(percent)    TIME(s)");
    mvprintw(3, 0, "------------------------------------------------------");

    int screen_height = LINES - 4;
    if (screen_height <= 0) return;

    // Corriger selected_index
    if (count > 0) {
        if (selected_index >= count) selected_index = count - 1;
        if (selected_index < 0) selected_index = 0;
    }

    // Ajuster le scroll
    if (selected_index < scroll_offset) {
        scroll_offset = selected_index;
    } else if (selected_index >= scroll_offset + screen_height) {
        scroll_offset = selected_index - screen_height + 1;
    }

    // Afficher les processus visibles
    int start = scroll_offset;
    int end = scroll_offset + screen_height;
    if (end > count) end = count;

    for (int i = start; i < end; i++) {
        int screen_line = 4 + (i - scroll_offset);

        if (i == selected_index) attron(A_REVERSE);

        // Calculer % mémoire à la volée
        float memory_percent = 0.0;
        if (total_memory_kb > 0 && list[i].memory_kb > 0) {
            memory_percent = (list[i].memory_kb * 100.0) / total_memory_kb;
        }

        mvprintw(screen_line, 0, "%-7d %-18s %6.1f%% %7.2f%% %8.1f",
                list[i].pid,
                list[i].name,
                list[i].cpu_percent,
                memory_percent,   // %.2f pour 2 décimales (mémoire change peu)
                list[i].time);

        if (i == selected_index) attroff(A_REVERSE);
    }

    // Afficher mémoire totale en bas
    if (total_memory_kb > 0) {
        float total_memory_gb = total_memory_kb / 1024.0 / 1024.0;
        mvprintw(LINES - 1, COLS - 30, "RAM: %.1f GB", total_memory_gb);
    }

    // Indicateurs de scroll
    if (scroll_offset > 0) {
        mvprintw(LINES - 1, 0, "↑ %d+", scroll_offset);
    } else if (end < count) {
        mvprintw(LINES - 1, 0, "%d+ ↓", count - end);
    }

    refresh();
}

ui_action_t ui_get_action(void) {
    int ch = getch();
    if (ch == ERR) return UI_ACTION_NONE;

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
            if (selected_index > 0) selected_index--;
            return UI_ACTION_NONE;

        case KEY_DOWN:
            selected_index++;
            return UI_ACTION_NONE;

        default:
            return UI_ACTION_NONE;
    }
}

int ui_get_selected_index(void) {
    return selected_index;
}

void ui_show_help(void) {
    timeout(-1);
    clear();
    mvprintw(2, 2, "Aide - Raccourcis clavier");
    mvprintw(4, 4, "F1  : Aide");
    mvprintw(5, 4, "F4  : Rechercher");
    mvprintw(6, 4, "F5  : Pause");
    mvprintw(7, 4, "F6  : Stop");
    mvprintw(8, 4, "F7  : Kill");
    mvprintw(9, 4, "F8  : Restart");
    mvprintw(10,4, "↑↓  : Naviguer");
    mvprintw(13,4, "Q   : Quitter");
    refresh();
    getch();
    timeout(100);
}

void ui_show_search(char *buffer, int maxlen) {
    timeout(-1);
    echo();
    curs_set(1);

    mvprintw(LINES - 2, 0, "Recherche: ");
    getnstr(buffer, maxlen);

    noecho();
    curs_set(0);
    timeout(100);
}