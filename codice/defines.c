/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

#include "defines.h"

void print_message(Message *m){
	printf("Message:\n");
	printf("Pid sender: %d\n",m->pid_sender);
	printf("Pid receiver: %d\n",m->pid_receiver);
	printf("Message id: %d\n",m->message_id);
	printf("Message text: %s\n", m->message);
	printf("Max distance: %f\n", m->max_distance);
}
void print_acknowledgement(Acknowledgment a){
	printf("Pid sender: %d\n",a->pid_sender);
	printf("Pid receiver: %d\n",a->pid_receiver);
	printf("Message id: %d\n",a->message_id);
	printf("Timestamp: %s\n", ctime(a->timestamp));
}
void print_board_status(int pos_device, int msg_id){
	

}
