#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <signal.h>   // kill, SIGKILL, SIGSTOP, SIGCONT
#include <sys/types.h>
#include <unistd.h>   // kill


#include "process.h"

/* Récupère la liste des processus */
int get_process_list(process_info_t **list, int *count) {
	DIR *proc_directory = opendir("/proc");
	if (!proc_directory) {
		perror("opendir");
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

	while ((sub_directory = readdir(proc_directory)) != NULL) {
		if (isdigit(sub_directory->d_name[0])) {
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

			char status_file_loc[512];
			snprintf(status_file_loc, sizeof(status_file_loc),
			         "/proc/%s/status", sub_directory->d_name);

			FILE *status_file = fopen(status_file_loc, "r");
			if (!status_file) {
				continue;
			}

			process_info_t proc;
			memset(&proc, 0, sizeof(process_info_t));

			char line[512];

			while (fgets(line, sizeof(line), status_file) != NULL) {
				sscanf(line, "State: %c", &proc.state);
				sscanf(line, "Pid: %d", &proc.pid);
				sscanf(line, "PPid: %d", &proc.ppid);
				sscanf(line, "Kthread: %d", &proc.is_kernel);
				sscanf(line, "VmRSS: %d", &proc.memory);
			}

			fclose(status_file);

			(*list)[index++] = proc;
		}
	}

	closedir(proc_directory);
	*count = index;
	return 0;
}

/* Tue un processus */
int kill_process(pid_t pid) {
	return kill(pid, SIGKILL);
}

/* Met un processus en pause */
int pause_process(pid_t pid) {
	return kill(pid, SIGSTOP);
}

/* Reprend un processus */
int resume_process(pid_t pid) {
	return kill(pid, SIGCONT);
}

/* Redémarre un processus (SIGTERM + SIGCONT) */
int restart_process(pid_t pid) {
	if (kill(pid, SIGTERM) == -1) {
		return -1;
	}
	return kill(pid, SIGCONT);
}

/* Récupère UN processus précis */
int get_process(pid_t pid, process_info_t *proc) {
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
		sscanf(line, "VmRSS: %d", &proc->memory);
	}

	fclose(status_file);
	return 0;
}
