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


static const struct fuse_lowlevel_ops fs_oper = {
	//.init           = fuse_study_init,
	.unlink		= fuse_study_unlink,
    //.create = fuse_study_create,
	.lookup = fuse_study_lookup
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

    struct fuse_entry_param e;
    memset(&e, 0, sizeof(e));

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