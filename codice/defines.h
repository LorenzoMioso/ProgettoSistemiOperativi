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
    time_t timestamp;
} Acknowledgment;

void print_message(Message *m);
void print_acknowledgement(Acknowledgment *a);
int eq_acknowledgement(Acknowledgment *ack1, Acknowledgment *ack2);
void print_ackList(Acknowledgment *ack_list, size_t size);
void add_ackList(Acknowledgment *ack, Acknowledgment *ack_list, size_t size);
void del_ackList(Acknowledgment *ack, Acknowledgment *ack_list, size_t size);
void print_board_status(int pos_device, int msg_id);
void print_board(int *mat, int row, int col);
void set_table_val(int *mat, int x, int y, int val);
int get_table_val(int *mat, int x, int y);
void nearby_pids(int *mat, int row, int col, int x, int y, int max_dist,
                 int pid_child, int is_near[], int num_child);
int dist_euclid(int x1, int y1, int x2, int y2);
void set_zero(int arr[], int size);

#endif
