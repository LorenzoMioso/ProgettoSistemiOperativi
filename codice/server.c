/// @file server.c
/// @brief Contiene l'implementazione del SERVER.

#include "defines.h"
#include "err_exit.h"
#include "fifo.h"
#include "semaphore.h"
#include "shared_memory.h"
#define BOARD_SIZE 10
#define MAX_ACK 100

int main(int argc, char* argv[]) {
    // check command line input arguments
    if (argc != 3) {
        printf("Usage: %s msg_queue_key file_posizioni\n", argv[0]);
        exit(1);
    }

    key_t msgKey = atoi(argv[1]);
    if (msgKey <= 0) {
        printf("The msg_queue_key must be greater than zero!\n");
        exit(1);
    }
	
    int fd = open(argv[2], O_RDONLY);
    if (fd == -1) 
		printf("File %s does not exist\n", argv[2]);
	
	
	// allocate a shared memory segment for board
    printf("<Server> allocating a shared memory segment for board...\n");
	int shmidBoard = alloc_shared_memory(IPC_PRIVATE, sizeof(int) * BOARD_SIZE * BOARD_SIZE);
	// attach the shared memory segment for board
    printf("<Server> attaching the shared memory segment...\n");
    int **board= (int **)get_shared_memory(shmidBoard, 0);

	// allocate a shared memory segment for acknowledgment list
    printf("<Server> allocating a shared memory segment for acknowledgment list...\n");
	int shmid_ack_list = alloc_shared_memory(IPC_PRIVATE, sizeof(Acknowledgment) * MAX_ACK);
	// attach the shared memory segment for acknowledgment list
    printf("<Server> attaching the shared memory segment ...\n");
    Acknowledgment *ack_list= (Acknowledgment *)get_shared_memory(shmid_ack_list, 0);

	
    for (int child = 0; child < 5; ++child) {
        pid_t pid = fork();
        // check error for fork
        if (pid == -1) printf("child %d not created!", child);
        // check if running process is child or parent
        else if (pid == 0) {
            // code executed only by the child
        }
        exit(0);
    }
    return 0;
}
