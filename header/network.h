#ifndef PROJETLP_NETWORK_H
#define PROJETLP_NETWORK_H

#include "process.h"

#define MAX_HOSTS 10
#define BUFFER_SIZE 4096

typedef enum {
    CONNECTION_SSH,
    CONNECTION_TELNET,
    CONNECTION_LOCAL
} connection_type_t;

typedef struct {
    char name[64];
    char address[128];
    int port;
    char username[64];
    char password[64];
    connection_type_t type;
    int is_local;  // 1 pour localhost, 0 pour distant
} host_config_t;

typedef struct {
    host_config_t *hosts;
    int count;
    int current_host;  // Index de l'hôte actuellement affiché
} network_manager_t;

// Initialisation réseau
int network_init(network_manager_t *manager, const char *config_file);
void network_cleanup(network_manager_t *manager);

// Gestion des connexions
int connect_to_host(const host_config_t *host);
int disconnect_from_host(const host_config_t *host);

// Récupération des processus distants
int get_remote_process_list(const host_config_t *host, process_info_t **list, int *count);

// Exécution de commandes distantes
int execute_remote_command(const host_config_t *host, const char *command, char *output, int output_size);

// Gestion des actions sur processus distants
int remote_kill_process(const host_config_t *host, int pid);
int remote_pause_process(const host_config_t *host, int pid);
int remote_resume_process(const host_config_t *host, int pid);
int remote_restart_process(const host_config_t *host, int pid);

// Parsing de configuration
int parse_config_file(const char *filename, host_config_t *hosts, int max_hosts);

// SSH spécifique
int ssh_execute(const char *host, int port, const char *username, const char *password,
                const char *command, char *output, int output_size);

// Telnet spécifique
int telnet_execute(const char *host, int port, const char *username, const char *password,
                   const char *command, char *output, int output_size);

#endif // PROJETLP_NETWORK_H