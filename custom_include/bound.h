#pragma once
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE 1024

struct pkt{
    int end;
    size_t size;
    size_t total_size;
    char buf[BUF_SIZE];
};

int bound_read(const int sd, struct pkt* buf);
int bound_send(const int sd, struct pkt* buf, void* data, size_t data_size);
