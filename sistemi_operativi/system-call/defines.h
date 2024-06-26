/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.
#include <bits/types/time_t.h>
#include <sys/types.h>
#include <time.h>

#ifndef _DEFINES_HH
#define _DEFINES_HH
#pragma once

#define MAX_ACK 100
#define MAX_ID 100
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
} AckQueue;  // messaggio nella queue contente i 5 ack

typedef struct {
    int m[DEV_NUM][MAX_ID];
} IdMatrix;  // matrice che conterrà gli id dei messaggi letti

typedef struct {
    int a[MAX_ID];
} ConfirmedList;

// funzioni per la board
void print_board_status(int *board, int col, int row, int *pidArr, size_t size,
                        IdMatrix *idMatrix, int step);
void print_board(int *mat, int row, int col);
void set_table_val(int *mat, int x, int y, int val);
int get_table_val(int *mat, int x, int y);
double dist_euclid(int x1, int y1, int x2, int y2);
void where_table_val(int *mat, int row, int col, int *x, int *y, int val);
void nearby_pids(int *mat, int row, int col, int x, int y, int max_dist,
                 int is_near[], int num_child);

// funzioni per i messaggi
void print_message(Message *m);
void new_Message(Message *msg, pid_t sender, pid_t receiver, int id,
                 char *message, double max_distance);
int add_messageArray(Message *msg, Message *msg_arr, size_t size);
int exists_messageArray(Message *msg, Message *msg_arr, size_t size);
void print_messageArray(Message *msg_arr, size_t size);
void set_zero_Message(Message *msg);
int is_ackedArray(Message *msg, Acknowledgment *ack_arr, size_t size);
int count_ackedId(int msg_id, Acknowledgment *ack_arr, size_t size);
int ackedMsgToString(Message *msg, Acknowledgment *ack_list, size_t size,
                     char *buff);
int eq_message(Message *msg1, Message *msg2);
void del_messageById(int message_id, Message *msg_arr, size_t size);

// funzioni per gli ack
void new_Acknowledgment(Acknowledgment *ack, pid_t sender, pid_t receiver,
                        int id, time_t timestamp);
void print_acknowledgement(Acknowledgment *a);
int acknowledgementToString(Acknowledgment *a, char *buff);
int eq_acknowledgement(Acknowledgment *ack1, Acknowledgment *ack2);

// funzioni per l'array di ack
void print_ackArray(Acknowledgment *ack_arr, size_t size);
int is_emptyArray(Acknowledgment *ack_arr, size_t size);
int exists_ackArray(Acknowledgment *ack, Acknowledgment *ack_arr, size_t size);
int add_ackArray(Acknowledgment *ack, Acknowledgment *ack_arr, size_t size);
int del_ackArray(Acknowledgment *ack, Acknowledgment *ack_arr, size_t size);
void del_ackArrayById(int message_id, Acknowledgment *ack_arr, size_t size);
int idAckedByAll(Acknowledgment *ack_arr, size_t size);

// funzioni per la message queue
void print_ackQueue(AckList *ack_list, size_t size);
void setAckQueue(Acknowledgment *ack_arr_q, size_t size_arr_q,
                 Acknowledgment *ack_arr, size_t size_arr, int message_id);

// array utils
void set_zero(int arr[], int size);
void add_arr(int arr[], int size, int val);
int exists_arr(int arr[], int size, int val);
void del_arr(int arr[], int size, int val);
void print_array(int *arr, int size);
void set_idArray(int *arr, Message *msg_arr, int size);
#endif
