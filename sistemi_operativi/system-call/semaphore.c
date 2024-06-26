/// @file semaphore.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione dei SEMAFORI.

#include "semaphore.h"

#include <stdio.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "defines.h"
#include "err_exit.h"

void semOp(int semid, unsigned short sem_num, short sem_op) {
    struct sembuf sop = {.sem_num = sem_num, .sem_op = sem_op, .sem_flg = 0};

    if (semop(semid, &sop, 1) == -1) ErrExit("semop failed");
    // if (semop(semid, &sop, 1) == -1) printf("semop failed\n");
}

int create_sem_set(unsigned short sem_num) {
    // Create a new semaphore set with sem_num semaphores
    int semid = semget(IPC_PRIVATE, sem_num, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (semid == -1) ErrExit("semget failed");

    // Initialize the semaphore set with semctl
    union semun arg;
    unsigned short values[sem_num];
    for (int i = 0; i < sem_num; i++) values[i] = 0;
    arg.array = values;

    if (semctl(semid, 0, SETALL, arg) == -1) ErrExit("semctl SETALL failed");
    return semid;
}
int create_sem_set_by_key(key_t key, unsigned short sem_num) {
    // Create a new semaphore set with sem_num semaphores
    int semid = semget(key, sem_num, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (semid == -1) ErrExit("semget failed");

    // Initialize the semaphore set with semctl
    union semun arg;
    unsigned short values[sem_num];
    for (int i = 0; i < sem_num; i++) values[i] = 0;
    arg.array = values;

    if (semctl(semid, 0, SETALL, arg) == -1) ErrExit("semctl SETALL failed");
    return semid;
}
int get_sem_set_by_key(key_t key) {
    // Create a new semaphore set with sem_num semaphores
    int semid = semget(key, 0, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (semid == -1) ErrExit("semget failed");
    return semid;
}
void delete_sem_set(int semid) {
    if (semctl(semid, 0 /*ignored*/, IPC_RMID, NULL) == -1)
        ErrExit("semctl IPC_RMID failed");
}

void printSemaphoresValue(int semid, int semnum) {
    unsigned short semVal[semnum];
    union semun arg;
    arg.array = semVal;

    // get the current state of the set
    if (semctl(semid, 0 /*ignored*/, GETALL, arg) == -1)
        ErrExit("semctl GETALL failed");

    // print the semaphore's value
    printf("semaphore set state: ");
    for (int i = 0; i < semnum; i++) printf("%d ", semVal[i]);
    printf("\n");
}
