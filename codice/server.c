/// @file server.c
/// @brief Contiene l'implementazione del SERVER.

#include "defines.h"
#include "err_exit.h"
#include "fifo.h"
#include "semaphore.h"
#include "shared_memory.h"

int create_sem_set(key_t semkey) {
    // Create a semaphore set with 2 semaphores
    int semid = semget(IPC_PRIVATE, 2, S_IRUSR | S_IWUSR);
    if (semid == -1) errExit("semget failed");

    // Initialize the semaphore set with semctl
    union semun arg;
    unsigned short values[] = {0, 0};
    arg.array = values;

    if (semctl(semid, 0, SETALL, arg) == -1) errExit("semctl SETALL failed");
    return semid;
}

int main(int argc, char* argv[]) {
    // check command line input arguments
    if (argc != 3) {
        printf("Usage: %s msg_queue_key file_posizioni\n", argv[0]);
        exit(1);
    }

    key_t msgKey = atoi(argv[1]);
    if (msgKey <= 0) {
        printf("The msg_queue_key must be greater than zero!\n");
        exit(1);
    }

    int fd = open(argv[2], O_RDONLY);
    if (fd == -1) printf("File %s does not exist\n", argv[2]);

    for (int child = 0; child < 5; ++child) {
        pid_t pid = fork();
        // check error for fork
        if (pid == -1) printf("child %d not created!", child);
        // check if running process is child or parent
        else if (pid == 0) {
            // code executed only by the child
        }
        exit(0);
    }
    return 0;
}
