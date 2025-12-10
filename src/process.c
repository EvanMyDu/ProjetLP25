#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#include "process.h"

// Structure pour stocker les échantillons CPU
typedef struct {
    int pid;
    unsigned long long last_cpu_time;
    struct timespec last_sample_time;
} cpu_sample_t;

cpu_sample_t *cpu_samples = NULL;
int cpu_sample_count = 0;
unsigned long long previous_total_cpu = 0;
struct timespec previous_sample_time;
long total_system_memory_kb = 0;

/**
* @brief Lit et retourne la mémoire totale du système
*
* Cette fonction lit le fichier /proc/meminfo pour obtenir la mémoire totale
* du système. La valeur est mise en cache après la première lecture.
*
* @return La mémoire totale du système en kilo-octets, ou 0 en cas d'erreur
*/
long get_total_system_memory(void) {
     long cached_total_memory = 0;

    if (cached_total_memory == 0) {
        FILE *f = fopen("/proc/meminfo", "r");
        if (!f) return 0;

        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strstr(line, "MemTotal:")) {
                sscanf(line, "MemTotal: %ld kB", &cached_total_memory);
                break;
            }
        }
        fclose(f);
    }

    return cached_total_memory;
}

/**
* @brief Recherche un échantillon CPU pour un PID donné
*
* Parcourt le tableau des échantillons CPU pour trouver celui correspondant
* au PID spécifié.
*
* @param pid Le PID du processus à rechercher
* @return Pointeur vers l'échantillon CPU, ou NULL si non trouvé
*/
cpu_sample_t* find_cpu_sample(int pid) {
    for (int i = 0; i < cpu_sample_count; i++) {
        if (cpu_samples[i].pid == pid) {
            return &cpu_samples[i];
        }
    }
    return NULL;
}

/**
* @brief Met à jour ou crée un échantillon CPU pour un processus
*
* Si un échantillon existe déjà pour le PID, il est mis à jour avec les
* nouvelles valeurs. Sinon, un nouvel échantillon est créé.
*
* @param pid Le PID du processus
* @param cpu_time Le temps CPU total du processus
*/
void update_cpu_sample(int pid, unsigned long long cpu_time) {
    for (int i = 0; i < cpu_sample_count; i++) {
        if (cpu_samples[i].pid == pid) {
            cpu_samples[i].last_cpu_time = cpu_time;
            clock_gettime(CLOCK_MONOTONIC, &cpu_samples[i].last_sample_time);
            return;
        }
    }

    // Nouvel échantillon
    cpu_sample_t *tmp = realloc(cpu_samples, (cpu_sample_count + 1) * sizeof(cpu_sample_t));
    if (!tmp) return;

    cpu_samples = tmp;
    cpu_samples[cpu_sample_count].pid = pid;
    cpu_samples[cpu_sample_count].last_cpu_time = cpu_time;
    clock_gettime(CLOCK_MONOTONIC, &cpu_samples[cpu_sample_count].last_sample_time);
    cpu_sample_count++;
}

/**
* @brief Lit le temps CPU total du système depuis /proc/stat
*
* Cette fonction lit le fichier /proc/stat pour obtenir le temps CPU total
* du système (somme de user, nice, system, idle, iowait, irq, softirq).
*
* @return Le temps CPU total du système en ticks
*/
unsigned long long read_total_cpu_time(void) {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return 0;

    unsigned long long user, nice, system, idle, iowait, irq, softirq;
    fscanf(f, "cpu %llu %llu %llu %llu %llu %llu %llu",
           &user, &nice, &system, &idle, &iowait, &irq, &softirq);
    fclose(f);

    return user + nice + system + idle + iowait + irq + softirq;
}

/**
* @brief Lit les temps CPU utime et stime d'un processus
*
* Lit le fichier /proc/[pid]/stat pour obtenir les temps CPU en mode
* utilisateur (utime) et mode système (stime) d'un processus.
*
* @param pid Le PID du processus
* @param utime Pointeur pour stocker le temps utilisateur
* @param stime Pointeur pour stocker le temps système
* @return 0 en cas de succès, -1 en cas d'erreur
*/
int read_process_cpu_ticks(int pid, unsigned long *utime, unsigned long *stime) {
    char path[256];
    FILE *f;

    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    f = fopen(path, "r");
    if (!f) return -1;

    char line[1024];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return -1;
    }
    fclose(f);

    // Parse manuel
    int field = 1;
    char *ptr = line;
    char *start = line;
    *utime = 0;
    *stime = 0;

    while (*ptr && field <= 15) {
        if (*ptr == ' ') {
            if (field == 14) {
                char buf[32];
                int len = ptr - start;
                if (len > 31) len = 31;
                strncpy(buf, start, len);
                buf[len] = '\0';
                *utime = strtoul(buf, NULL, 10);
            } else if (field == 15) {
                char buf[32];
                int len = ptr - start;
                if (len > 31) len = 31;
                strncpy(buf, start, len);
                buf[len] = '\0';
                *stime = strtoul(buf, NULL, 10);
                break;
            }
            field++;
            start = ptr + 1;
        }
        ptr++;
    }

    return 0;
}

/**
* @brief Lit la mémoire utilisée par un processus
*
* Lit le fichier /proc/[pid]/status pour obtenir la mémoire RSS
* (Resident Set Size) d'un processus en kilo-octets.
*
* @param pid Le PID du processus
* @return La mémoire utilisée en kilo-octets, ou 0 en cas d'erreur
*/
int read_process_memory_kb(int pid) {
    char path[256];
    FILE *f;
    char line[256];
    int memory_kb = 0;

    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    f = fopen(path, "r");
    if (!f) return 0;

    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "VmRSS:")) {
            // Format: "VmRSS:     kB"
            char *num_start = strchr(line, ':');
            if (num_start) {
                num_start++; // Passer le ':'
                // Sauter les espaces
                while (*num_start == ' ' || *num_start == '\t') num_start++;
                // Lire le nombre
                sscanf(num_start, "%d", &memory_kb);
            }
            break;
        }
    }

    fclose(f);
    return memory_kb;
}

/**
* @brief Lit le nom d'un processus
*
* Tente d'abord de lire le nom depuis /proc/[pid]/comm, et si cela échoue,
* lit depuis /proc/[pid]/stat.
*
* @param pid Le PID du processus
* @param name Buffer pour stocker le nom du processus
*/
void read_process_name(int pid, char *name) {
    char path[256];
    FILE *f;

    snprintf(path, sizeof(path), "/proc/%d/comm", pid);
    f = fopen(path, "r");
    if (f) {
        if (fgets(name, 256, f)) {
            char *nl = strchr(name, '\n');
            if (nl) *nl = '\0';
        }
        fclose(f);
        return;
    }

    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    f = fopen(path, "r");
    if (!f) {
        strcpy(name, "?");
        return;
    }

    int tmp_pid;
    fscanf(f, "%d (%255[^)])", &tmp_pid, name);
    fclose(f);
}

/**
* @brief Lit le temps d'exécution d'un processus
*
* Calcule le temps d'exécution d'un processus en secondes en utilisant
* son starttime et l'uptime du système.
*
* @param pid Le PID du processus
* @return Le temps d'exécution en secondes, ou 0.0 en cas d'erreur
*/
float read_process_uptime(int pid) {
    char path[256];
    FILE *f;
    unsigned long starttime = 0;
    double system_uptime;

    // Lire starttime
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    f = fopen(path, "r");
    if (!f) return 0.0f;

    char line[1024];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return 0.0f;
    }
    fclose(f);

    // Parse champ 22
    int field = 1;
    char *ptr = line;
    char *start = line;

    while (*ptr && field <= 22) {
        if (*ptr == ' ') {
            if (field == 22) {
                char buf[32];
                int len = ptr - start;
                if (len > 31) len = 31;
                strncpy(buf, start, len);
                buf[len] = '\0';
                starttime = strtoul(buf, NULL, 10);
                break;
            }
            field++;
            start = ptr + 1;
        }
        ptr++;
    }

    if (starttime == 0) return 0.0f;

    // Lire uptime système
    f = fopen("/proc/uptime", "r");
    if (!f) return 0.0f;

    fscanf(f, "%lf", &system_uptime);
    fclose(f);

    // Convertir
    long ticks_per_sec = sysconf(_SC_CLK_TCK);
    float uptime = system_uptime - (starttime / (double)ticks_per_sec);

    return (uptime > 0) ? uptime : 0.0f;
}

/**
* @brief Lit l'état d'un processus
*
* Lit le fichier /proc/[pid]/stat pour obtenir l'état actuel du processus
* (R: running, S: sleeping, D: disk sleep, Z: zombie, etc.).
*
* @param pid Le PID du processus
* @return Le caractère représentant l'état du processus, ou '?' en cas d'erreur
*/
char read_process_state(int pid) {
    char path[256];
    FILE *f;
    char state = '?';

    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    f = fopen(path, "r");
    if (!f) return state;

    int tmp_pid;
    char name[256];
    fscanf(f, "%d (%[^)]) %c", &tmp_pid, name, &state);
    fclose(f);

    return state;
}

/**
* @brief Récupère la liste complète des processus
*
* Parcourt le répertoire /proc, lit les informations de chaque processus
* et calcule les statistiques CPU et mémoire. Les échantillons CPU sont
* conservés entre les appels pour calculer les pourcentages CPU.
*
* @param list Pointeur vers un tableau qui contiendra la liste des processus
* @param count Pointeur vers un entier qui contiendra le nombre de processus
* @return 0 en cas de succès, -1 en cas d'erreur
*/
int get_process_list(process_info_t **list, int *count) {
    DIR *proc_directory = opendir("/proc");
    if (!proc_directory) {
        return -1;
    }

    struct dirent *sub_directory;
    int capacity = 256;
    int index = 0;

    *list = malloc(sizeof(process_info_t) * capacity);
    if (!*list) {
        closedir(proc_directory);
        return -1;
    }

    // Obtenir mémoire totale système
    total_system_memory_kb = get_total_system_memory();

    // Temps actuel pour calcul CPU
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    unsigned long long current_total_cpu = read_total_cpu_time();

    double time_diff = 0.0;
    if (previous_sample_time.tv_sec > 0) {
        time_diff = (current_time.tv_sec - previous_sample_time.tv_sec) +
                   (current_time.tv_nsec - previous_sample_time.tv_nsec) / 1e9;
    }

    // Parcourir /proc
    while ((sub_directory = readdir(proc_directory)) != NULL) {
        if (isdigit(sub_directory->d_name[0])) {
            int pid = atoi(sub_directory->d_name);

            if (index >= capacity) {
                capacity *= 2;
                process_info_t *tmp = realloc(*list, sizeof(process_info_t) * capacity);
                if (!tmp) {
                    free(*list);
                    closedir(proc_directory);
                    return -1;
                }
                *list = tmp;
            }

            process_info_t *proc = &(*list)[index];
            memset(proc, 0, sizeof(process_info_t));

            // Données de base (TOUJOURS relues)
            proc->pid = pid;
            proc->state = read_process_state(pid);
            read_process_name(pid, proc->name);

            // Mémoire - TOUJOURS relue pour mise à jour dynamique
            proc->memory_kb = read_process_memory_kb(pid);

            // Uptime - TOUJOURS relu
            proc->time = read_process_uptime(pid);

            // PPid
            char path[256];
            FILE *f;
            snprintf(path, sizeof(path), "/proc/%d/status", pid);
            f = fopen(path, "r");
            if (f) {
                char line[256];
                while (fgets(line, sizeof(line), f)) {
                    if (strstr(line, "PPid:")) {
                        sscanf(line, "PPid: %d", &proc->ppid);
                        break;
                    }
                }
                fclose(f);
            }

            // Kernel thread (peut être mis en cache)
            int pid_cache = 0;
            int is_kernel_cache = 0;
            if (pid != pid_cache) {
                snprintf(path, sizeof(path), "/proc/%d/exe", pid);
                proc->is_kernel = (access(path, F_OK) == -1) ? 1 : 0;
                pid_cache = pid;
                is_kernel_cache = proc->is_kernel;
            } else {
                proc->is_kernel = is_kernel_cache;
            }

            // Calcul CPU % (nécessite échantillonnage)
            unsigned long utime, stime;
            if (read_process_cpu_ticks(pid, &utime, &stime) == 0) {
                unsigned long long current_process_cpu = utime + stime;
                cpu_sample_t *prev = find_cpu_sample(pid);

                if (prev && time_diff > 0.1 && previous_total_cpu > 0) {
                    unsigned long long process_diff = current_process_cpu - prev->last_cpu_time;
                    unsigned long long total_diff = current_total_cpu - previous_total_cpu;

                    if (total_diff > 0) {
                        long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
                        if (num_cores < 1) num_cores = 1;

                        proc->cpu_percent = ((double)process_diff / total_diff) * 100.0 * num_cores;

                        // Limiter
                        if (proc->cpu_percent > 100.0) proc->cpu_percent = 100.0;
                        if (proc->cpu_percent < 0.0) proc->cpu_percent = 0.0;
                    }
                } else {
                    // Pas encore d'échantillon précédent
                    proc->cpu_percent = 0.0;
                }

                update_cpu_sample(pid, current_process_cpu);
            } else {
                proc->cpu_percent = 0.0;
            }

            index++;
        }
    }

    closedir(proc_directory);

    // Mettre à jour pour prochain appel
    previous_sample_time = current_time;
    previous_total_cpu = current_total_cpu;

    *count = index;
    return 0;
}

/* Tue un processus */
int kill_process(int pid) {
	return kill(pid, SIGKILL);
}

/* Met un processus en pause */
int pause_process(int pid) {
	return kill(pid, SIGSTOP);
}

/* Reprend un processus */
int resume_process(int pid) {
	return kill(pid, SIGCONT);
}

/* Redémarre un processus (SIGTERM + SIGCONT) */
int restart_process(int pid) {
	if (kill(pid, SIGTERM) == -1) {
		return -1;
	}
	return kill(pid, SIGCONT);
}

/* Récupère UN processus précis */
int get_process(int pid, process_info_t *proc) {
	char status_file_loc[512];
	snprintf(status_file_loc, sizeof(status_file_loc),
	         "/proc/%d/status", pid);

	FILE *status_file = fopen(status_file_loc, "r");
	if (!status_file) {
		return -1;
	}

	memset(proc, 0, sizeof(process_info_t));

	char line[512];

	while (fgets(line, sizeof(line), status_file) != NULL) {
		sscanf(line, "State: %c", &proc->state);
		sscanf(line, "Pid: %d", &proc->pid);
		sscanf(line, "PPid: %d", &proc->ppid);
		sscanf(line, "Kthread: %d", &proc->is_kernel);
		sscanf(line, "VmRSS: %d", &proc->memory_kb);
	}

	fclose(status_file);
	return 0;
}
