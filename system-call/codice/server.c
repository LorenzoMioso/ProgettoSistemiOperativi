/// @file server.c
/// @brief Contiene l'implementazione del SERVER.

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "defines.h"
#include "err_exit.h"
#include "fifo.h"
#include "semaphore.h"
#include "shared_memory.h"

#define BOARD_SIZE 10
#define LINE_SIZE 20
#define CHILD_NUM 5
#define SIZE_FIFO_PATH 40
#define POSX(c) c * 4
#define POSY(c) c * 4 + 2
#define USED_ID_PATH "/tmp/used.id"
#define SEM_ID_FILE_KEY 1234

int shmBoardId;
int semBoardId;
int *board;
int shmAckListId;
int semAckListId;
AckList *ackList;
int pos_fd;
int shmPosId;
int semPosId;
char *posLine;
int ackManager;
int msqid = -1;
sigset_t prevSet;
int pid_child[CHILD_NUM];
int child;
int semProcId;
char child_fifo_path[SIZE_FIFO_PATH];
int child_fifo_fd;
int move = 0;
int step;
int semFileId;
int usedIdFileDs;

void serverSigHandler(int sig);
void childSigHandler(int sig);
void ackManagerSigHandler(int sig);
void init();
void quit();
void get_fifo_path_child(char *path, pid_t pid);
int get_fifo_child(char *fifo_path);

int main(int argc, char *argv[]) {
    // Check command line input arguments
    if (argc != 3) {
        printf("Usage: %s msg_queue_key file_posizioni\n", argv[0]);
        exit(1);
    }
    key_t msgKey = atoi(argv[1]);
    if (msgKey <= 0) {
        printf("The msg_queue_key must be greater than zero!\n");
        exit(1);
    }
    // Open position file
    pos_fd = open(argv[2], O_RDONLY);
    if (pos_fd == -1) printf("File %s does not exist\n", argv[2]);

    init();
    // Remove otger signals
    sigset_t mySet;
    sigfillset(&mySet);
    // sigdelset(&mySet, SIGTERM);
    sigdelset(&mySet, SIGINT);
    sigprocmask(SIG_SETMASK, &mySet, &prevSet);
    // Set signal handler
    // if (signal(SIGTERM, serverSigHandler) == SIG_ERR)
    if (signal(SIGINT, serverSigHandler) == SIG_ERR)
        ErrExit("change signal handler failed");
    // Start ack manager
    ackManager = fork();
    if (ackManager == -1) {
        printf("Ack Manager not created!\n");
    } else if (ackManager == 0) {
        // Set normal signal mask
        sigprocmask(SIG_SETMASK, &prevSet, NULL);
        // set signal handler
        if (signal(SIGTERM, ackManagerSigHandler) == SIG_ERR)
            ErrExit("change signal handler failed");
        // get the message queue, or create a new one if it does not exist
        msqid = msgget(msgKey, IPC_CREAT | S_IRUSR | S_IWUSR);
        if (msqid == -1) ErrExit("msgget failed");
        int achedId;
        AckQueue q;
        size_t gSize = sizeof(AckQueue) - sizeof(long);
        // managing acks
        while (1) {
            sleep(5);
            printf("AckArray:\n");
            semOp(semAckListId, 0, -1);
            print_ackArray(ackList->arr, MAX_ACK);
            // trovo un messaggio confermato da tutti i devices
            achedId = idAckedByAll(ackList->arr, MAX_ACK);
            if (achedId != 0) {
                // preparo il message da inviare nella queue
                // mtype prende l'id confermato
                q.mtype = achedId;
                // inserisco gli ack nella messaggio
                setAckQueue(q.arr, DEV_NUM, ackList->arr, MAX_ACK, achedId);
                del_ackArrayById(achedId, ackList->arr, MAX_ACK);
                if (msgsnd(msqid, &q, gSize, 0) == -1)
                    printf("msgsnd failed\n");
            }
            semOp(semAckListId, 0, 1);
        }
    } else {
        printf("Server on going!\n");
        // Create and starting devices D1,...,D5 (5 children)
        for (child = 0; child < CHILD_NUM; ++child) {
            pid_child[child] = fork();
            if (pid_child[child] == -1)
                printf("child %d not created!", child);
            else if (pid_child[child] == 0) {
                // Set normal signal mask
                sigprocmask(SIG_SETMASK, &prevSet, NULL);
                // set signal handler
                if (signal(SIGUSR1, childSigHandler) == SIG_ERR ||
                    signal(SIGTERM, childSigHandler) == SIG_ERR ||
                    signal(SIGCONT, childSigHandler) == SIG_ERR)
                    ErrExit("change signal handler failed");
                // create device fifo
                get_fifo_path_child(child_fifo_path, getpid());
                child_fifo_fd = get_fifo_child(child_fifo_path);
                int x = -1, y = -1;
                while (1) {
                    // initialize variables
                    Message msg_buff = {0};
                    Acknowledgment ack_buff = {0};
                    int is_near[CHILD_NUM] = {0};
                    int numRead = 0;
                    int dist = 0;

                    // receive message from pipe
                    numRead =
                        read_fifo(child_fifo_fd, &msg_buff, sizeof(Message));

                    if (numRead == sizeof(Message)) {
                        // printf("<child%d> Messaggio ricevuto\n", child);
                        dist = msg_buff.max_distance;
                        // crea l ack
                        new_Acknowledgment(&ack_buff, msg_buff.pid_sender,
                                           msg_buff.pid_receiver,
                                           msg_buff.message_id, time(NULL));
                        semOp(semAckListId, 0, -1);
                        if (add_ackArray(&ack_buff, ackList->arr, MAX_ACK) ==
                            -1)
                            numRead = 0;
                        semOp(semAckListId, 0, 1);
                    }

                    // find nerby devices
                    set_zero(is_near, CHILD_NUM);
                    if (x != -1 && y != -1) {
                        semOp(semBoardId, 0, -1);
                        nearby_pids(board, BOARD_SIZE, BOARD_SIZE, x, y, dist,
                                    is_near, CHILD_NUM);
                        semOp(semBoardId, 0, 1);
                    }
                    // send message to nearby_pids
                    if (numRead != 0) {
                        for (int i = 0; i < CHILD_NUM; i++) {
                            if (is_near[i] != 0 && is_near[i] != getpid()) {
                                // change sender and receiver
                                msg_buff.pid_sender = getpid();
                                msg_buff.pid_receiver = is_near[i];
                                // check if device has already received the
                                // mesage by checking on ack list
                                semOp(semAckListId, 0, -1);
                                int isAckedArray = is_ackedArray(
                                    &msg_buff, ackList->arr, MAX_ACK);
                                semOp(semAckListId, 0, 1);
                                if (isAckedArray == 0) {
                                    char nearby_fifo_path[SIZE_FIFO_PATH];
                                    get_fifo_path_child(nearby_fifo_path,
                                                        is_near[i]);
                                    int nearby_fifo_fd =
                                        open_fifo(nearby_fifo_path,
                                                  O_WRONLY | O_NONBLOCK);
                                    write_fifo(nearby_fifo_fd, &msg_buff,
                                               sizeof(Message));
                                }
                            }
                        }
                    }
                    if (child == 0) {
                        pause();
                    }
                    // possibly move
                    // if (move == 1) {
                    semOp(semProcId, child, -1);
                    if (x != -1 && y != -1) {
                        semOp(semBoardId, 0, -1);
                        set_table_val(board, x, y, 0);
                        semOp(semBoardId, 0, 1);
                    }
                    semOp(semPosId, 0, -1);
                    x = posLine[POSX(child)] - '0';
                    y = posLine[POSY(child)] - '0';
                    semOp(semPosId, 0, 1);
                    semOp(semBoardId, 0, -1);
                    set_table_val(board, x, y, getpid());
                    semOp(semBoardId, 0, 1);
                    move = 0;
                    child == 4 ? semOp(semProcId, 0, 1)
                               : semOp(semProcId, child + 1, 1);
                    //}
                }
                exit(0);
            }
        }
        sleep(1);
        while (1) {
            // sleep(1);
            step++;
            int numRead = read(pos_fd, posLine, LINE_SIZE);
            if (numRead == -1) ErrExit("error reading position file");
            if (numRead == 0) printf("<server> File posizioni terminato\n");
            if (numRead > 0) {
                //  kill(pid_child[0], SIGUSR1);
                //  kill(pid_child[1], SIGUSR1);
                //  kill(pid_child[2], SIGUSR1);
                //  kill(pid_child[3], SIGUSR1);
                //  kill(pid_child[4], SIGUSR1);
                kill(pid_child[0], SIGCONT);
            }
            // sleep(1);
            sleep(2);
            semOp(semBoardId, 0, -1);
            print_board_status(board, BOARD_SIZE, BOARD_SIZE, &pid_child[0],
                               DEV_NUM, step);
            semOp(semBoardId, 0, 1);
        }
    }
    return 0;
}
void init() {
    // Allocate a shared memory segment for board
    printf("<Server> allocating a shared memory segment for board\n");
    shmBoardId =
        alloc_shared_memory(IPC_PRIVATE, sizeof(int) * BOARD_SIZE * BOARD_SIZE);
    // Attach the shared memory segment for board
    printf("<Server> attaching the shared memory segment for board\n");
    board = (int *)get_shared_memory(shmBoardId, 0);
    // Allocate a shared memory segment for scharing positin (sever ->
    // childern)
    printf("<Server> allocating a shared memory segment for board\n");
    shmPosId = alloc_shared_memory(IPC_PRIVATE, sizeof(char) * LINE_SIZE);
    // Attach shared memory segment for scharing positin (sever -> childern)
    printf("<Server> attaching the shared memory segment for board\n");
    posLine = (char *)get_shared_memory(shmPosId, 0);
    // Allocate a shared memory segment for acknowledgment list
    printf(
        "<Server> allocating a shared memory segment for acknowledgment "
        "list\n");
    shmAckListId = alloc_shared_memory(IPC_PRIVATE, sizeof(AckList));
    // Attach the shared memory segment for acknowledgment list
    printf(
        "<Server> attaching the shared memory segment acknowledgment "
        "list\n");
    ackList = (AckList *)get_shared_memory(shmAckListId, 0);
    // Create a semaphore for board access
    printf("<Server> creating a semaphore set for board access.\n");
    semBoardId = create_sem_set(1);
    // Set semaphore at 1 to allow board access
    printf("<Server> Setting board semaphore to 1.\n");
    semOp(semBoardId, 0, 1);
    // Create a semaphore for ack list access
    printf("<Server> creating a semaphore set for ack list access.\n");
    semAckListId = create_sem_set(1);
    // Set semaphore at 1 to allow ack list access
    printf("<Server> Setting ack list semaphore to 1.\n");
    semOp(semAckListId, 0, 1);
    // Create a semaphore set of 5 for process syncronization
    printf("<Server> creating a semaphore set for process syncronization\n");
    semProcId = create_sem_set(5);
    // Set semaphore at 1 to allow first process to start
    printf("<Server> Setting first process semaphore to 1.\n");
    semOp(semProcId, 0, 1);

    semPosId = create_sem_set(1);
    semOp(semPosId, 0, 1);

    // Create file for message ids
    printf("<Server> Creating file for message id check\n");
    usedIdFileDs = open(USED_ID_PATH, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (usedIdFileDs == -1) printf("Error creating file for used id\n");

    // Create semaphore for file message ids
    semFileId = create_sem_set_by_key(SEM_ID_FILE_KEY, 1);
    semOp(semFileId, 0, 1);
}
void quit() {
    printf("<Server> Terminating AckManager.\n");
    kill(ackManager, SIGTERM);
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
    delete_sem_set(semPosId);
    delete_sem_set(semFileId);
    printf("<Server> Dataching memory.\n");
    free_shared_memory(board);
    free_shared_memory(ackList);
    printf("<Server> Removing memory.\n");
    remove_shared_memory(shmBoardId);
    remove_shared_memory(shmAckListId);
    printf("<Server> Removing file for message id.\n");
    close(usedIdFileDs);
    unlink(USED_ID_PATH);
    exit(0);
}
void serverSigHandler(int sig) { quit(); }
void childSigHandler(int sig) {
    if (sig == SIGTERM) {
        printf("<Child> SIGTERM ricevuto\n");
        close_fifo(child_fifo_fd);
        unlink_fifo(child_fifo_path);
        exit(0);
    }
}
void ackManagerSigHandler(int sig) {
    if (sig == SIGTERM) {
        printf("<Ack> SIGTERM ricevuto\n");
        printf("<Ack> Deleting message queue\n");
        if (msgctl(msqid, IPC_RMID, NULL) == -1)
            ErrExit("Message queue could not be deleted.\n");
        exit(0);
    }
}
void get_fifo_path_child(char *path, pid_t pid) {
    sprintf(path, "/tmp/dev_fifo.%d", pid);
}
int get_fifo_child(char *fifo_path) {
    mk_fifo(fifo_path, IPC_CREAT | S_IRUSR | S_IWUSR);
    return open_fifo(fifo_path, O_RDONLY | O_NONBLOCK);
}
