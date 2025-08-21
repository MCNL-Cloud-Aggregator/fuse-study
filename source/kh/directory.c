#include "directory.h"
#include "../custom_include/bound.h"

static int fill_dir_plus = 0;

int sock;
char temp_buf[BUFSIZ];


static int fuse_study_readdir_for_server(){
    DIR *dp;
    struct dirent *de;
    char real_path[BUF_SIZE];
    struct pkt * pkt_data = calloc(1,sizeof(struct pkt));
    int flag = 1;
    if(pkt_data == NULL)
        error_handling("calloc() error");
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


static void fuse_study_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
			 struct fuse_file_info *fi)
{
	(void) fi;
    int opcode = 2;
    struct pkt * pkt_data = calloc(1,sizeof(struct pkt)); 
    if(pkt_data == NULL)
        error_handling("calloc() error");
    bound_send(sock,pkt_data,&opcode,sizeof(int));
    bound_send(sock,pkt_data,path,strlen(path)+1);
    int flag = 1;
    while (1)
    {
        bound_read(sock,pkt_data);
        //read(sock,&flag,sizeof(int));
        if(flag == 0) break;
        memset(pkt_data,0,sizeof(struct pkt));
        bound_read(sock,pkt_data);
        //struct stat st;
        struct pkt * st = calloc(1,sizeof(struct pkt)); 
        //memset(&st,0,sizeof(st));
        bound_read(sock,st);
        //read(sock,&st,sizeof(st));
        if(filler(buf,pkt_data->buf,st->buf,0))
            error_handling("filler() error");
         free(st);
    }
    free(pkt_data);
	
}

static int fuse_study_mkdir_for_server()
{
	int res;
    struct pkt * pkt_data = calloc(1,sizeof(struct pkt));
    if(pkt_data == NULL)
        error_handling("calloc() error");
    bound_read(sock,pkt_data);
    mode_t mode;
    read(sock,&mode,sizeof(mode_t));
	res = mkdir(pkt_data->buf, mode);
    write(sock,&res,sizeof(int));
    free(pkt_data);
	return 0;
}

static void fuse_study_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode)
{
    int opcode = 6;
    struct pkt * pkt_data = calloc(1,sizeof(struct pkt));
    if(pkt_data == NULL)
        error_handling("calloc() error");
    bound_send(sock,pkt_data,&opcode,sizeof(int));
    bound_send(sock,pkt_data,path,strlen(path)+1);
	write(sock,&mode,sizeof(mode_t));
    int res;
    read(sock,&res,sizeof(int));
    if (res == -1)
		return -errno;
    free(pkt_data);
}

static int fuse_study_rmdir_for_server()
{
	int res;
    struct pkt * pkt_data = calloc(1,sizeof(struct pkt));
    if(pkt_data == NULL)
        error_handling("calloc() error");
    bound_read(sock,pkt_data);
	res = rmdir(pkt_data->buf);
	write(sock,&res,sizeof(int));
    free(pkt_data);
	return 0;
}

static void fuse_study_rmdir(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    int opcode = 9;
	struct pkt * pkt_data = calloc(1,sizeof(struct pkt));
    if(pkt_data == NULL)
        error_handling("calloc() error");
    bound_send(sock,pkt_data,&opcode,sizeof(int));
    bound_send(sock,pkt_data,path,strlen(path)+1);
    int res;
    read(sock,&res,sizeof(int));
    if (res == -1)
		return -errno;
    free(pkt_data);
}