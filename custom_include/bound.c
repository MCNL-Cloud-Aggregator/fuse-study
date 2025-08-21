#include "bound.h"

/*int bound_read(const int sd, struct pkt* buf){
    int total_read = 0;
    int read_cnt = 0;
    int read_sz = (int)sizeof(struct pkt);
    void* ptr = buf;

    while(read_sz > 0){
        read_cnt = read(sd, ptr + total_read, read_sz);
        if(read_cnt <= 0){ //error
            return read_cnt;
        }
        
        total_read += read_cnt;
        read_sz -= read_cnt;
    }

    return buf->size;
}*/

int bound_read(const int sd, struct pkt* buf){
    int total_read = 0;
    int read_cnt = 0;
    int read_sz = sizeof(struct pkt);
    char* ptr = (char*)buf;  // **struct* -> char*로 캐스팅**

    while(read_sz > 0){
        read_cnt = read(sd, ptr + total_read, read_sz); // 이제 바이트 단위
        if(read_cnt <= 0){
            return read_cnt;
        }

        total_read += read_cnt;
        read_sz -= read_cnt;
    }

    return buf->size;
}

int bound_send(const int sd, struct pkt* buf, void* data, size_t data_size){
    size_t remain_len = data_size;
    size_t loc = 0;
    int write_cnt;

    while(remain_len > 0){
        int send_size = (remain_len > BUF_SIZE) ? BUF_SIZE : remain_len;
        memcpy(buf->buf, data + loc, send_size);
        
        buf->size = send_size;
        buf->end = (remain_len <= BUF_SIZE);
        buf->total_size = data_size;

        write_cnt = write(sd, buf, sizeof(struct pkt));
        if(write_cnt == -1){
            return write_cnt;
        }

        remain_len -= send_size;
        loc += send_size;
    }

    return data_size;
}
