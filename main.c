#include <stdio.h>
#include <dirent.h>
#include <ctype.h>


int main(int argc, char* argv[]) {

    // Ouverture du dossier proc
    DIR* proc_directory = opendir("/proc");
    if(!proc_directory) {
        printf("Error with folder /pro\n");
        return 1;
    }
    struct dirent* sub_directory;


    // On parcours tout le dossier, à la recherche des dossiers des processus (ex : /proc/2548)
    while( (sub_directory = readdir(proc_directory)) != NULL) {
        if(isdigit(sub_directory->d_name[0])) {

            // Ouverture du fichier proc/[..PID..]/status
            char status_file_loc[521];
            snprintf(status_file_loc, sizeof(status_file_loc), "/proc/%s/status", sub_directory->d_name);
            FILE* status_file = fopen(status_file_loc, "r");
            if(!status_file) printf("Error with fopen() of status_file");


            // Parcours du fichier status, et récupération des données intéressantes grâce à sscanf()
            char line[512];

            char state;
            int pid;
            int ppid;
            int kern_proc;
            int mem_size;

            while(fgets(line, sizeof(line), status_file) != NULL) {
                sscanf(line, "State: %c", &state);
                sscanf(line, "Pid: %d", &pid);
                sscanf(line, "PPid; %d", &ppid);
                sscanf(line, "Kthread: %d", &kern_proc);
                sscanf(line, "VmRSS: %d", &mem_size);
            }
            printf("Etat : %c, PID : %d, PPID : %d, Process Kernel : %d, Memory size : %d\n", state, pid, ppid, kern_proc, mem_size);
        }
    }
}
