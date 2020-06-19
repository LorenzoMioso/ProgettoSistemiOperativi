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
#define OUTPUT_SIZE 600
#define USED_ID_PATH "/tmp/used.id"
#define INT_DIGIT_NUM 10
#define SEM_ID_FILE_KEY 1234

int is_valid_id(int usedIdFileDs, int id);
int append_id(int usedIdFileDs, int id);
int is_num(char* num);
int is_great_zero(int num);
int is_valid_fifo(int pid);

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
    // controllo esistenza file dei pid usati
    int usedIdFileDs = open(USED_ID_PATH, O_RDWR, 0);
    if (usedIdFileDs == -1) printf("Error opening file for used id\n");
    int semFileId = get_sem_set_by_key(SEM_ID_FILE_KEY);

    // input pid device con controlli
    char str_dev_pid[INT_DIGIT_NUM];
    int dev_pid;
    int fifo_fd;
    do {
        printf("Insersci il pid del device a cui vuoi connetterti : ");
        scanf("%s", str_dev_pid);
        dev_pid = atoi(str_dev_pid);
        if (dev_pid == 0) {
            printf("Numero non valido!\n");
            continue;
        }
        if (dev_pid <= 0) {
            printf("Devi inserire un numero maggiore di 0!\n");
            continue;
        }
        fifo_fd = is_valid_fifo(dev_pid);
        if (fifo_fd == -1) {
            printf("La fifo non esiste!\n");
            continue;
        }
    } while (dev_pid <= 0 || fifo_fd == -1);

    // input message id con controlli
    char str_msg_id[INT_DIGIT_NUM];
    int msg_id;
    int idIsValid;
    do {
        printf("Insersci id del messaggio : ");
        scanf("%s", str_msg_id);
        msg_id = atoi(str_msg_id);
        if (msg_id == 0) {
            printf("Numero non valido!\n");
            continue;
        }
        if (msg_id <= 0) {
            printf("Devi inserire un numero maggiore di 0!\n");
            continue;
        }
        semOp(semFileId, 0, -1);
        idIsValid = is_valid_id(usedIdFileDs, msg_id);
        semOp(semFileId, 0, 1);
        if (idIsValid == 1) {
            printf("Id già utilizato!\n");
            continue;
        }
    } while (msg_id <= 0 || idIsValid != 0);

    // trova un id valido lo aggiungo al file per indicare che è utilizzato
    semOp(semFileId, 0, -1);
    append_id(usedIdFileDs, msg_id);
    semOp(semFileId, 0, 1);

    // imput message
    char msg_txt[256];
    printf("Insersci testo del messaggio : ");
    scanf("%s", msg_txt);

    // input distanza con controlli
    char str_msg_dist[INT_DIGIT_NUM * 2];
    double msg_dist = 0;
    do {
        printf("Insersci la distanza del messaggio : ");
        scanf("%s", str_msg_dist);
        msg_dist = atof(str_msg_dist);
        if (msg_dist == 0) {
            printf("Numero non valido!\n");
            continue;
        }
        if (msg_dist <= 0) {
            printf("Devi inserire un numero maggiore di 0!\n");
            continue;
        }
    } while (msg_dist <= 0);

    // message creation
    Message msg;
    new_Message(&msg, getpid(), dev_pid, msg_id, msg_txt, msg_dist);

    int numWrite = write(fifo_fd, &msg, sizeof(Message));
    if (numWrite == -1) printf("error writing message\n");
    if (numWrite == 0) printf("non e stato scritto nulla\n");
    if (numWrite == sizeof(Message)) {
        printf("Attendo le conferme...\n");
        // creo un buffer per il messaggio della queue
        AckQueue q;
        size_t gSize = sizeof(AckQueue) - sizeof(long);
        // ricevo il messaggio
        if (msgrcv(msqid, &q, gSize, msg_id, 0) == -1)
            printf("msgrcv failed\n");
        // predispongo gli ack da scrivere nel file
        char out[OUTPUT_SIZE];
        int num = ackedMsgToString(&msg, q.arr, DEV_NUM, out);
        // creo il file
        char filePath[20];
        sprintf(filePath, "out_%d.txt", msg_id);
        int fileds = open(filePath, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        if (fileds == -1)
            printf("Error creating file\n");
        else
            // scrivo gli ack nel file
            write(fileds, out, num);
    }
    return 0;
}
int is_valid_id(int usedIdFileDs, int id) {
    int numRead;
    char c;
    char str_num[INT_DIGIT_NUM];
    int int_num = 0;
    int cont = 0;
    // mi sposto all inizio del file
    lseek(usedIdFileDs, 0, SEEK_SET);
    while (1) {
        numRead = read(usedIdFileDs, &c, sizeof(char));
        if (numRead == -1) return -1;
        if (numRead == 0) return 0;
        if ((int)c == 10) {
            int_num = atoi(str_num);
            if (id == int_num) return 1;
            memset(str_num, 0, INT_DIGIT_NUM);
            cont = 0;
        }
        str_num[cont] = c;
        cont++;
    }
}
int append_id(int usedIdFileDs, int id) {
    char str_num[INT_DIGIT_NUM + 2];
    sprintf(str_num, "%d", id);
    strcat(str_num, "\n");
    lseek(usedIdFileDs, 0, SEEK_END);
    return write(usedIdFileDs, str_num, strlen(str_num));
}
int is_num(char* num) {
    int n = 0;
    n = atoi(num);
    if (n == 0) {
        printf("Non hai inserito un numero\n");
        return 0;
    }
    return n;
}
int is_great_zero(int num) {
    if (num <= 0) {
        printf("Non hai inserito un numero maggiore di 0\n");
        return 0;
    }
    return 1;
}
int is_valid_fifo(int pid) {
    char fifo_name[22];
    sprintf(fifo_name, "/tmp/dev_fifo.%d", pid);
    return open(fifo_name, O_WRONLY | O_NONBLOCK);
}
