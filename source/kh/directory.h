#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fuse_lowlevel.h>

static int fuse_study_readdir_for_server();
static void fuse_study_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
			 struct fuse_file_info *fi);
static int fuse_study_mkdir_for_server();
static void fuse_study_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode);
static int fuse_study_rmdir_for_server();
static void fuse_study_rmdir(fuse_req_t req, fuse_ino_t parent, const char *name);