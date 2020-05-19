/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

#include "defines.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

void print_message(Message *m) {
    printf("Message:\n");
    printf("Pid sender: %d\n", m->pid_sender);
    printf("Pid receiver: %d\n", m->pid_receiver);
    printf("Message id: %d\n", m->message_id);
    printf("Message text: %s\n", m->message);
    printf("Max distance: %f\n", m->max_distance);
}

void print_acknowledgement(Acknowledgment *a) {
    printf("Pid sender: %d\n", a->pid_sender);
    printf("Pid receiver: %d\n", a->pid_receiver);
    printf("Message id: %d\n", a->message_id);
    printf("Timestamp: %s\n", ctime(a->timestamp));
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

void nearby_pids(int *mat, int row, int col, int x, int y, int max_dist,
                 int pid_child, int pid_chilidren[], int is_near[],
                 int num_child) {
    int dist, j = 0;
    for (int i = 0; i < row; i++) {
        for (j = 0; j < col; j++) {
        }
    }
}

void print_board_status(int pos_device, int msg_id) {}

void errExit(const char *msg) {
    perror(msg);
    exit(1);
}
