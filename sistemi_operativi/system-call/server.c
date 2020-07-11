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
#define MAX_ID 100

int shmConfirmedList;
int semConfirmedList;
ConfirmedList *confirmedList;
int shmIdMatrix;
int semIdMatrix;
IdMatrix *idMatrix;
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
    // Remove other signals
    sigset_t mySet;
    sigfillset(&mySet);
    sigdelset(&mySet, SIGTERM);
    sigprocmask(SIG_SETMASK, &mySet, &prevSet);
    // Set signal handler
    if (signal(SIGTERM, serverSigHandler) == SIG_ERR)
        ErrExit("change signal handler failed");
    // Start ack manager
    ackManager = fork();
    if (ackManager == -1) {
        printf("Ack Manager not created!\n");
    } else if (ackManager == 0) {
        // Set normal signal mask
        sigprocmask(SIG_SETMASK, &mySet, NULL);
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
            // printf("AckArray:\n");
            semOp(semAckListId, 0, -1);
            //  print_ackArray(ackList->arr, MAX_ACK);
            // trovo un messaggio confermato da tutti i devices
            achedId = idAckedByAll(ackList->arr, MAX_ACK);
            // printf("Id acked by all :%d\n", achedId);
            if (achedId != -1) {
                add_arr(confirmedList->a, MAX_ID, achedId);
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
        // Create and starting devices D1,...,D5 (5 children)
        for (child = 0; child < CHILD_NUM; ++child) {
            pid_child[child] = fork();
            if (pid_child[child] == -1)
                printf("child %d not created!", child);
            else if (pid_child[child] == 0) {
                // Set only sigcont and sigterm
                sigdelset(&prevSet, SIGCONT);
                sigdelset(&prevSet, SIGTERM);
                sigprocmask(SIG_SETMASK, &prevSet, NULL);
                // set signal handler
                if (signal(SIGTERM, childSigHandler) == SIG_ERR ||
                    signal(SIGCONT, childSigHandler) == SIG_ERR)
                    ErrExit("change signal handler failed");
                // create device fifo
                get_fifo_path_child(child_fifo_path, getpid());
                child_fifo_fd = get_fifo_child(child_fifo_path);
                int x = -1, y = -1;
                Message msgs[MAX_ID] = {0};
                while (1) {
                    while (1) {
                        Message msg_buff = {0};
                        int numRead = read_fifo(child_fifo_fd, &msg_buff,
                                                sizeof(Message));
                        if (numRead == 0) break;
                        if (numRead == -1) break;
                        if (numRead == sizeof(Message)) {
                            if (exists_arr(confirmedList->a, MAX_ID,
                                           msg_buff.message_id) != 0 &&
                                add_messageArray(&msg_buff, msgs, MAX_ID) ==
                                    0) {
                                Acknowledgment ack_buff;
                                new_Acknowledgment(
                                    &ack_buff, msg_buff.pid_sender,
                                    msg_buff.pid_receiver, msg_buff.message_id,
                                    time(NULL));
                                semOp(semAckListId, 0, -1);
                                add_ackArray(&ack_buff, ackList->arr, MAX_ACK);
                                semOp(semAckListId, 0, 1);
                            }
                        }
                    }
                    semOp(semIdMatrix, 0, -1);
                    set_idArray(idMatrix->m[child], msgs, MAX_ID);
                    semOp(semIdMatrix, 0, 1);

                    for (int i = 0; i < MAX_ID; i++) {
                        Message msg_buff = {0};
                        if (eq_message(&msgs[i], &msg_buff) != 0) {
                            new_Message(&msg_buff, msgs[i].pid_sender,
                                        msgs[i].pid_receiver,
                                        msgs[i].message_id, msgs[i].message,
                                        msgs[i].max_distance);

                            int is_near[CHILD_NUM] = {0};
                            if (x != -1 && y != -1) {
                                semOp(semBoardId, 0, -1);
                                nearby_pids(board, BOARD_SIZE, BOARD_SIZE, x, y,
                                            msg_buff.max_distance, is_near,
                                            CHILD_NUM);
                                semOp(semBoardId, 0, 1);
                            }
                            for (int j = 0; j < CHILD_NUM; j++) {
                                if (is_near[j] != 0 && is_near[j] != getpid()) {
                                    msg_buff.pid_sender = getpid();
                                    msg_buff.pid_receiver = is_near[j];
                                    semOp(semAckListId, 0, -1);
                                    int isAckedArray = is_ackedArray(
                                        &msg_buff, ackList->arr, MAX_ACK);
                                    semOp(semAckListId, 0, 1);
                                    if (isAckedArray != 0) {
                                        char nearby_fifo_path[SIZE_FIFO_PATH];
                                        get_fifo_path_child(nearby_fifo_path,
                                                            is_near[j]);
                                        int nearby_fifo_fd =
                                            open_fifo(nearby_fifo_path,
                                                      O_WRONLY | O_NONBLOCK);
                                        write_fifo(nearby_fifo_fd, &msg_buff,
                                                   sizeof(Message));
                                        del_messageById(msg_buff.message_id,
                                                        msgs, MAX_ID);
                                    }
                                }
                            }
                        }
                    }
                    if (child == 0) pause();
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
                    child == 4 ? semOp(semProcId, 0, 1)
                               : semOp(semProcId, child + 1, 1);
                }
                exit(0);
            }
        }
        sleep(1);
        while (1) {
            step++;
            int numRead = read(pos_fd, posLine, LINE_SIZE);
            if (numRead == -1) ErrExit("error reading position file");
            if (numRead == 0) {
                printf("<server> End of position file!\n");
                kill(pid_child[0], SIGCONT);
            }
            if (numRead > 0) kill(pid_child[0], SIGCONT);
            sleep(2);
            semOp(semBoardId, 0, -1);
            semOp(semIdMatrix, 0, -1);
            print_board_status(board, BOARD_SIZE, BOARD_SIZE, &pid_child[0],
                               DEV_NUM, idMatrix, step);
            semOp(semIdMatrix, 0, 1);
            semOp(semBoardId, 0, 1);
        }
    }
    return 0;
}
void init() {
    shmBoardId =
        alloc_shared_memory(IPC_PRIVATE, sizeof(int) * BOARD_SIZE * BOARD_SIZE);
    board = (int *)get_shared_memory(shmBoardId, 0);
    semBoardId = create_sem_set(1);
    semOp(semBoardId, 0, 1);

    shmIdMatrix = alloc_shared_memory(IPC_PRIVATE, sizeof(IdMatrix));
    idMatrix = (IdMatrix *)get_shared_memory(shmIdMatrix, 0);
    semIdMatrix = create_sem_set(1);
    semOp(semIdMatrix, 0, 1);

    shmConfirmedList = alloc_shared_memory(IPC_PRIVATE, sizeof(ConfirmedList));
    confirmedList = (ConfirmedList *)get_shared_memory(shmConfirmedList, 0);
    semConfirmedList = create_sem_set(1);
    semOp(semConfirmedList, 0, 1);

    shmPosId = alloc_shared_memory(IPC_PRIVATE, sizeof(char) * LINE_SIZE);
    posLine = (char *)get_shared_memory(shmPosId, 0);
    semPosId = create_sem_set(1);
    semOp(semPosId, 0, 1);

    shmAckListId = alloc_shared_memory(IPC_PRIVATE, sizeof(AckList));
    ackList = (AckList *)get_shared_memory(shmAckListId, 0);
    semAckListId = create_sem_set(1);
    semOp(semAckListId, 0, 1);

    semProcId = create_sem_set(5);
    semOp(semProcId, 0, 1);

    usedIdFileDs = open(USED_ID_PATH, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (usedIdFileDs == -1) printf("Error creating file for used id\n");
    semFileId = create_sem_set_by_key(SEM_ID_FILE_KEY, 1);
    semOp(semFileId, 0, 1);
}
void quit() {
    kill(ackManager, SIGTERM);
    kill(pid_child[0], SIGTERM);
    kill(pid_child[1], SIGTERM);
    kill(pid_child[2], SIGTERM);
    kill(pid_child[3], SIGTERM);
    kill(pid_child[4], SIGTERM);

    if (close(pos_fd) == -1) printf("error closing file descriptor");

    free_shared_memory(board);
    free_shared_memory(ackList);
    free_shared_memory(idMatrix);
    free_shared_memory(confirmedList);

    remove_shared_memory(shmBoardId);
    remove_shared_memory(shmAckListId);
    remove_shared_memory(shmAckListId);
    remove_shared_memory(shmConfirmedList);

    close(usedIdFileDs);
    unlink(USED_ID_PATH);

    delete_sem_set(semBoardId);
    delete_sem_set(semProcId);
    delete_sem_set(semPosId);
    delete_sem_set(semFileId);
    delete_sem_set(semIdMatrix);
    delete_sem_set(semConfirmedList);

    exit(0);
}
void serverSigHandler(int sig) { quit(); }
void childSigHandler(int sig) {
    if (sig == SIGTERM) {
        close_fifo(child_fifo_fd);
        unlink_fifo(child_fifo_path);
        exit(0);
    }
}
void ackManagerSigHandler(int sig) {
    if (sig == SIGTERM) {
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
