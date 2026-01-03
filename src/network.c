#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctype.h>

/**
 * @brief Exécute une commande shell et capture sa sortie
 *
 * Cette fonction utilise popen() pour exécuter une commande système
 * et récupérer sa sortie standard dans un buffer.
 *
 * @param cmd Commande shell à exécuter
 * @param output Buffer pour stocker la sortie de la commande
 * @param output_size Taille maximale du buffer de sortie
 * @return Code de retour de la commande (0 pour succès), -1 en cas d'erreur
 */
static int run_command(const char *cmd, char *output, int output_size) {
    FILE *fp = popen(cmd, "r");
    if (!fp) return -1;

    output[0] = '\0';
    char buffer[256];
    size_t total = 0;
    size_t output_size_t = (size_t)output_size;  // Conversion pour comparaison

    while (fgets(buffer, sizeof(buffer), fp) != NULL && total < output_size_t - 1) {
        size_t len = strlen(buffer);
        if (total + len < output_size_t) {
            strcat(output, buffer);
            total += len;
        }
    }

    int status = pclose(fp);
    return WEXITSTATUS(status);
}

/**
 * @brief Exécute une commande sur une machine distante via SSH
 *
 * Utilise la commande ssh (ou sshpass si mot de passe fourni) pour
 * exécuter une commande sur un hôte distant.
 *
 * @param host Adresse de l'hôte distant
 * @param port Port SSH (généralement 22)
 * @param username Nom d'utilisateur pour la connexion
 * @param password Mot de passe (peut être NULL pour utiliser les clés SSH)
 * @param command Commande à exécuter sur l'hôte distant
 * @param output Buffer pour stocker la sortie de la commande
 * @param output_size Taille maximale du buffer de sortie
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int ssh_execute(const char *host, int port, const char *username, const char *password,
                const char *command, char *output, int output_size) {
    char cmd[1024];

    // Si mot de passe fourni, utiliser sshpass
    if (password != NULL && strlen(password) > 0) {
        // Vérifier si sshpass est disponible
        if (system("which sshpass > /dev/null 2>&1") != 0) {
            fprintf(stderr, "Warning: sshpass non installé. Installation: sudo apt-get install sshpass\n");
            return -1;
        }

        snprintf(cmd, sizeof(cmd),
                 "sshpass -p '%s' ssh -o StrictHostKeyChecking=no -o ConnectTimeout=3 "
                 "-p %d %s@%s '%s' 2>/dev/null",
                 password, port, username, host, command);
    } else {
        // Sans mot de passe (utilise les clés SSH par défaut)
        snprintf(cmd, sizeof(cmd),
                 "ssh -o StrictHostKeyChecking=no -o ConnectTimeout=3 "
                 "-p %d %s@%s '%s' 2>/dev/null",
                 port, username, host, command);
    }

    return run_command(cmd, output, output_size);
}

/**
 * @brief Exécute une commande sur une machine distante via Telnet
 *
 * Utilise Telnet avec un script Expect pour automatiser la connexion
 * et l'exécution de commandes.
 *
 * @param host Adresse de l'hôte distant
 * @param port Port Telnet (généralement 23)
 * @param username Nom d'utilisateur pour la connexion
 * @param password Mot de passe pour la connexion
 * @param command Commande à exécuter sur l'hôte distant
 * @param output Buffer pour stocker la sortie de la commande
 * @param output_size Taille maximale du buffer de sortie
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int telnet_execute(const char *host, int port, const char *username, const char *password,
                   const char *command, char *output, int output_size) {
    // Vérifier si expect est disponible
    if (system("which expect > /dev/null 2>&1") != 0) {
        fprintf(stderr, "Warning: expect non installé. Installation: sudo apt-get install expect\n");
        return -1;
    }

    // Créer un script expect temporaire
    char script_path[] = "/tmp/telnet_script_XXXXXX";
    int fd = mkstemp(script_path);
    if (fd == -1) return -1;

    char script[1024];
    snprintf(script, sizeof(script),
             "#!/usr/bin/expect -f\n"
             "set timeout 5\n"
             "spawn telnet %s %d\n"
             "expect \"login:\"\n"
             "send \"%s\\r\"\n"
             "expect \"Password:\"\n"
             "send \"%s\\r\"\n"
             "expect {\n"
             "  \">\" {\n"
             "    send \"%s\\r\"\n"
             "    expect \">\"\n"
             "    send \"exit\\r\"\n"
             "    expect eof\n"
             "  }\n"
             "  timeout {\n"
             "    exit 1\n"
             "  }\n"
             "}\n", host, port, username, password, command);

    write(fd, script, strlen(script));
    close(fd);
    chmod(script_path, 0700);

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s 2>/dev/null | tail -n +3", script_path);

    int result = run_command(cmd, output, output_size);

    // Nettoyer le script temporaire
    unlink(script_path);

    return result;
}

/**
 * @brief Parse un fichier de configuration réseau
 *
 * Lit un fichier texte au format "name:address:port:username:password:type"
 * pour configurer les connexions aux hôtes distants.
 *
 * @param filename Chemin vers le fichier de configuration
 * @param hosts Tableau pour stocker les configurations d'hôtes
 * @param max_hosts Nombre maximum d'hôtes pouvant être stockés
 * @return Nombre d'hôtes configurés, ou -1 en cas d'erreur
 */
int parse_config_file(const char *filename, host_config_t *hosts, int max_hosts) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        // Fichier non trouvé, créer localhost par défaut
        strcpy(hosts[0].name, "localhost");
        strcpy(hosts[0].address, "127.0.0.1");
        hosts[0].port = 22;
        hosts[0].type = CONNECTION_LOCAL;
        hosts[0].is_local = 1;
        hosts[0].username[0] = '\0';
        hosts[0].password[0] = '\0';
        return 1;
    }

    char line[512];
    int count = 0;

    // Ajouter localhost en premier
    strcpy(hosts[count].name, "localhost");
    strcpy(hosts[count].address, "127.0.0.1");
    hosts[count].port = 0;
    hosts[count].type = CONNECTION_LOCAL;
    hosts[count].is_local = 1;
    hosts[count].username[0] = '\0';
    hosts[count].password[0] = '\0';
    count++;

    while (fgets(line, sizeof(line), file) && count < max_hosts) {
        // Ignorer lignes vides et commentaires
        if (line[0] == '\n' || line[0] == '#' || line[0] == '\r') continue;

        // Supprimer le retour à la ligne
        line[strcspn(line, "\n\r")] = '\0';

        // Parser : name:address:port:username:password:type
        char *tokens[6];
        char *saveptr;
        int token_count = 0;

        char *token = strtok_r(line, ":", &saveptr);
        while (token && token_count < 6) {
            tokens[token_count++] = token;
            token = strtok_r(NULL, ":", &saveptr);
        }

        if (token_count < 6) continue;

        // Remplir la structure
        strncpy(hosts[count].name, tokens[0], sizeof(hosts[count].name) - 1);
        strncpy(hosts[count].address, tokens[1], sizeof(hosts[count].address) - 1);
        hosts[count].port = atoi(tokens[2]);
        if (hosts[count].port <= 0) hosts[count].port = 22; // Port par défaut

        strncpy(hosts[count].username, tokens[3], sizeof(hosts[count].username) - 1);
        strncpy(hosts[count].password, tokens[4], sizeof(hosts[count].password) - 1);

        if (strcmp(tokens[5], "ssh") == 0) {
            hosts[count].type = CONNECTION_SSH;
        } else if (strcmp(tokens[5], "telnet") == 0) {
            hosts[count].type = CONNECTION_TELNET;
        } else {
            hosts[count].type = CONNECTION_LOCAL;
        }

        hosts[count].is_local = 0;
        count++;
    }

    fclose(file);
    return count;
}

/**
 * @brief Récupère la liste des processus d'un hôte distant
 *
 * Exécute la commande 'ps' sur l'hôte distant et parse sa sortie
 * pour construire une liste de processus.
 *
 * @param host Configuration de l'hôte distant
 * @param list Pointeur vers un tableau qui contiendra la liste des processus
 * @param count Pointeur vers un entier qui contiendra le nombre de processus
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int get_remote_process_list(const host_config_t *host, process_info_t **list, int *count) {
    if (host->is_local) {
        // Utiliser la fonction locale
        extern int get_process_list(process_info_t **list, int *count);
        return get_process_list(list, count);
    }

    // Commande pour récupérer les processus (compatible Linux)
    // Format simple: PID, CPU%, MEM%, COMMAND
    const char *cmd = "ps -eo pid,pcpu,pmem,comm --no-headers --sort=-pcpu | head -50";
    char output[8192];

    int result;
    if (host->type == CONNECTION_SSH) {
        result = ssh_execute(host->address, host->port, host->username,
                           host->password, cmd, output, sizeof(output));
    } else {
        result = telnet_execute(host->address, host->port, host->username,
                              host->password, cmd, output, sizeof(output));
    }

    if (result != 0) {
        // Fallback: commande plus simple
        const char *simple_cmd = "ps aux --no-headers | head -50";
        if (host->type == CONNECTION_SSH) {
            result = ssh_execute(host->address, host->port, host->username,
                               host->password, simple_cmd, output, sizeof(output));
        } else {
            result = telnet_execute(host->address, host->port, host->username,
                                  host->password, simple_cmd, output, sizeof(output));
        }

        if (result != 0) return -1;
    }

    // Parser la sortie
    char *lines[100];  // Max 100 processus
    int line_count = 0;
    char *saveptr;

    char *line = strtok_r(output, "\n", &saveptr);
    while (line && line_count < 100) {
        lines[line_count] = line;
        line_count++;
        line = strtok_r(NULL, "\n", &saveptr);
    }

    // Allouer mémoire pour les processus
    *list = malloc(sizeof(process_info_t) * line_count);
    if (!*list) return -1;

    for (int i = 0; i < line_count; i++) {
        process_info_t *proc = &(*list)[i];
        memset(proc, 0, sizeof(process_info_t));

        // Initialiser avec des valeurs par défaut
        proc->pid = i + 1;
        proc->cpu_percent = 0.0f;
        proc->memory_kb = 0;
        proc->time = 0.0f;
        proc->state = 'R';
        proc->ppid = 1;
        proc->is_kernel = 0;

        // Tenter différents formats de parsing
        char name_buf[256] = {0};
        float cpu_temp = 0.0f;
        float mem_temp = 0.0f;
        int pid_temp = 0;

        // Format 1: ps -eo pid,pcpu,pmem,comm
        if (sscanf(lines[i], "%d %f %f %255s",
                   &pid_temp, &cpu_temp, &mem_temp, name_buf) >= 4) {
            proc->pid = pid_temp;
            proc->cpu_percent = cpu_temp;
            // Convertir % de mémoire en kB (approximation)
            proc->memory_kb = (int)(mem_temp * 1024);
            strncpy(proc->name, name_buf, sizeof(proc->name) - 1);
        }
        // Format 2: ps aux (plus courant)
        else if (sscanf(lines[i], "%*s %d %*f %f %f %*s %*s %*s %*s %*s %255s",
                       &pid_temp, &cpu_temp, &mem_temp, name_buf) >= 4) {
            proc->pid = pid_temp;
            proc->cpu_percent = cpu_temp;
            proc->memory_kb = (int)(mem_temp * 1024);
            strncpy(proc->name, name_buf, sizeof(proc->name) - 1);
        }
        // Format 3: juste le nom
        else {
            sscanf(lines[i], "%255s", name_buf);
            strncpy(proc->name, name_buf, sizeof(proc->name) - 1);
        }

        // Nettoyer le nom si nécessaire
        char *newline = strchr(proc->name, '\n');
        if (newline) *newline = '\0';
    }

    *count = line_count;
    return 0;
}

/**
 * @brief Tue un processus sur un hôte distant
 *
 * @param host Configuration de l'hôte distant
 * @param pid PID du processus à tuer
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int remote_kill_process(const host_config_t *host, int pid) {
    if (host->is_local) {
        extern int kill_process(int pid);
        return kill_process(pid);
    }

    if (pid <= 0) return -1;

    char command[64];
    snprintf(command, sizeof(command), "kill -9 %d 2>/dev/null; echo $?", pid);
    char output[256];

    int result;
    if (host->type == CONNECTION_SSH) {
        result = ssh_execute(host->address, host->port, host->username,
                           host->password, command, output, sizeof(output));
    } else {
        result = telnet_execute(host->address, host->port, host->username,
                              host->password, command, output, sizeof(output));
    }

    if (result != 0) return -1;

    // Vérifier le code de retour
    int exit_code = atoi(output);
    return (exit_code == 0) ? 0 : -1;
}

/**
 * @brief Met en pause un processus sur un hôte distant
 *
 * @param host Configuration de l'hôte distant
 * @param pid PID du processus à mettre en pause
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int remote_pause_process(const host_config_t *host, int pid) {
    if (host->is_local) {
        extern int pause_process(int pid);
        return pause_process(pid);
    }

    if (pid <= 0) return -1;

    char command[64];
    snprintf(command, sizeof(command), "kill -STOP %d 2>/dev/null; echo $?", pid);
    char output[256];

    int result;
    if (host->type == CONNECTION_SSH) {
        result = ssh_execute(host->address, host->port, host->username,
                           host->password, command, output, sizeof(output));
    } else {
        result = telnet_execute(host->address, host->port, host->username,
                              host->password, command, output, sizeof(output));
    }

    if (result != 0) return -1;

    int exit_code = atoi(output);
    return (exit_code == 0) ? 0 : -1;
}

/**
 * @brief Reprend un processus sur un hôte distant
 *
 * @param host Configuration de l'hôte distant
 * @param pid PID du processus à reprendre
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int remote_resume_process(const host_config_t *host, int pid) {
    if (host->is_local) {
        extern int resume_process(int pid);
        return resume_process(pid);
    }

    if (pid <= 0) return -1;

    char command[64];
    snprintf(command, sizeof(command), "kill -CONT %d 2>/dev/null; echo $?", pid);
    char output[256];

    int result;
    if (host->type == CONNECTION_SSH) {
        result = ssh_execute(host->address, host->port, host->username,
                           host->password, command, output, sizeof(output));
    } else {
        result = telnet_execute(host->address, host->port, host->username,
                              host->password, command, output, sizeof(output));
    }

    if (result != 0) return -1;

    int exit_code = atoi(output);
    return (exit_code == 0) ? 0 : -1;
}

/**
 * @brief Redémarre un processus sur un hôte distant
 *
 * Envoie SIGTERM puis SIGCONT au processus pour simuler un redémarrage.
 *
 * @param host Configuration de l'hôte distant
 * @param pid PID du processus à redémarrer
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int remote_restart_process(const host_config_t *host, int pid) {
    if (host->is_local) {
        extern int restart_process(int pid);
        return restart_process(pid);
    }

    if (pid <= 0) return -1;

    char command[128];
    snprintf(command, sizeof(command),
             "kill -TERM %d 2>/dev/null; sleep 0.5; kill -CONT %d 2>/dev/null; echo $?",
             pid, pid);
    char output[256];

    int result;
    if (host->type == CONNECTION_SSH) {
        result = ssh_execute(host->address, host->port, host->username,
                           host->password, command, output, sizeof(output));
    } else {
        result = telnet_execute(host->address, host->port, host->username,
                              host->password, command, output, sizeof(output));
    }

    if (result != 0) return -1;

    int exit_code = atoi(output);
    return (exit_code == 0) ? 0 : -1;
}

/**
 * @brief Initialise le gestionnaire réseau
 *
 * Charge la configuration depuis un fichier et prépare les structures
 * pour les connexions distantes.
 *
 * @param manager Pointeur vers la structure network_manager_t à initialiser
 * @param config_file Chemin vers le fichier de configuration
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int network_init(network_manager_t *manager, const char *config_file) {
    if (!manager) return -1;

    memset(manager, 0, sizeof(network_manager_t));

    manager->hosts = malloc(sizeof(host_config_t) * MAX_HOSTS);
    if (!manager->hosts) return -1;

    int count = parse_config_file(config_file, manager->hosts, MAX_HOSTS);
    if (count <= 0) {
        free(manager->hosts);
        manager->hosts = NULL;
        return -1;
    }

    manager->count = count;
    manager->current_host = 0;
    return 0;
}

/**
 * @brief Nettoie les ressources du gestionnaire réseau
 *
 * Libère la mémoire allouée pour les configurations d'hôtes.
 *
 * @param manager Pointeur vers la structure network_manager_t à nettoyer
 */
void network_cleanup(network_manager_t *manager) {
    if (manager && manager->hosts) {
        free(manager->hosts);
        manager->hosts = NULL;
    }
    if (manager) {
        manager->count = 0;
        manager->current_host = 0;
    }
}