/// @file server.c
/// @brief Contiene l'implementazione del SERVER.

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "defines.h"
#include "err_exit.h"
#include "fifo.h"
#include "semaphore.h"
#include "shared_memory.h"
#define BOARD_SIZE 10
#define MAX_ACK 100
#define LINE_SIZE 20
#define CHILD_NUM 5
#define POSX(c) c * 4
#define POSY(c) c * 4 + 2

int child;
int pid_child[CHILD_NUM] = {};
int pos_fd;
int semBoardId;
int semProcId;
int *board;
Acknowledgment *ackList;
int shmBoardId;
int shmAckListId;
int shmPosId;
char *posLine;

void serverSigHandler(int sig);
void childSigHandler(int sig);
void set_table_val_sem(int *board, int x, int y, int val, int sem_id);
void init();
void quit();

int main(int argc, char *argv[]) {
    //-----------------------------------------------------------------------------
    // Check command line input arguments
    //-----------------------------------------------------------------------------

    if (argc != 3) {
        printf("Usage: %s msg_queue_key file_posizioni\n", argv[0]);
        exit(1);
    }

    key_t msgKey = atoi(argv[1]);
    if (msgKey <= 0) {
        printf("The msg_queue_key must be greater than zero!\n");
        exit(1);
    }

    //-----------------------------------------------------------------------------
    // Open position file
    //-----------------------------------------------------------------------------

    pos_fd = open(argv[2], O_RDONLY);
    if (pos_fd == -1) printf("File %s does not exist\n", argv[2]);

    init();

    //-----------------------------------------------------------------------------
    // Set signal handler
    // TODO remove other signals
    //-----------------------------------------------------------------------------

    if (signal(SIGTERM, serverSigHandler) == SIG_ERR)
        errExit("change signal handler failed");

    //-----------------------------------------------------------------------------
    // Create and starting devices D1,...,D5 (5 children)
    //-----------------------------------------------------------------------------

    for (child = 0; child < CHILD_NUM; ++child) {
        pid_child[child] = fork();

        if (pid_child[child] == -1)
            printf("child %d not created!", child);

        else if (pid_child[child] == 0) {
            if (signal(SIGCONT, childSigHandler) == SIG_ERR ||
                signal(SIGTERM, childSigHandler) == SIG_ERR)
                errExit("change signal handler failed");

            int x = -1, y = -1;

            while (1) {
                if (child == 0) pause();

                semOp(semProcId, child, -1);

                // overwrite old position
                if (x != -1 && y != -1)
                    set_table_val_sem(board, x, y, 0, semBoardId);

                x = posLine[POSX(child)] - '0';
                y = posLine[POSY(child)] - '0';

                set_table_val_sem(board, x, y, getpid(), semBoardId);

                child == 4 ? semOp(semProcId, 0, 1)
                           : semOp(semProcId, child + 1, 1);
            }
            exit(0);
        }
    }

    sleep(2);

    while (1) {
        int numRead = read(pos_fd, posLine, LINE_SIZE);
        if (numRead == -1) errExit("error reading position file");
        if (numRead == 0) printf("<server> File posizioni terminato \n");
        if (numRead > 0) {
            kill(pid_child[0], SIGCONT);
            printf("Posizioni: %s", posLine);
            sleep(1);
            print_board(board, BOARD_SIZE, BOARD_SIZE);
        }
        sleep(5);
    }
    return 0;
}

void init() {
    //-----------------------------------------------------------------------------
    // Allocate a shared memory segment for board
    //-----------------------------------------------------------------------------

    printf("<Server> allocating a shared memory segment for board\n");
    shmBoardId =
        alloc_shared_memory(IPC_PRIVATE, sizeof(int) * BOARD_SIZE * BOARD_SIZE);

    //-----------------------------------------------------------------------------
    // Attach the shared memory segment for board
    //-----------------------------------------------------------------------------

    printf("<Server> attaching the shared memory segment for board\n");
    board = (int *)get_shared_memory(shmBoardId, 0);

    //-----------------------------------------------------------------------------
    // Allocate a shared memory segment for scharing positin (sever -> childern)
    //-----------------------------------------------------------------------------

    printf("<Server> allocating a shared memory segment for board\n");
    shmPosId = alloc_shared_memory(IPC_PRIVATE, sizeof(char) * LINE_SIZE);

    //-----------------------------------------------------------------------------
    // Attach shared memory segment for scharing positin (sever -> childern)
    //-----------------------------------------------------------------------------

    printf("<Server> attaching the shared memory segment for board\n");
    posLine = (char *)get_shared_memory(shmPosId, 0);

    //-----------------------------------------------------------------------------
    // Allocate a shared memory segment for acknowledgment list
    //-----------------------------------------------------------------------------

    printf(
        "<Server> allocating a shared memory segment for acknowledgment "
        "list\n");
    shmAckListId =
        alloc_shared_memory(IPC_PRIVATE, sizeof(Acknowledgment) * MAX_ACK);

    //-----------------------------------------------------------------------------
    // Attach the shared memory segment for acknowledgment list
    //-----------------------------------------------------------------------------

    printf(
        "<Server> attaching the shared memory segment acknowledgment list\n");
    Acknowledgment *ackList =
        (Acknowledgment *)get_shared_memory(shmAckListId, 0);

    //-----------------------------------------------------------------------------
    // Create a semaphore for board access
    //-----------------------------------------------------------------------------

    printf("<Server> creating a semaphore set for board access.\n");
    semBoardId = create_sem_set(1);

    //-----------------------------------------------------------------------------
    // Set semaphore at 1 to allow board access
    //-----------------------------------------------------------------------------

    printf("<Server> Setting board semaphore to 1.\n");
    semOp(semBoardId, 0, 1);

    //-----------------------------------------------------------------------------
    // Create a semaphore set of 5 for process syncronization
    //-----------------------------------------------------------------------------

    printf("<Server> creating a semaphore set for process syncronization\n");
    semProcId = create_sem_set(5);

    //-----------------------------------------------------------------------------
    // Set semaphore at 1 to allow first process to start
    //-----------------------------------------------------------------------------

    printf("<Server> Setting first process semaphore to 1.\n");
    semOp(semProcId, 0, 1);
}
void quit() {
    printf("<Server> Terminating all child processes.\n");
    kill(pid_child[0], SIGTERM);
    kill(pid_child[1], SIGTERM);
    kill(pid_child[2], SIGTERM);
    kill(pid_child[3], SIGTERM);
    kill(pid_child[4], SIGTERM);

    printf("<Server> Closing list position file.\n");
    if (close(pos_fd) == -1) printf("error closing file descriptor");

    printf("<Server> Deleting semaphores.\n");
    delete_sem_set(semBoardId);
    delete_sem_set(semProcId);

    printf("<Server> Dataching memory.\n");
    free_shared_memory(board);
    free_shared_memory(ackList);

    printf("<Server> Removing memory.\n");
    remove_shared_memory(shmBoardId);
    remove_shared_memory(shmAckListId);

    exit(0);
}
void set_table_val_sem(int *board, int x, int y, int val, int sem_id) {
    // lock semaphore
    semOp(sem_id, 0, -1);
    // set value on the table
    set_table_val(board, x, y, val);
    // unlock semaphore
    semOp(sem_id, 0, 1);
}

void serverSigHandler(int sig) {
    if (sig == SIGTERM) {
        quit();
    }
}
void childSigHandler(int sig) {
    if (sig == SIGCONT)
        ;
    else if (sig == SIGTERM) {
        exit(0);
    }
}
