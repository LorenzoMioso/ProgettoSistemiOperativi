/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.
#include <time.h>

#ifndef _DEFINES_HH
#define _DEFINES_HH
#pragma once

#define MAX_ACK 100
#define DEV_NUM 5
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

typedef struct {
    Acknowledgment arr[MAX_ACK];
} AckList;

typedef struct {
    long mtype;
    Acknowledgment arr[DEV_NUM];
} AckQueue;

void print_ackQueue(AckList *ack_list, size_t size);
void print_board_status(int pos_device, int msg_id);
void print_board(int *mat, int row, int col);
int acknowledgementToString(Acknowledgment *a, char *buff);
int ackedMsgToString(Message *msg, Acknowledgment *ack_list, size_t size,
                     char *buff);
void set_table_val(int *mat, int x, int y, int val);
int get_table_val(int *mat, int x, int y);
void nearby_pids(int *mat, int row, int col, int x, int y, int max_dist,
                 int pid_child, int is_near[], int num_child);
int dist_euclid(int x1, int y1, int x2, int y2);
void set_zero(int arr[], int size);
void print_acknowledgement(Acknowledgment *a);
int eq_acknowledgement(Acknowledgment *ack1, Acknowledgment *ack2);

void print_ackArray(Acknowledgment *ack_arr, size_t size);
int add_ackArray(Acknowledgment *ack, Acknowledgment *ack_arr, size_t size);
int is_ackedArray(Message *msg, Acknowledgment *ack_arr, size_t size);
int is_emptyArray(Acknowledgment *ack_arr, size_t size);
int del_ackArray(Acknowledgment *ack, Acknowledgment *ack_arr, size_t size);
void del_ackArrayById(int message_id, Acknowledgment *ack_arr, size_t size);
int exists_ackArray(Acknowledgment *ack, Acknowledgment *ack_arr, size_t size);
int idAckedByAll(Acknowledgment *ack_arr, size_t size);
void setAckQueue(Acknowledgment *ack_arr_q, size_t size_arr_q,
                 Acknowledgment *ack_arr, size_t size_arr, int message_id);
#endif
