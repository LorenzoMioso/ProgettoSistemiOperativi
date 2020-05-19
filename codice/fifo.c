/// @file fifo.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione delle FIFO.

#include "defines.h"
#include "fifo.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>


void mk_fifo(const char *pathname, mode_t mode){
	if (mkfifo(pathname, mode) == -1)
        errExit("mkfifo failed");
}
int open_fifo(const char *pathname, int flags){
	int file_ds;
	file_ds = open(pathname, flags);
	if(file_ds == -1)
		errExit("open fifo failed");
	return file_ds;
}
int read_fifo(int fifods, void *buf, size_t count, char *who){
	int bR = read(fifods, buf, count);

    // Checking the number of bytes from the FIFO
    if (bR == -1)
        printf("<%s> it looks like the FIFO is broken", who);
    if (bR != count || bR == 0)
        printf("<%s> it looks like I did not receive buf", who);
    return bR;
}
void write_fifo(int fifo_ds, void *buf, size_t count){
	//errExit al posto di printf?
	 if (write(fifo_ds, buf, count) != count)
        printf("write fifo failed");


}
void close_fifo(int fifo_ds){
   if (close(fifo_ds) != 0)
        errExit("close fifo failed");
}
void unlink_fifo(const char *pathname){
	if (unlink(pathname) != 0)
        errExit("unlink fifo failed");

}
