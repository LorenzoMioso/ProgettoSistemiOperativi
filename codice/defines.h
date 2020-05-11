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
void print_acknowledgement(Acknowledgment a);
void print_board_status(int pos_device, int msg_id);
#endif
