/// @file client.c
/// @brief Contiene l'implementazione del client.

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
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

    char fifo_name[22] = "/tmp/dev_fifo.";
    char str_to_pid[12];
    int msg_id = 0;
    char msg_txt[256];
    int msg_dist = 0;
    Message msg;

    printf("Insersci il pid del device a cui vuoi connetterti : ");
    scanf("%s", str_to_pid);

    printf("Insersci id del messaggio : ");
    scanf("%d", &msg_id);

    printf("Insersci testo del messaggio : ");
    scanf("%s", msg_txt);

    printf("Insersci la distanza del messaggio : ");
    scanf("%d", &msg_dist);

    msg.pid_sender = getpid();
    msg.pid_receiver = atoi(str_to_pid);
    msg.message_id = msg_id;
    strcpy(msg.message, msg_txt);
    msg.max_distance = msg_dist;

    strcat(fifo_name, str_to_pid);
    int fifo_fd = open(fifo_name, O_WRONLY);
    if (fifo_fd == -1)
        printf("error opening fifo\n");
    else {
        printf("fifo been opened\n");
        int numWrite = write(fifo_fd, &msg, sizeof(Message));
        if (numWrite == -1) printf("error writing message\n");
        if (numWrite == 0) printf("non e stato scritto nulla\n");
        if (numWrite == sizeof(Message)) printf("Il messaggio e stato inviato\n");
    }

    return 0;
}
