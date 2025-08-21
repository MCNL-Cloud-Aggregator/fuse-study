/*
init		= hello_init,			    0
.getattr	= hello_getattr,	종현	1
.readdir	= hello_readdir,	강현	2
.open		= hello_open,		종현	3
.read		= hello_read,		민석	4
.create		영리						5
.mkdir		강현						6
.write		민석						7
.unlink		영리						8
.rmdir		강현						9

select는 유저 단위
thread는 유저 한명당이 아닌 요청 한번당

fuse daemon 파일 , connection 파일 헤더파일 분리 , TCP Boundary read/write
header 파일 분리 

마운트시 서버의 어떤 디렉토리와 연동할지 옵션을 주든지 해서
*/

#define FUSE_USE_VERSION 31


#include <fuse.h>
#include <fuse_lowlevel.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>


//opcode 매크로 등록
#define LOOKUP	0x00
#define READDIR 0x02
#define READ	0x04
#define CREATE	0x05
#define MKDIR	0x06
#define WRITE	0x07
#define UNLINK	0x08
#define RMDIR	0x09
/*
server_socket descriptor

/mnt/hello/hello.c
./hello/hello.c
*/

typedef struct status{
	uint8_t status;
	uint8_t end;
	size_t size; //body length
	char body[512];
}status;


/*
status

*/

/*
 * Command line options
 *
 * We can't set default values for the char* fields here because
 * fuse_opt_parse would attempt to free() them when the user specifies
 * different values on the command line.
 */
/*static struct options {
	const char *filename;
	const char *contents;
	int show_help;
} options;


#define OPTION(t, p)                           \
    { t, offsetof(struct options, p), 1 }
static const struct fuse_opt option_spec[] = {
	OPTION("--name=%s", filename),
	OPTION("--contents=%s", contents),
	OPTION("-h", show_help),
	OPTION("--help", show_help),
	FUSE_OPT_END
};*/

//int fuse_study_unlink(const char *path);
//int fuse_study_create(const char *path, mode_t mode, struct fuse_file_info *fi);
//void *fuse_study_init(struct fuse_conn_info *conn, struct fuse_config *cfg);