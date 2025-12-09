#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>

#include "../header/ui.h"
#include "../header/process.h"

typedef struct program_options {
    int show_help;
    int dry_run;
    char *remote_config;
    char *connection_type;
    int port;
    char *login;
    char *remote_server;
    char *username;
    char *password;
    int all;
} program_options_t;

void print_help(void) {
    printf("Usage: ./lp25 [options]\n\n");
    printf("Options:\n");
    printf("  -h, --help                 Affiche cette aide\n");
    printf("  --dry-run                  Test sans affichage\n");
    printf("  -c, --remote-config FILE   Configuration distante\n");
    printf("  -t, --connexion-type TYPE  Type de connexion\n");
    printf("  -P, --port PORT            Port de connexion\n");
    printf("  -l, --login user@host      Login distant\n");
    printf("  -s, --remote-server HOST   Serveur distant\n");
    printf("  -u, --username USER        Nom d'utilisateur\n");
    printf("  -p, --password PASS        Mot de passe\n");
    printf("  -a, --all                  Local + distant\n");
}

int main(int argc, char **argv) {
    program_options_t options;
    memset(&options, 0, sizeof(options));
    options.port = -1;

    struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"dry-run", no_argument, 0, 1},
        {"remote-config", required_argument, 0, 'c'},
        {"connexion-type", required_argument, 0, 't'},
        {"port", required_argument, 0, 'P'},
        {"login", required_argument, 0, 'l'},
        {"remote-server", required_argument, 0, 's'},
        {"username", required_argument, 0, 'u'},
        {"password", required_argument, 0, 'p'},
        {"all", no_argument, 0, 'a'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "hc:t:P:l:s:u:p:a", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'h':
                options.show_help = 1;
                break;
            case 1:
                options.dry_run = 1;
                break;
            case 'c':
                options.remote_config = optarg;
                break;
            case 't':
                options.connection_type = optarg;
                break;
            case 'P':
                options.port = atoi(optarg);
                break;
            case 'l':
                options.login = optarg;
                break;
            case 's':
                options.remote_server = optarg;
                break;
            case 'u':
                options.username = optarg;
                break;
            case 'p':
                options.password = optarg;
                break;
            case 'a':
                options.all = 1;
                break;
            default:
                printf("Option inconnue. Utilisez --help.\n");
                return 1;
        }
    }

    if (options.show_help) {
        print_help();
        return 0;
    }

    if (options.dry_run) {
        printf("Mode test activé\n");
        return 0;
    }

    // Initialisation
    process_info_t *process_list = NULL;
    int process_count = 0;
    ui_init();

    int running = 1;
    int refresh_counter = 0;

    while (running) {
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
            case UI_ACTION_QUIT:
                running = 0;
                break;

            case UI_ACTION_HELP:
                ui_show_help();
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

    return 0;
}