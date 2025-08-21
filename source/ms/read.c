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