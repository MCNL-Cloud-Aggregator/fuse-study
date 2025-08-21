#pragma once
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE 1024

struct pkt{
    int end;
    size_t size;
    char buf[BUF_SIZE];
};

void error_handling(const char*);
int bound_read(const int sd, struct pkt* buf);
int bound_send(const int sd, struct pkt* buf, void* data, size_t data_size);