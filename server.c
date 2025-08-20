#include <stdio.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/epoll.h>

#define EPOLL_SIZE 96
#define BUFSIZE 512

struct thread_arg {
	unsigned short opcode;
	int client_sock;
};

void* thread_handler(void* arg) {
	int client_sock = ((struct thread_arg*) arg)->client_sock;
	unsigned short opcode = ((struct thread_arg*) arg)->opcode;
	
	switch(opcode) {
		case 0x00 : break;
		case 0x01 : // fuse_study_getattr();
		case 0x02 : // fuse_study_readdir();
		case 0x03 : // fuse_study_open();
		case 0x04 : // fuse_study_read();
		case 0x05 : // fuse_study_create();
		case 0x06 : // fuse_study_mkdir();
		case 0x07 : // fuse_study_write();
		case 0x08 : // fuse_study_unlink();
		case 0x09 : // fuse_study_rmdir();
		default : break;
	}
	
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
	server_addr.sin_addr.s_addr = inet_addr("192.168.64.16");
	server_addr.sin_port = htons(24682);
	
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
	
	// epoll 생성
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
				
				// epoll register
				event.events = EPOLLIN;
				event.data.fd = client_sock;
				epoll_ctl(epfd, EPOLL_CTL_ADD, client_sock, &event);
				
			}
			else {
				struct thread_arg thread_arg;
				
				// client_sock 저장
				thread_arg.client_sock = epoll_events[i].data.fd;
				
				// opcode 수신
				read(epoll_events[i].data.fd, &thread_arg.opcode, sizeof(thread_arg.opcode));
				
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
