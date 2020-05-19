/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.
#include <time.h>
#ifndef _DEFINES_HH
#define _DEFINES_HH
#pragma once

typedef struct {
    pid_t pid_sender;
    pid_t pid_receiver;
    int message_id;
    char message[256];
    double max_distance;
} Message;

typedef struct {
    pid_t pid_sender;
    pid_t pid_receiver;
    int message_id;
    long int timestamp;
} Acknowledgment;

void print_message(Message *m);
void print_acknowledgement(Acknowledgment *a);
void print_board_status(int pos_device, int msg_id);
void print_board(int *mat, int row, int col);
void set_table_val(int *mat, int x, int y, int val);
void nearby_pids(int *mat, int row, int col, int x, int y, int max_dist,
                 int pid_child, int pid_chilidren[], int is_near[],
                 int num_child);
void errExit(const char *msg);

#endif
