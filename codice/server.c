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

//-----------------------------
int shmBoardId;
int semBoardId;
int *board;
//-----------------------------
int shmAckListId;
int semAckListId;
AckList *ackList;
//-----------------------------
int pos_fd;
int shmPosId;
int semPosId;  // TODO usare semeforo per accedere alla posizione
char *posLine;
//-----------------------------
int ackManager;
//-----------------------------
int msqid = -1;
//-----------------------------
sigset_t prevSet;
int pid_child[CHILD_NUM];
int child;
int semProcId;
char child_fifo_path[SIZE_FIFO_PATH];
int child_fifo_fd;
int move = 0;

void serverSigHandler(int sig);
void childSigHandler(int sig);
void ackManagerSigHandler(int sig);
void set_table_val_sem(int *board, int x, int y, int val, int sem_id);
void get_fifo_path_child(char *path, pid_t pid);
int get_fifo_child(char *fifo_path);

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
    // Remove otger signals
    //-----------------------------------------------------------------------------

    sigset_t mySet;
    // initialize mySet to contain all signals
    sigfillset(&mySet);
    // remove SIGTERM from mySet
    // sigdelset(&mySet, SIGTERM);
    sigdelset(&mySet, SIGINT);
    // blocking all signals but SIGTERM
    sigprocmask(SIG_SETMASK, &mySet, &prevSet);
    /* Code that shouldn’t be interrupted by signals but SIGTERM */

    //-----------------------------------------------------------------------------
    // Set signal handler
    //-----------------------------------------------------------------------------

    // if (signal(SIGTERM, serverSigHandler) == SIG_ERR)
    if (signal(SIGINT, serverSigHandler) == SIG_ERR)
        ErrExit("change signal handler failed");

    //-----------------------------------------------------------------------------
    // Start ack manager
    //-----------------------------------------------------------------------------
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

        // creating message to send
        AckGroup g;
        g.mtype = 1;
        size_t gSize = sizeof(AckGroup) - sizeof(long);
        // sending message
        // sending the message in the queue
        if (msgsnd(msqid, &g, gSize, IPC_NOWAIT) == -1) {
            if (errno == EAGAIN) {
                printf("The queue was full! \n");
            } else {
                ErrExit("msgsnd failed");
            }
        }

        // managing acks
        while (1) {
            sleep(5);
            printf("AckList:\n");
            semOp(semAckListId, 0, -1);
            print_ackList(ackList, MAX_ACK);
            semOp(semAckListId, 0, 1);
        }
    } else {
        printf("Server on going!\n");
        //-----------------------------------------------------------------------------
        // Create and starting devices D1,...,D5 (5 children)
        //-----------------------------------------------------------------------------

        for (child = 0; child < CHILD_NUM; ++child) {
            pid_child[child] = fork();

            if (pid_child[child] == -1)
                printf("child %d not created!", child);

            else if (pid_child[child] == 0) {
                // Set normal signal mask
                sigprocmask(SIG_SETMASK, &prevSet, NULL);
                // set signal handler
                if (signal(SIGUSR1, childSigHandler) == SIG_ERR ||
                    signal(SIGTERM, childSigHandler) == SIG_ERR)
                    ErrExit("change signal handler failed");

                // create device fifo
                get_fifo_path_child(child_fifo_path, getpid());
                child_fifo_fd = get_fifo_child(child_fifo_path);
                int x = -1, y = -1;

                while (1) {
                    sleep(1);

                    Message msg_buff;
                    Acknowledgment ack_buff;
                    int is_near[CHILD_NUM];
                    int numRead = 0;
                    int dist = 0;

                    // red message from pipe
                    numRead =
                        read_fifo(child_fifo_fd, &msg_buff, sizeof(Message));
                    if (numRead == sizeof(Message)) {
                        // printf("<D%d>Messaggio letto\n", child);
                        dist = msg_buff.max_distance;
                        // print_message(&msg_buff);
                        // crea l ack
                        ack_buff.pid_sender = msg_buff.pid_sender;
                        ack_buff.pid_receiver = msg_buff.pid_receiver;
                        ack_buff.message_id = msg_buff.message_id;
                        ack_buff.timestamp = time(NULL);
                        // print_acknowledgement(&ack_buff);
                        // scrivo ack nellla lista di acks
                        semOp(semAckListId, 0, -1);
                        if (add_ackList(&ack_buff, ackList, MAX_ACK) == -1) {
                            printf("Ack non aggiunto\n");
                            numRead = 0;
                        }
                        semOp(semAckListId, 0, 1);
                    }

                    // find nerby devices
                    set_zero(is_near, CHILD_NUM);
                    if (x != -1 && y != -1) {
                        semOp(semBoardId, 0, -1);
                        nearby_pids(board, BOARD_SIZE, BOARD_SIZE, x, y, dist,
                                    getpid(), is_near, CHILD_NUM);
                        semOp(semBoardId, 0, 1);
                    }
                    // send message to nearby_pids
                    if (numRead != 0) {
                        for (int i = 0; i < CHILD_NUM; i++) {
                            if (is_near[i] != 0 && is_near[i] != getpid()) {
                                //     change sender and receiver
                                msg_buff.pid_sender = getpid();
                                msg_buff.pid_receiver = is_near[i];
                                // check if device has already recieved the
                                // mesage by checking on ack list
                                if (is_acked(&msg_buff, ackList, MAX_ACK) ==
                                    0) {
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

                    if (move == 1) {
                        // printf("<D%d>Tocca a me \n", child);
                        semOp(semProcId, child, -1);

                        if (x != -1 && y != -1)
                            set_table_val_sem(board, x, y, 0, semBoardId);
                        x = posLine[POSX(child)] - '0';
                        y = posLine[POSY(child)] - '0';
                        set_table_val_sem(board, x, y, getpid(), semBoardId);

                        move = 0;
                        // printf("<D%d>Mi sto muovendo\n", child);
                        child == 4 ? semOp(semProcId, 0, 1)
                                   : semOp(semProcId, child + 1, 1);
                    }
                }
                exit(0);
            }
        }

        sleep(5);

        while (1) {
            int numRead = read(pos_fd, posLine, LINE_SIZE);
            if (numRead == -1) ErrExit("error reading position file");
            if (numRead == 0)
                ;  // printf("<server> File posizioni terminato
                   // \n");
            if (numRead > 0) {
                //  kill(0, SIGCONT);
                kill(pid_child[0], SIGUSR1);
                kill(pid_child[1], SIGUSR1);
                kill(pid_child[2], SIGUSR1);
                kill(pid_child[3], SIGUSR1);
                kill(pid_child[4], SIGUSR1);
                sleep(1);
                // print_board(board, BOARD_SIZE, BOARD_SIZE);
                // printf("--------------------------- \n");
            }
            sleep(2);
        }
    }

    while (1) {
        sleep(10);
    }
    return 0;
}

void init() {
    //-----------------------------------------------------------------------------
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
    // Allocate a shared memory segment for scharing positin (sever ->
    // childern)
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
    shmAckListId = alloc_shared_memory(IPC_PRIVATE, sizeof(AckList));

    //-----------------------------------------------------------------------------
    // Attach the shared memory segment for acknowledgment list
    //-----------------------------------------------------------------------------

    printf(
        "<Server> attaching the shared memory segment acknowledgment "
        "list\n");
    ackList = (AckList *)get_shared_memory(shmAckListId, 0);

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
    // Create a semaphore for ack list access
    //-----------------------------------------------------------------------------

    printf("<Server> creating a semaphore set for ack list access.\n");
    semAckListId = create_sem_set(1);

    //-----------------------------------------------------------------------------
    // Set semaphore at 1 to allow ack list access
    //-----------------------------------------------------------------------------

    printf("<Server> Setting ack list semaphore to 1.\n");
    semOp(semAckListId, 0, 1);
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

    printf("<Server> Dataching memory.\n");
    free_shared_memory(board);
    free_shared_memory(ackList);

    printf("<Server> Removing memory.\n");
    remove_shared_memory(shmBoardId);
    remove_shared_memory(shmAckListId);

    exit(0);
}
void serverSigHandler(int sig) { quit(); }
void childSigHandler(int sig) {
    if (sig == SIGUSR1) {
        // printf("SIGUSR1 segnale ricevuto\n");
        move = 1;
    } else if (sig == SIGTERM) {
        printf("<Child> SIGTERM ricevuto\n");
        close_fifo(child_fifo_fd);
        exit(0);
    }
}
void ackManagerSigHandler(int sig) {
    if (sig == SIGTERM) {
        printf("<Ack> SIGTERM ricevuto\n");
        exit(0);
    }
}
void set_table_val_sem(int *board, int x, int y, int val, int sem_id) {
    // lock semaphore
    semOp(sem_id, 0, -1);
    // set value on the table
    set_table_val(board, x, y, val);
    // unlock semaphore
    semOp(sem_id, 0, 1);
}

void get_fifo_path_child(char *path, pid_t pid) {
    char fifo_name[22] = "/tmp/dev_fifo.";
    char pid_to_strg[12];
    sprintf(pid_to_strg, "%d", pid);
    strcat(fifo_name, pid_to_strg);
    strcpy(path, fifo_name);
}
int get_fifo_child(char *fifo_path) {
    mk_fifo(fifo_path, IPC_CREAT | S_IRUSR | S_IWUSR);
    return open_fifo(fifo_path, O_RDONLY | O_NONBLOCK);
}
