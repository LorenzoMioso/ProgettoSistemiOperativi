/// @file semaphore.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione dei SEMAFORI.

#pragma once
#ifndef _SEMAPHORE_HH
#define _SEMAPHORE_HH

#include <sys/sem.h>

// definition of the union semun
union semun {
    int val;
    struct semid_ds* buf;
    unsigned short* array;
};

/* errsemOpExit is a support function to manipulate a semaphore's value
 * of a semaphore set. semid is a semaphore set identifier, sem_num is the
 * index of a semaphore in the set, sem_op is the operation performed on sem_num
 */

void semOp(int semid, unsigned short sem_num, short sem_op);
int create_sem_set(unsigned short sem_num);
int create_sem_set_by_key(key_t key, unsigned short sem_num);
int get_sem_set_by_key(key_t key);
void printSemaphoresValue(int semid, int semnum);
void delete_sem_set(int semid);

#endif
