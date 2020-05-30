/// @file fifo.h
/// @brief Contiene la definizioni di variabili e
///         funzioni specifiche per la gestione delle FIFO.

#pragma once
#include <sys/types.h>
#include <unistd.h>
#ifndef _FIFO_HH
#define _FIFO_HH

void mk_fifo(const char *pathname, mode_t mode);
int open_fifo(const char *pathname, int flags);
int read_fifo(int fifods, void *buf, size_t count);
void write_fifo(int fifo_ds, void *buf, size_t count);
void close_fifo(int fifo_ds);
void unlink_fifo(const char *pathname);

#endif
