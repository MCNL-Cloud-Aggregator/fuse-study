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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>

/*
 * Command line options
 *
 * We can't set default values for the char* fields here because
 * fuse_opt_parse would attempt to free() them when the user specifies
 * different values on the command line.
 */
static struct options {
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
};