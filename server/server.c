#include <stdio.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/epoll.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "../custom_include/bound.h"

#define EPOLL_SIZE 96
#define DEFAULT_PATH "/"
#define PORT 9000

/*
gcc -Wall fuse_study.c ./yr/init.c ../custom_include/bound.c `pkg-config fuse3 --cflags --libs` -o fs_cli
*/

int fuse_study_readdir(int sock);
int fuse_study_rmdir(int sock);
int fuse_study_mkdir(int sock);

struct thread_arg {
	unsigned short opcode;
	char* path;
	int client_sock;
	int epfd;
	struct epoll_event event;
};

void* thread_handler(void* arg) {
	int client_sock = ((struct thread_arg*) arg)->client_sock;
	unsigned short opcode = ((struct thread_arg*) arg)->opcode;
	int epfd = ((struct thread_arg*) arg)->epfd;
	struct epoll_event event = ((struct thread_arg*) arg)->event;
	char* path = ((struct thread_arg*) arg)->path;

	struct pkt send_buf;
	
	switch(opcode) {
		case 0x00 : int flag = open(path); bound_send(client_sock, &send_buf, &flag, sizeof(int)); break;
		case 0x01 : // fuse_study_getattr();
		case 0x02 : fuse_study_readdir(client_sock);
		case 0x03 : // fuse_study_open();
		case 0x04 : // fuse_study_read();
		case 0x05 : printf("%s", path); break;// fuse_study_create();
		case 0x06 : fuse_study_mkdir(client_sock);
		case 0x07 : // fuse_study_write();
		case 0x08 : printf("%s", path); fflush(stdout); break;// fuse_study_unlink();
		case 0x09 : fuse_study_rmdir(client_sock);
		default : break;
	}
	
	// epoll list에 client socket 다시 등록
	epoll_ctl(epfd, EPOLL_CTL_ADD, client_sock, &event);
	
	// path 메모리 반환
	free(path);
	
	//todo: 다시 epoll list에 클라이언트 소켓을 추가
	/*
		goto를 사용하든 조건문을 통해서 그냥 return 하든 client가 연결을 끊었을 때는 그냥 return, 그렇지않으면 epoll list에 client socket을 추가하고 return
	*/
	return NULL;
}

int main() {
	int server_sock, client_sock;
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_addr_size = sizeof(client_addr);
	
	int epfd, event_count;
	struct epoll_event *epoll_events;
	struct epoll_event event;
	
	// socket()
	server_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sock == -1) {
		printf("[Error] socket()\n");
		exit(-1);
	}
	
	// server_sock 세팅
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);
	
	// client_sock 세팅
	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = 0;
	client_addr.sin_port = 0;
	
	// bind()
	if (bind(server_sock, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1) {
		printf("[Error] bind()\n");
		exit(-1);
	}
	
	// listen()
	if (listen(server_sock, 10) == -1) {
		printf("[Error] listen()\n");
		exit(-1);
	}
	
	// epoll_list 생성
	epfd = epoll_create(EPOLL_SIZE);
	epoll_events = malloc(sizeof(struct epoll_event) * EPOLL_SIZE);
	
	// server_sock epoll 등록
	event.events = EPOLLIN;
	event.data.fd = server_sock;
	epoll_ctl(epfd, EPOLL_CTL_ADD, server_sock, &event);
	
	pthread_t thread_id;
	while (true) {
		event_count = epoll_wait(epfd, epoll_events, EPOLL_SIZE, -1);
		if (event_count == -1) {
			printf("[Error] epoll_wait\n");
			break;
		}
		
		for (int i = 0; i < event_count; i ++) {
			if (epoll_events[i].data.fd == server_sock) {
				
				// accept()
				client_sock = accept(server_sock, (struct sockaddr*) &client_addr, &client_addr_size);
				if (client_sock == -1) {
					printf("[Error] accept()\n");
					exit(-1);
				}
				printf("cliend %d connection created\n", client_sock);
				
				// epoll register
				event.events = EPOLLIN;
				event.data.fd = client_sock;
				epoll_ctl(epfd, EPOLL_CTL_ADD, client_sock, &event);
				
			}
			else {
				struct thread_arg thread_arg;
				struct pkt pkt;
				int read_byte;
				
				// opcode 수신 및 thread_arg에 opcode 저장
				read_byte = bound_read(epoll_events[i].data.fd, &pkt);
				
				// client가 connection을 종료했을 경우
				if (read_byte == 0) {
					printf("cliend %d connection closed\n", epoll_events[i].data.fd);
					close(client_sock);
					epoll_ctl(epfd, EPOLL_CTL_DEL, client_sock, &event);
					continue;
				}
				
				memcpy(&thread_arg.opcode, pkt.buf, pkt.size);
				
				// path 수신 및 thread_arg에 path 저장
				char* path = NULL;
				size_t received_byte = 0;
				do {
					bound_read(epoll_events[i].data.fd, &pkt);
					if (path == NULL) {
						path = malloc(pkt.total_size);
					}
					
					strncpy(&path[received_byte], pkt.buf, pkt.size);
					received_byte += pkt.size;
					if (pkt.end == 1) {
						break;
					}
				} while (true);
				thread_arg.path = path;
				
				// thread_arg에 client_sock, epfd, event 저장
				thread_arg.client_sock = epoll_events[i].data.fd;
				thread_arg.epfd = epfd;
				thread_arg.event = epoll_events[i];
				
				// thread 생성 전에 epoll list에서 sock 제거
				epoll_ctl(epfd, EPOLL_CTL_DEL, client_sock, &event);
				
				// pthread_create()
				if (pthread_create(&thread_id, NULL, thread_handler, (void*) &thread_arg) != 0) {
					printf("[Error] pthread_create()\n");
					exit(-1);
				}
				
				pthread_detach(thread_id);
			}
		}
	}
	
	close(server_sock);
	free(epoll_events);
	
	return 0;
}


int fuse_study_readdir(int sock){
    DIR *dp;
    struct dirent *de;
    char real_path[BUF_SIZE*2];
    struct pkt * pkt_data = calloc(1,sizeof(struct pkt));
    int flag = 1;
   if(pkt_data == NULL){
		perror("calloc() error\n");
		exit(1);
	}
    bound_read(sock,pkt_data);
	
    snprintf(real_path,sizeof(real_path),".%s",pkt_data->buf);
    dp = opendir(real_path);

	if (dp == NULL){
        int error = -errno;
        flag = 0;
        bound_send(sock,pkt_data,&flag,sizeof(int));
        //write(sock,&flag,sizeof(int));
        free(pkt_data);
		return -errno;
    }
    
	while ((de = readdir(dp)) != NULL) {
        bound_send(sock,pkt_data,&flag,sizeof(int));
        //write(sock,&flag,sizeof(int));
        memset(pkt_data,0,sizeof(struct pkt));
        bound_send(sock,pkt_data,de->d_name,strlen(de->d_name)+1);
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        bound_send(sock,pkt_data,&st,sizeof(st));
        //write(sock,&st,sizeof(st));
	}
    flag = 0;
    bound_send(sock,pkt_data,&flag,sizeof(int));
    //write(sock,&flag,sizeof(int));
	closedir(dp);
    free(pkt_data);
    return 0;
}

int fuse_study_mkdir(int sock)
{
	int res;
    struct pkt * pkt_data = calloc(1,sizeof(struct pkt));
    if(pkt_data == NULL){
		perror("calloc() error\n");
		exit(1);
	}
    bound_read(sock,pkt_data);
    mode_t mode;
    read(sock,&mode,sizeof(mode_t));
	res = mkdir(pkt_data->buf, mode);
    write(sock,&res,sizeof(int));
    free(pkt_data);
	return 0;
}

int fuse_study_rmdir(int sock)
{
	int res;
    struct pkt * pkt_data = calloc(1,sizeof(struct pkt));
    if(pkt_data == NULL){
		perror("calloc() error\n");
		exit(1);
	}
    bound_read(sock,pkt_data);
	res = rmdir(pkt_data->buf);
	write(sock,&res,sizeof(int));
    free(pkt_data);
	return 0;
}