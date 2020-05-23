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
    return memcmp(ack1, ack2, sizeof(Acknowledgment));
}
void print_ackList(Acknowledgment *ack_list, size_t size) {
    for (int i = 0; i < size; i++) {
        printf("sono nel for \n");
        print_acknowledgement(&ack_list[i]);
    }
}
void add_ackList(Acknowledgment *ack, Acknowledgment *ack_list, size_t size) {}
void del_ackList(Acknowledgment *ack, Acknowledgment *ack_list, size_t size);

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
