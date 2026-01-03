#include "manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "../header/ui.h"
#include "../header/process.h"
#include "../header/network.h"
#include "ncurses.h"

// Ajouter ces variables globales
static network_manager_t network_manager;
static int use_network = 0;

void manager_run() {
    printf("[DRY RUN] Mode test activé - Aucune action ne sera exécutée\n");
    printf("[DRY RUN] Simulation de l'accès aux processus...\n");

    // Tester la configuration réseau
    host_config_t test_hosts[MAX_HOSTS];
    int host_count = parse_config_file(".config", test_hosts, MAX_HOSTS);
    if (host_count > 0) {
        printf("[DRY RUN] Configuration réseau détectée: %d hôte(s)\n", host_count);
        for (int i = 0; i < host_count; i++) {
            printf("[DRY RUN] Hôte %d: %s (%s)\n", i, test_hosts[i].name, test_hosts[i].address);
        }
    }

    printf("[DRY RUN] Connexion aux services distants simulée\n");
    printf("[DRY RUN] Opération terminée avec succès (mode test)\n");
}

int get_selected_pid(process_info_t *list, int count) {
    // Fonction inchangée
    int selected_index = ui_get_selected_index();
    if (selected_index >= 0 && selected_index < count && list != NULL) {
        return list[selected_index].pid;
    }
    return -1;
}

void manager(int options) {
    ui_init();

    // Initialiser le réseau si config fournie
    use_network = 0;
    const char *config_file = ".config"; // À récupérer des options
    if (network_init(&network_manager, config_file) == 0 && network_manager.count > 1) {
        use_network = 1;
    }

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
            if (action == UI_ACTION_HELP) {
                erase();
                options = 0;
            }
        }

        // Rafraîchir la liste toutes les 10 itérations
        if (refresh_counter % 10 == 0) {
            process_info_t *new_list = NULL;
            int new_count = 0;

            if (use_network) {
                // Récupérer les processus de l'hôte courant
                get_remote_process_list(&network_manager.hosts[network_manager.current_host],
                                      &new_list, &new_count);
            } else {
                // Mode local uniquement
                get_process_list(&new_list, &new_count);
            }

            if (process_list) free(process_list);
            process_list = new_list;
            process_count = new_count;
        }

        // Afficher l'en-tête avec le nom de l'hôte
        char header[256];
        if (use_network) {
            snprintf(header, sizeof(header), " %s | F1 Help | F2 Next | F3 Prev | F4 Search | F5 Pause | F6 Stop | F7 Kill | F8 Restart | Q Quit ",
                    network_manager.hosts[network_manager.current_host].name);
        } else {
            strcpy(header, " Localhost | F1 Help | F2 Next | F3 Prev | F4 Search | F5 Pause | F6 Stop | F7 Kill | F8 Restart | Q Quit ");
        }

        attron(A_REVERSE);
        mvprintw(0, 0, "%-*s", COLS, header);
        attroff(A_REVERSE);

        // Afficher les processus
        ui_draw_processes(process_list, process_count);

        // Gestion des touches
        ui_action_t action = ui_get_action();
        switch (action) {
            case UI_ACTION_PAUSE:
                if (use_network) {
                    // Implémenter remote_pause_process
                } else {
                    pause_process(get_selected_pid(process_list, process_count));
                }
                break;

            case UI_ACTION_RESTART:
                if (use_network) {
                    // Implémenter remote_restart_process
                } else {
                    restart_process(get_selected_pid(process_list, process_count));
                }
                break;

            case UI_ACTION_RESUME:
                if (use_network) {
                    // Implémenter remote_resume_process
                } else {
                    resume_process(get_selected_pid(process_list, process_count));
                }
                break;

            case UI_ACTION_KILL:
                if (use_network) {
                    remote_kill_process(&network_manager.hosts[network_manager.current_host],
                                      get_selected_pid(process_list, process_count));
                } else {
                    kill_process(get_selected_pid(process_list, process_count));
                }
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

            // Navigation entre hôtes (F2/F3)
            case UI_ACTION_NEXT_HOST:
                if (use_network) {
                    network_manager.current_host = (network_manager.current_host + 1) % network_manager.count;
                    // Forcer le rafraîchissement
                    refresh_counter = 9;
                }
                break;

            case UI_ACTION_PREV_HOST:
                if (use_network) {
                    network_manager.current_host = (network_manager.current_host - 1 + network_manager.count) % network_manager.count;
                    refresh_counter = 9;
                }
                break;

            default:
                break;
        }

        refresh_counter++;
        usleep(50000);
    }

    // Nettoyage
    if (process_list) free(process_list);
    if (use_network) {
        network_cleanup(&network_manager);
    }
    ui_cleanup();
}