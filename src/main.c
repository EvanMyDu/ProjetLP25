#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>

#include "../header/manager.h"
#include "../header/ui.h"

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

    if (options.dry_run) {
        printf("Mode test activé\n");
        return 0;
    }
    manager(options.show_help);
    return 0;
}