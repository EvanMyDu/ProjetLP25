#ifndef PROCESS_H
#define PROCESS_H

#include <sys/types.h>

/* Structure d'un processus */
typedef struct process_info {
    pid_t pid;
    pid_t ppid;
    char state;
    int memory;        // en kB
    int is_kernel;     // 0 ou 1
} process_info_t;

/* Récupération de la liste des processus */
int get_process_list(process_info_t **list, int *count);

/* Actions sur un processus */
int kill_process(pid_t pid);
int pause_process(pid_t pid);
int resume_process(pid_t pid);
int restart_process(pid_t pid);

/* Récupération d'un processus précis */
int get_process(pid_t pid, process_info_t *proc);

#endif
