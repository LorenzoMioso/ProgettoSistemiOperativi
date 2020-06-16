/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

#include "defines.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_message(Message *m) {
    printf("Message:\n");
    printf("Pid sender: %d\n", m->pid_sender);
    printf("Pid receiver: %d\n", m->pid_receiver);
    printf("Message id: %d\n", m->message_id);
    printf("Message text: %s\n", m->message);
    printf("Max distance: %lf\n", m->max_distance);
}

void print_acknowledgement(Acknowledgment *a) {
    char buff[50];
    strftime(buff, 50, "%Y-%m-%d %H:%M:%S", localtime(&a->timestamp));
    printf("%d, ", a->pid_sender);
    printf("%d, ", a->pid_receiver);
    printf("%s\n", buff);
}
int acknowledgementToString(Acknowledgment *a, char *buff) {
    char timestamp[50];
    strftime(timestamp, 50, "%Y-%m-%d %H:%M:%S", localtime(&a->timestamp));
    int i = 0;
    i += sprintf(buff, "%d, ", a->pid_sender);
    i += sprintf(buff + i, "%d, ", a->pid_receiver);
    i += sprintf(buff + i, "%s\n", timestamp);
    return i;
}
int ackedMsgToString(Message *msg, Acknowledgment *ack_list, size_t size,
                     char *buff) {
    int i = 0;
    char ackString[400];
    int num;
    i += sprintf(buff, "Messaggio %d: %s\n", msg->message_id, msg->message);
    i += sprintf(buff + i, "Lista acknowledgment:\n");
    for (int j = 0; j < size; j++) {
        num = acknowledgementToString(ack_list + j, ackString);
        strncpy(buff + i, ackString, num);
        i += num;
    }
    return i;
}
// if ack1 == ack2 return 1
// else return 0
int eq_acknowledgement(Acknowledgment *ack1, Acknowledgment *ack2) {
    if (ack1->message_id == ack2->message_id &&
        ack1->pid_receiver == ack2->pid_receiver &&
        ack1->pid_sender == ack2->pid_sender)
        return 1;
    return 0;
}

void print_ackArray(Acknowledgment *ack_arr, size_t size) {
    Acknowledgment null = {0};
    for (int i = 0; i < size; i++)
        if (eq_acknowledgement(ack_arr + i, &null) == 0)
            print_acknowledgement(ack_arr + i);
}
int add_ackArray(Acknowledgment *ack, Acknowledgment *ack_arr, size_t size) {
    if (exists_ackArray(ack, ack_arr, size) == 0) {
        Acknowledgment null = {0};
        for (int i = 0; i < size; i++)
            if (eq_acknowledgement(ack_arr + i, &null) == 1) {
                *(ack_arr + i) = *ack;
                return 0;
            }
    }
    return -1;
}
int del_ackArray(Acknowledgment *ack, Acknowledgment *ack_arr, size_t size) {
    Acknowledgment null = {0};
    for (int i = 0; i < size; i++)
        if (eq_acknowledgement(ack_arr + i, ack) == 0) {
            *(ack_arr + i) = null;
            return 0;
        }
    return -1;
}
void del_ackArrayById(int message_id, Acknowledgment *ack_arr, size_t size) {
    Acknowledgment null = {0};
    for (int i = 0; i < size; i++)
        if ((ack_arr + i)->message_id == message_id) {
            *(ack_arr + i) = null;
        }
}
int exists_ackArray(Acknowledgment *ack, Acknowledgment *ack_arr, size_t size) {
    for (int i = 0; i < size; i++)
        if (eq_acknowledgement(ack_arr + i, ack) == 0) return 0;
    return 1;
}
// controlla nell array degli ack se c'è gia una conferma del messaggio
// confrontando id e pid_receiver
int is_ackedArray(Message *msg, Acknowledgment *ack_arr, size_t size) {
    Acknowledgment null = {0};
    for (int i = 0; i < size; i++) {
        if (eq_acknowledgement(ack_arr + i, &null) == 0) {
            Acknowledgment ack = *(ack_arr + i);
            if (ack.message_id == msg->message_id &&
                ack.pid_receiver == msg->pid_receiver) {
                return 1;
            }
        }
    }
    return 0;
}
int is_emptyArray(Acknowledgment *ack_arr, size_t size) {
    Acknowledgment null = {0};
    for (int i = 0; i < size; i++)
        if (eq_acknowledgement(ack_arr + i, &null) == 0) return 0;
    return 1;
}
int idAckedByAll(Acknowledgment *ack_arr, size_t size) {
    if (is_emptyArray(ack_arr, size) == 1) return 0;
    int cont = 0;
    Acknowledgment check = {0};
    Acknowledgment null = {0};
    // prendo il primo ack diverso da 0
    for (int i = 0; i < size; i++) {
        if (eq_acknowledgement(ack_arr + i, &null) == 0) {
            check = *(ack_arr + i);
            break;
        }
    }
    // controllo se ci sono 5 acks con message_id uguale a check
    for (int i = 0; i < size; i++) {
        if ((ack_arr + i)->message_id == check.message_id) {
            cont++;
            if (cont == DEV_NUM) return check.message_id;
        }
    }
    return 0;
}
void setAckQueue(Acknowledgment *ack_arr_q, size_t size_arr_q,
                 Acknowledgment *ack_arr, size_t size_arr, int message_id) {
    int j = 0;
    for (int i = 0; i < size_arr; i++) {
        if ((ack_arr + i)->message_id == message_id && j < size_arr_q) {
            *(ack_arr_q + j) = *(ack_arr + i);
            j++;
        }
    }
}
void print_board(int *mat, int row, int col) {
    int j;
    for (int i = 0; i < row; i++) {
        printf("|");
        for (j = 0; j < col - 1; j++) printf("%d,", mat[i + j * 10]);
        printf("%d|\n", mat[i + j * 10]);
    }
}
void set_table_val(int *mat, int x, int y, int val) { mat[10 * x + y] = val; }
int get_table_val(int *mat, int x, int y) { return mat[10 * x + y]; }
void where_table_val(int *mat, int row, int col, int *x, int *y, int val) {
    int j;
    for (int i = 0; i < row; i++) {
        for (j = 0; j < col; j++) {
            if (mat[i + j * 10] == val) {
                *x = i;
                *y = j;
            }
        }
    }
}

// scrive in nearby_pids tutti i pid che trova vicino a (x,y) escluso
// scorre tutta la matrice
// prende incosiderazione solo i punti che sitrovano a una distanza < di
// max_dist da (x,y) se il nella cella ce un numero != 0 lo mette in nearby_pids
// (x1,y1) e vicino di (x1,y1)
void nearby_pids(int *mat, int row, int col, int x, int y, int max_dist,
                 int is_near[], int num_child) {
    int dist, k = 0, j = 0;
    for (int i = 0; i < row; i++)
        for (j = 0; j < col; j++) {
            dist = dist_euclid(x, y, i, j);
            if (dist <= max_dist) {
                int pid = get_table_val(mat, i, j);
                if (pid != 0) {
                    is_near[k] = pid;
                    k++;
                }
            }
        }
}
double dist_euclid(int x1, int y1, int x2, int y2) {
    return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}
void set_zero(int arr[], int size) {
    for (int i = 0; i < size; i++) arr[i] = 0;
}
void print_board_status(int *board, int col, int row, int *pidArr, size_t size,
                        int step) {
    printf("Step %d: device positions ########################\n", step);
    for (int i = 0; i < size; i++) {
        int x = -1, y = -1;
        where_table_val(board, row, col, &x, &y, *(pidArr + i));
        printf("%d %d %d\n", *(pidArr + i), x, y);
    }
    printf("#################################################\n");
}
void new_Acknowledgment(Acknowledgment *ack, pid_t sender, pid_t receiver,
                        int id, time_t timestamp) {
    ack->pid_sender = sender;
    ack->pid_receiver = receiver;
    ack->message_id = id;
    ack->timestamp = timestamp;
}
void new_Message(Message *msg, pid_t sender, pid_t receiver, int id,
                 char *message, double max_distance) {
    msg->pid_sender = sender;
    msg->pid_receiver = receiver;
    msg->message_id = id;
    strcpy(msg->message, message);
    msg->max_distance = max_distance;
}
