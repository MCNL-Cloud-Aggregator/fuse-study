#include <../custom_include/custom.h>
#include <../custom_include/bound.h>

void fuse_study_read (fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi) {
	
	struct pkt send_buf, read_buf;
	unsigned short opcode = READ;
	char* data;
	
	bound_send(serv_sd, &send_buf, &opcode, sizeof(opcode));
	bound_send(serv_sd, &send_buf, _path, strlen(_path));
	
	size_t read_byte = 0;
	do {
		bound_read(serv_sd, read_buf);
		if (data == NULL) {
			data = malloc(read_buf.total_size);
		}
		memset(&data[read_byte], read_buf.buf, read_buf.size);
		
		read_byte += read_buf.size;
		if (read_buf.end == 1) {
			break;
		}
	}
	
}

int fuse_study_read(int client_sock, char* path) {
	
	struct pkt read_buf, send_buf;
	char* data = NULL;
	
	FILE* read_fd = fopen(path, "rb");
	fseek(read_fd, 0, SEEK_END);
    long file_size = ftell(read_fd);
    rewind(read_fd);
	
	data = (char*)malloc(file_size);
	
	fread(data, 1, file_size, read_fd);
	
	bound_send(client_sock, &send_buf, data, file_size);
	
	return 0;
}