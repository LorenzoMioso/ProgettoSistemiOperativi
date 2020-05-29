/// @file client.c
/// @brief Contiene l'implementazione del client.

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "defines.h"
#include "err_exit.h"
#include "fifo.h"
#include "semaphore.h"
#include "shared_memory.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s msg_queue_key file_posizioni\n", argv[0]);
        exit(1);
    }

    key_t msgKey = atoi(argv[1]);
    if (msgKey <= 0) {
        printf("The msg_queue_key must be greater than zero!\n");
        exit(1);
    }

    int msqid = msgget(msgKey, S_IRUSR | S_IWUSR);
    if (msqid == -1) ErrExit("msgget failed");

    // input pid con controlli
    char fifo_name[22];
    int dev_pid;
    int fifo_fd = -1;
    do {
        printf("Insersci il pid del device a cui vuoi connetterti : ");
        scanf("%d", &dev_pid);
        if (dev_pid <= 0) {
            printf("Pid deve essere > 0\n");
        } else {
            sprintf(fifo_name, "/tmp/dev_fifo.%d", dev_pid);
            fifo_fd = open(fifo_name, O_WRONLY);
            if (fifo_fd == -1) {
                perror("Error openig fifo");
            }
        }
    } while (fifo_fd <= 0 && fifo_fd == -1);

    // input message id con controlli
    int msg_id = 0;
    do {
        printf("Insersci id del messaggio : ");
        scanf("%d", &msg_id);
        if (msg_id <= 0) printf("Message id must be > 0\n");
    } while (msg_id <= 0);

    // imput message
    char msg_txt[256];
    printf("Insersci testo del messaggio : ");
    scanf("%s", msg_txt);

    // input distanza con controlli
    int msg_dist = 0;
    do {
        printf("Insersci la distanza del messaggio : ");
        scanf("%d", &msg_dist);
        if (msg_dist <= 0) printf("Distance must be > 0\n");
    } while (msg_dist <= 0);

    // message creation
    Message msg;
    msg.pid_sender = getpid();
    msg.pid_receiver = dev_pid;
    msg.message_id = msg_id;
    strcpy(msg.message, msg_txt);
    msg.max_distance = msg_dist;

    int numWrite = write(fifo_fd, &msg, sizeof(Message));
    if (numWrite == -1) printf("error writing message\n");
    if (numWrite == 0) printf("non e stato scritto nulla\n");
    if (numWrite == sizeof(Message)) {
        printf("Attendo le conferme...\n");
        // creo un messaggio della queue
        AckQueue q;
        size_t gSize = sizeof(AckQueue) - sizeof(long);
        // ricevo il messaggio
        if (msgrcv(msqid, &q, gSize, msg_id, 0) == -1)
            printf("msgrcv failed\n");
        // predispongo gli ack da scrivere nel file
        char out[255];
        int num = ackedMsgToString(&msg, q.arr, DEV_NUM, out);
        // creo il file
        char filePath[20];
        sprintf(filePath, "out_%d.txt", msg_id);
        int fileds =
            open(filePath, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
        if (fileds == -1)
            printf("Error creating file\n");
        else
            // scrivo gli ack nel file
            write(fileds, out, num);
    }
    return 0;
}
