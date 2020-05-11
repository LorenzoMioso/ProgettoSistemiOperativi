/// @file semaphore.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione dei SEMAFORI.

#include "err_exit.h"
#include "semaphore.h"

#include <sys/sem.h>



void semOp (int semid, unsigned short sem_num, short sem_op) {
    struct sembuf sop = {.sem_num = sem_num, .sem_op = sem_op, .sem_flg = 0};

    if (semop(semid, &sop, 1) == -1)
        errExit("semop failed");
}

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
