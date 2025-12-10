#include "ui.h"
#include <ncurses.h>
#include <string.h>

int selected_index = 0;
int scroll_offset = 0;


/**
* @brief Fonction qui affiche la page help dans la console (ui)
*
* La fonction appelle ui_draw_header qui est l'entête de l'ui puis
* on montre l'ensemble des arguments possibles grâce à ncurses
*
*/
void ui_draw_help() {
    ui_draw_header();
    mvprintw(1,0,"Options:");
    mvprintw(2,0,"  -h, --help                 Affiche cette aide");
    mvprintw(4,0,"  --dry-run                  Test sans affichage");
    mvprintw(6,0,"  -c, --remote-config FILE   Configuration distante");
    mvprintw(8,0,"  -t, --connexion-type TYPE  Type de connexion");
    mvprintw(10,0,"  -P, --port PORT            Port de connexion");
    mvprintw(12,0,"  -l, --login user@host      Login distant");
    mvprintw(14,0,"  -s, --remote-server HOST   Serveur distant");
    mvprintw(16,0,"  -u, --username USER        Nom d'utilisateur");
    mvprintw(18,0,"  -p, --password PASS        Mot de passe");
    mvprintw(20,0,"  -a, --all                  Local + distant");
}


/**
* @brief Initialise l'interface utilisateur ncurses
*
* Initialise la bibliothèque ncurses et configure
* l'environnement du terminal pour une interface
* utilisateur interactive :
*  - désactive l'écho clavier -> noecho()
*  - active la lecture immédiate des touches ->cbreak
*  - autorise les touches spéciales (flèches, F1, etc.) -> keypad()
*  - masque le curseur -> curs_set
*  - configure une lecture non bloquante du clavier -> timeout()
*
* @note Cette fonction doit être appelée une seule fois
*       au démarrage du programme.
* @note L'appel à endwin() est nécessaire avant de quitter
*       le programme pour restaurer l'état du terminal. (Fonction ui_cleanup())
*
*
*/

void ui_init() {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(100);
    refresh();
}

/**
* @brief Restaure l'état du terminal à la fin du processus
*
* L'appel à endwin() permet de restaurer l'état du terminal
* à la fin de l'utilisation du programme
*
*
*/
void ui_cleanup() {
    endwin();
}
/**
* @brief Affiche l'en-tête de l'interface utilisateur
*
* Affiche l'en-tête de l'interface utilisateur grâce à ncurses,
* avec un effet de style inversant la couleur du fond et
* du texte pour faire un effet barre de menu
*/
void ui_draw_header() {
    attron(A_REVERSE);
    mvprintw(0, 0, " Localhost | F1 Help | F4 Search | F5 Pause | F6 Stop | F7 Kill | F8 Restart | Q Quit ");
    attroff(A_REVERSE);
}

/**
* @brief Retourne la mémoire totale du système en kilo-octets (mise en cache)
*
* Lit le fichier /proc/meminfo pour obtenir la mémoire totale du système.
* La valeur est mise en cache après la première lecture.
*
* @return La mémoire totale du système en kilo-octets, ou 0 en cas d'erreur
*/
long get_total_memory_kb() {
    long total_memory = 0;
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
/**
 * @brief Affiche la liste des processus dans l'interface utilisateur
 *
 * Affiche les processus fournis dans une vue ncurses avec :
 *  - une barre d'en-tête
 *  - une ligne de sélection active
 *  - un défilement vertical automatique
 *  - l'affichage des informations CPU, mémoire et temps
 *
 * La mémoire utilisée par chaque processus est affichée en pourcentage
 * de la mémoire totale du système.
 *
 * @param list Tableau de structures contenant les informations des processus
 * @param count Nombre de processus dans la liste
 */

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

/**
 * @brief Lit l'entrée utilisateur et renvoie l'action correspondante.
 *
 * Cette fonction lit la touche pressée par l'utilisateur et retourne l'action
 * correspondante sous forme de `ui_action_t`. Elle gère les actions principales
 * (aide, recherche, pause, reprise, kill, redémarrage, quitter) ainsi que la
 * navigation dans la liste via les flèches haut/bas.
 *
 * Les touches sont mappées aux actions de l'interface, incluant les touches
 * fonction (F1-F8) et certaines touches de test (lettres).
 *
 * @return ui_action_t L'action détectée, ou `UI_ACTION_NONE` si aucune action
 *         n'est associée à la touche pressée.
 */

ui_action_t ui_get_action() {
    int ch = getch();
    if (ch == ERR) return UI_ACTION_NONE;

    switch (ch) {
        case KEY_F(1): return UI_ACTION_HELP;
        case KEY_F(4): return UI_ACTION_SEARCH;
        case KEY_F(5): return UI_ACTION_PAUSE;
        case KEY_F(6): return UI_ACTION_RESUME;
        case KEY_F(7): return UI_ACTION_KILL;
        case KEY_F(8): return UI_ACTION_RESTART;

        //cases de test
        case 'p': return UI_ACTION_PAUSE;
        case 'r': return UI_ACTION_RESTART;
        case 'k': return UI_ACTION_KILL;
        case 's': return UI_ACTION_RESUME;
        case 'h': return UI_ACTION_HELP;
        case 'f': return UI_ACTION_SEARCH;

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

/**
 * @brief Retourne l'index actuellement sélectionné dans l'interface utilisateur.
 *
 * Cette fonction renvoie la valeur de l'index `selected_index`, qui est mise à jour
 * lors des interactions de l'utilisateur (par exemple avec les touches fléchées
 * haut et bas dans `ui_get_action()`).
 *
 * @return int L'index actuellement sélectionné.
 */

int ui_get_selected_index() {
    return selected_index;
}

/**
 * @brief Affiche un prompt de recherche et récupère l'entrée utilisateur.
 *
 * Cette fonction affiche "Recherche:" à l'écran et permet à l'utilisateur
 * de saisir une chaîne de caractères, qui est ensuite stockée dans le buffer fourni.
 * La saisie est limitée à "maxlen" caractères.
 *
 * @param buffer : Le buffer dans lequel stocker la chaîne saisie.
 * @param maxlen : La longueur maximale de la chaîne à saisir.
 */
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