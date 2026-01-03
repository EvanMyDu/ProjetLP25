#ifndef PROJETLP_PROCESS_H
#define PROJETLP_PROCESS_H

typedef struct {
    int pid;
    char name[256];
    float cpu_percent;
    int memory_kb;
    float time;
    char state;
    int ppid;
    int is_kernel;
} process_info_t;

// Fonctions existantes
int get_process_list(process_info_t **list, int *count);
int kill_process(int pid);
int pause_process(int pid);
int resume_process(int pid);
int restart_process(int pid);
int get_process(int pid, process_info_t *proc);

// Nouvelle fonction abstraite
typedef int (*process_fetcher_t)(void *context, process_info_t **list, int *count);

#endif // PROJETLP_PROCESS_H