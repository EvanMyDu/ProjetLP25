#include "manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../header/ui.h"
#include "../header/process.h"
#include "ncurses.h"

void manager_run(int dry_run) {
    if (dry_run) {
        printf("[DRY RUN] Mode test activé - Aucune action ne sera exécutée\n");
        printf("[DRY RUN] Simulation de l'accès aux processus...\n");
        printf("[DRY RUN] Connexion aux services distants simulée\n");
        printf("[DRY RUN] Opération terminée avec succès (mode test)\n");
        return;
    }
}

int get_selected_pid(process_info_t *list, int count) {
    int selected_index = ui_get_selected_index();

    // Vérifier que l'index est valide
    if (selected_index >= 0 && selected_index < count && list != NULL) {
        return list[selected_index].pid;
    }

    return -1;  // Erreur : pas de processus sélectionné
}

void manager(int options) {
    ui_init();
    if (options != 0) {
        ui_draw_help();
    }
    process_info_t *process_list = NULL;
    int process_count = 0;
    int running = 1;
    int refresh_counter = 0;
    while (running) {
        while (options != 0) {
            ui_action_t action = ui_get_action();
            switch (action) {
                case UI_ACTION_HELP:
                    erase();
                    options = 0;
                default:
                    break;
            }
        }
        // Rafraîchir la liste toutes les 10 itérations
        if (refresh_counter % 10 == 0) {
            process_info_t *new_list = NULL;
            int new_count = 0;

            if (get_process_list(&new_list, &new_count) == 0) {
                if (process_list) free(process_list);
                process_list = new_list;
                process_count = new_count;
            }
        }

        // Affichage
        ui_draw_processes(process_list, process_count);

        // Gestion des touches
        ui_action_t action = ui_get_action();
        switch (action) {
            case UI_ACTION_PAUSE:
                pause_process(get_selected_pid(process_list, process_count));
                break;
            case UI_ACTION_RESTART:
                restart_process(get_selected_pid(process_list, process_count));
                break;
            case UI_ACTION_RESUME:
                resume_process(get_selected_pid(process_list, process_count));
                break;
            case UI_ACTION_KILL:
                kill_process(get_selected_pid(process_list, process_count));
                break;
            case UI_ACTION_QUIT:
                running = 0;
                break;

            case UI_ACTION_HELP:
                erase();
                ui_draw_help();
                options = 1;
                break;

            case UI_ACTION_SEARCH: {
                char buffer[256];
                ui_show_search(buffer, sizeof(buffer));
                break;
            }

            default:
                // Les flèches sont gérées dans ui_get_action()
                break;
        }

        refresh_counter++;
        usleep(50000);  // 50ms de pause
    }

    // Nettoyage
    if (process_list) free(process_list);
    ui_cleanup();
}