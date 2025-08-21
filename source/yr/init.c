#include "../../custom_include/custom.h"

void *fuse_study_init(struct fuse_conn_info *conn, struct fuse_config *cfg){
    (void) conn;
	cfg->use_ino = 1;
    cfg->parallel_direct_writes = 1;
    if (!cfg->auto_cache) {
		cfg->entry_timeout = 0;
		cfg->attr_timeout = 0;
		cfg->negative_timeout = 0;
	}

    return NULL;
}