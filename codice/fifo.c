/// @file fifo.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione delle FIFO.

#include "fifo.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "defines.h"
#include "err_exit.h"

void mk_fifo(const char *pathname, mode_t mode) {
    if (mkfifo(pathname, mode) == -1) ErrExit("mkfifo failed");
}
int open_fifo(const char *pathname, int flags) {
    int file_ds;
    file_ds = open(pathname, flags);
    if (file_ds == -1) ErrExit("open fifo failed");
    return file_ds;
}
int read_fifo(int fifods, void *buf, size_t count) {
    int bR = read(fifods, buf, count);
    // Checking the number of bytes from the FIFO
    // if (bR == -1) printf("Read fifo failed\n");
    // ErrExit("Read fifo failed");
    return bR;
}
void write_fifo(int fifo_ds, void *buf, size_t count) {
    // errExit al posto di printf?
    if (write(fifo_ds, buf, count) != count) ErrExit("write fifo failed\n");
    // printf("write fifo failed\n");
}
void close_fifo(int fifo_ds) {
    if (close(fifo_ds) != 0) ErrExit("close fifo failed");
}
void unlink_fifo(const char *pathname) {
    if (unlink(pathname) != 0) ErrExit("unlink fifo failed");
}
