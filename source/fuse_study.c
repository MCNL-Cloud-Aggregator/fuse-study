#include "../custom_include/custom.h"
#include "../custom_include/bound.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/stat.h>
#include <sys/stat.h>

#define SERV_PORT 9000
#define SERV_IP "203.252.99.216"


void error_handling(char*);
void fuse_study_unlink(fuse_req_t req, fuse_ino_t parent, const char *name);
int fuse_study_create(const char *path, mode_t mode, struct fuse_file_info *fi);
void fuse_study_lookup(fuse_req_t req, fuse_ino_t parent, const char *name);
void fuse_study_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,struct fuse_file_info *fi);
void fuse_study_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode);
void fuse_study_rmdir(fuse_req_t req, fuse_ino_t parent, const char *name);
void fuse_study_read (fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi);

static const struct fuse_lowlevel_ops fs_oper = {
	//.init           = fuse_study_init,
	.unlink		= fuse_study_unlink,
    //.create = fuse_study_create,
	.lookup = fuse_study_lookup,
    .readdir = fuse_study_readdir,
    .mkdir = fuse_study_mkdir,
    .rmdir = fuse_study_rmdir,
};

int serv_sd;
char* _path;

int main(int argc, char *argv[]) {
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    struct fuse_cmdline_opts opts;
    struct fuse_session *se;
    //struct fuse_loop_config config;
    int ret = -1;
    struct sockaddr_in serv_ad;
    serv_sd = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_ad, 0, sizeof(serv_ad));
    serv_ad.sin_family = AF_INET;
    serv_ad.sin_addr.s_addr = inet_addr(SERV_IP);
    serv_ad.sin_port = htons(SERV_PORT);
    
    if(connect(serv_sd, (struct sockaddr*)&serv_ad, sizeof(serv_ad)) == -1){
        error_handling("connect error\n");
    }

    fputs("connect successed\n", stdout);

    if (fuse_parse_cmdline(&args, &opts) != 0) {
        fuse_opt_free_args(&args);
        return 1;
    }

    se = fuse_session_new(&args, &fs_oper, sizeof(fs_oper), NULL);
    if (!se) {
        fprintf(stderr, "fuse_session_new failed\n");
        goto err_out;
    }

    if (fuse_session_mount(se, opts.mountpoint) != 0) {
        fprintf(stderr, "fuse_session_mount failed\n");
        goto err_destroy;
    }

    if (!opts.foreground) {
        if (fuse_daemonize(0) == -1) {
            perror("fuse_daemonize");
            goto err_unmount;
        }
    }

    // 이벤트 루프 실행
    ret = fuse_session_loop_mt(se, 0);  // clone_fd = 0
err_unmount:
    fuse_session_unmount(se);   // 마운트 해제
err_destroy:
    fuse_session_destroy(se);   // 세션 파괴
err_out:
    free(opts.mountpoint);
    fuse_opt_free_args(&args);
    return ret ? 1 : 0;
}

void error_handling(char* message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}


void fuse_study_read (fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi) {
	
	struct pkt send_buf, read_buf;
	unsigned short opcode = READ;
	char* data = NULL;
	
	bound_send(serv_sd, &send_buf, &opcode, sizeof(opcode));
	bound_send(serv_sd, &send_buf, _path, strlen(_path)+1);
	
	size_t read_byte = 0;
	do {
		bound_read(serv_sd, &read_buf);
		if (data == NULL) {
			data = malloc(read_buf.total_size);
		}
		memcpy(&data[read_byte], read_buf.buf, read_buf.size);
		
		read_byte += read_buf.size;
		if (read_buf.end == 1) {
			break;
		}
	} while(true);
    
    printf("data : %s\n", data);
    
    fuse_reply_buf(req, data, read_byte);
}

//void fuse_study_write (fuse_req_t req, fuse_ino_t ino, const char *buf, size_t size, off_t off, struct fuse_file_info *fi) {
	
//	struct pkt send_buf, read_buf;
//	unsigned short opcode = READ;
//	char* data;
	
//	bound_send(serv_sd, &send_buf, &opcode, sizeof(opcode));
//	bound_send(serv_sd, &send_buf, _path, strlen(_path));
	
//	bound_send(serv_sd, &send_buf, buf, strlen(buf));
	
//}

void fuse_study_unlink(fuse_req_t req, fuse_ino_t parent, const char *path) {
    printf("unlink executed: parent=%lu, name=%s\n", parent, path);

    unsigned short opcode = UNLINK;
    struct pkt send_buf;
    char* data;
    data = (char*)malloc(sizeof(char) * (strlen(path) + 1));
    strcpy(data, path);
    data[strlen(path)] = '\0';
    bound_send(serv_sd, &send_buf, &opcode, sizeof(unsigned short));
    bound_send(serv_sd, &send_buf, data, strlen(path));

    // 서버 응답 대신 일단 성공으로 처리
    fuse_reply_err(req, 0);

    // 서버에서 에러코드 주면 그대로 fuse_reply_err(req, errno) 하면 됨
}


int fuse_study_create(const char *path, mode_t mode, struct fuse_file_info *fi){

    (void) fi;

    //여기에서는 opcode와 path를 전달
    struct pkt send_buf;
    //struct pkt recv_buf;
    //status stat;
    unsigned short opcode = CREATE;

    bound_send(serv_sd, &send_buf, &opcode, sizeof(unsigned short));
    bound_send(serv_sd, &send_buf, "/path", strlen("/path"));

    /*do{
        bound_read(serv_sd, &recv_buf);
        memcpy(&stat, recv_buf.buf, sizeof(status));

        switch (stat.status)
        {
        case 0 : //임시 error status
            //show help(오류로 인한 클라이언트에서 기능 관련 내용을 출력)
            break;
        
        default:
            break;
        }
        
    }while(stat.end != 1);*/

    /*
    return status code를 받고, 해당 code에 따라서 상태 메시지 출력 or 출력 x
    */
   return 0;
}

void fuse_study_lookup(fuse_req_t req, fuse_ino_t parent, const char *path) {
    printf("lookup executed: parent=%lu, name=%s\n", parent, path);

    if(_path != NULL){
        free(_path);
    }

    _path = (char*)malloc(sizeof(char) * (strlen(path) + 1));
    strcpy(_path, path);
    _path[strlen(path)] = '\0';
    
    unsigned short opcode = LOOKUP;
    struct pkt send_buf;
    struct pkt recv_buf;
    int flag;
    
    bound_send(serv_sd, &send_buf, &opcode, sizeof(unsigned short));
    bound_send(serv_sd, &send_buf, _path, strlen(_path));

    bound_read(serv_sd, &recv_buf);
    memcpy(&flag, recv_buf.buf, sizeof(int));

    struct fuse_entry_param e;
    memset(&e, 0, sizeof(e));


    if(flag == 1){
            // 예시: 그냥 inode=1인 더미 regular file 응답
        e.ino = 100;  // FUSE_ROOT_ID(1)와 겹치지 않는 값
        e.attr.st_mode = 0100000 | 0644;
        e.attr.st_nlink = 1;
        e.attr.st_uid = getuid();
        e.attr.st_gid = getgid();
        e.attr.st_size = 0;
        e.attr_timeout = 1.0;
        e.entry_timeout = 1.0;
        fuse_reply_entry(req, &e);
    }

    else{
        fuse_reply_err(req, ENOENT);
    }
    

}

void fuse_study_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                       struct fuse_file_info *fi) {
    (void) fi;
    int opcode = 2;
    struct pkt *pkt_data = calloc(1, sizeof(struct pkt));
    if (pkt_data == NULL) {
        fuse_reply_err(req, ENOMEM);
        return;
    }
    
    bound_send(serv_sd, pkt_data, &opcode, sizeof(int));
    bound_send(serv_sd, pkt_data, _path, strlen(_path) + 1);
    
    char *buffer = malloc(size);
    if (!buffer) {
        free(pkt_data);
        fuse_reply_err(req, ENOMEM);
        return;
    }
    
    size_t bufsize = 0;
    int current_offset = 0;
    
    while (1) {
        int flag;
        bound_read(serv_sd, pkt_data);
        memcpy(&flag, pkt_data->buf, sizeof(int));  
        
        if (flag == 0) break;
        
        bound_read(serv_sd, pkt_data);
        
        struct pkt *st_pkt = calloc(1, sizeof(struct pkt));
        bound_read(serv_sd, st_pkt);
        
        struct stat st;
        memcpy(&st, st_pkt->buf, sizeof(struct stat));
        
        if (current_offset >= off) {  
            size_t entry_size = fuse_add_direntry(req, buffer + bufsize, 
                                                 size - bufsize,
                                                 pkt_data->buf, &st, 
                                                 current_offset + 1);
            if (entry_size > size - bufsize) {
                free(st_pkt);
                break;  
            }
            bufsize += entry_size;
        }
        
        current_offset++;
        free(st_pkt);
    }
    
    free(pkt_data);
    fuse_reply_buf(req, buffer, bufsize);
    free(buffer);
}

void fuse_study_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode)
{
    int opcode = MKDIR;
    struct pkt * pkt_data = calloc(1,sizeof(struct pkt));
    if(pkt_data == NULL){
        fuse_reply_err(req, ENOMEM);
        return;
    }
    bound_send(serv_sd,pkt_data,&opcode,sizeof(int));
    bound_send(serv_sd,pkt_data,_path,strlen(_path)+1);
	write(serv_sd,&mode,sizeof(mode_t));
    free(pkt_data);

    bound_read(serv_sd, pkt_data);
    int result;
    memcpy(&result, pkt_data->buf, sizeof(int));
    
    if (result == 0) {
        struct fuse_entry_param entry;
        memset(&entry, 0, sizeof(entry));
        entry.ino = generate_new_inode();  
        entry.attr_timeout = 1.0;
        entry.entry_timeout = 1.0;
        fuse_reply_entry(req, &entry);
    } else {
        fuse_reply_err(req, -result); 
    }
    
    free(pkt_data);
}

void fuse_study_rmdir(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    int opcode = 9;
	struct pkt * pkt_data = calloc(1,sizeof(struct pkt));
    if (pkt_data == NULL) {
        fuse_reply_err(req, ENOMEM);
        return;
    }
    bound_send(serv_sd,pkt_data,&opcode,sizeof(int));
    bound_send(serv_sd,pkt_data,_path,strlen(_path)+1);

    bound_read(serv_sd, pkt_data);
    int result;
    memcpy(&result, pkt_data->buf, sizeof(int));
    if (result == 0) {
        fuse_reply_err(req, 0);
    } else {
        fuse_reply_err(req, -result);
    }
    free(pkt_data);
}