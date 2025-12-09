#ifndef PROCESS_H
#define PROCESS_H


/* Structure d'un processus */
typedef struct process_info {
    int pid;
    int ppid;
    char state;
    int memory_kb;      // mémoire en kilo-octets
    int is_kernel;      // 0 ou 1
    char name[256];     // nom du processus
    float cpu_percent;  // utilisation CPU en %
    float time;         // temps écoulé en secondes
} process_info_t;

/* Récupération de la liste des processus */
int get_process_list(process_info_t **list, int *count);

/* Actions sur un processus */
int kill_process(int pid);
int pause_process(int pid);
int resume_process(int pid);
int restart_process(int pid);

/* Récupération d'un processus précis */
int get_process(int pid, process_info_t *proc);

#endif