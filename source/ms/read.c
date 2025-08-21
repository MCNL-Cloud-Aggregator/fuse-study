#include <../custom_include/custom.h>
#include <../custom_include/bound.h>

static int fuse_study_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	
	size_t len;
	(void) fi;
	if(strcmp(path+1, options.filename) != 0)
		return -ENOENT;

	len = strlen(options.contents);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, options.contents + offset, size);
	} else
		size = 0;

	return size;
}