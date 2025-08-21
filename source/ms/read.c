#include <../custom_include/custom.h>
#include <../custom_include/bound.h>

void fuse_study_read (fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi) {
	
	struct pkt send_buf;
	unsigned short opcode = READ;
	
	bound_send(serv_sd, &send_buf, &opcode, sizeof(opcode));
	bound_send(serv_sd, &send_buf, _path, strlen(_path));
	
	
}