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
    printf("Max distance: %f\n", m->max_distance);
}

void print_acknowledgement(Acknowledgment *a) {
    char buff[100];
    strftime(buff, 100, "%Y-%m-%d %H:%M:%S", localtime(&a->timestamp));
    printf("%d, ", a->pid_sender);
    printf("%d, ", a->pid_receiver);
    printf("%s\n", buff);
}

int eq_acknowledgement(Acknowledgment *ack1, Acknowledgment *ack2) {
    int ret = 0;
    if (ack1->message_id == ack2->message_id &&
        ack1->pid_receiver == ack2->pid_receiver &&
        ack1->pid_sender == ack2->pid_sender)
        ret = 1;
    return ret;
}

void print_ackList(AckList *ack_list, size_t size) {
    Acknowledgment null = {0};
    for (int i = 0; i < size; i++) {
        if (eq_acknowledgement(&ack_list->list[i], &null) == 0)
            print_acknowledgement(&ack_list->list[i]);
    }
}
int add_ackList(Acknowledgment *ack, AckList *ack_list, size_t size) {
    if (exists_ackList(ack, ack_list, size) == 0) {
        int i = 0;
        Acknowledgment null = {0};
        while (i < size) {
            if (eq_acknowledgement(&ack_list->list[i], &null) == 1) {
                ack_list->list[i] = *ack;
                return 0;
            }
            i++;
        }
    }
    return -1;
}
int del_ackList(Acknowledgment *ack, AckList *ack_list, size_t size) {
    int i = 0;
    Acknowledgment null = {0};
    while (i < size) {
        if (eq_acknowledgement(&ack_list->list[i], ack) == 0) {
            ack_list->list[i] = null;
            return 0;
        }
        i++;
    }
    return -1;
}
int exists_ackList(Acknowledgment *ack, AckList *ack_list, size_t size) {
    int i = 0;
    while (i < size) {
        if (eq_acknowledgement(&ack_list->list[i], ack) == 0) {
            return 0;
        }
        i++;
    }
    return 1;
}
int is_acked(Message *msg, AckList *ack_list, size_t size) {
    int i = 0;
    Acknowledgment null = {0};
    while (i < size) {
        if (eq_acknowledgement(&ack_list->list[i], &null) == 0) {
            Acknowledgment ack = ack_list->list[i];
            if (ack.message_id == msg->message_id &&
                ack.pid_receiver == msg->pid_receiver) {
                return 1;
            }
        }
        i++;
    }
    return 0;
}
void print_board(int *mat, int row, int col) {
    int j;
    for (int i = 0; i < row; i++) {
        printf("|");
        for (j = 0; j < col - 1; j++) {
            printf("%d,", mat[i + j * 10]);
        }
        printf("%d|\n", mat[i + j * 10]);
    }
}
void set_table_val(int *mat, int x, int y, int val) { mat[10 * x + y] = val; }
int get_table_val(int *mat, int x, int y) { return mat[10 * x + y]; }
// scrive in nearby_pids tutti i pid che trova vicino a (x,y) escluso
// scorre tutta la matrice
// prende incosiderazione solo i punti che sitrovano a una distanza < di
// max_dist da (x,y) se il nella cella ce un numero != 0 lo mette in nearby_pids
// (x1,y1) e vicino di (x1,y1)
void nearby_pids(int *mat, int row, int col, int x, int y, int max_dist,
                 int pid_child, int is_near[], int num_child) {
    int dist, k = 0, j = 0;
    for (int i = 0; i < row; i++) {
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
}
int dist_euclid(int x1, int y1, int x2, int y2) {
    return (int)sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}
void set_zero(int arr[], int size) {
    for (int i = 0; i < size; i++) arr[i] = 0;
}
void print_board_status(int pos_device, int msg_id) {}
