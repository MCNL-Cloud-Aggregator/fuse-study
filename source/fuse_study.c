#include "../custom_include/custom.h"
#include "../custom_include/bound.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/stat.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <time.h>          
#include <fuse_lowlevel.h>
#define SERV_PORT 9000
#define SERV_IP "203.252.99.216"


void error_handling(char*);
void fuse_study_unlink(fuse_req_t req, fuse_ino_t parent, const char *name);
void fuse_study_create(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode, struct fuse_file_info *fi);
void fuse_study_lookup(fuse_req_t req, fuse_ino_t parent, const char *name);
void fuse_study_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,struct fuse_file_info *fi);
void fuse_study_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode);
void fuse_study_rmdir(fuse_req_t req, fuse_ino_t parent, const char *name);
void fuse_study_read (fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi);
void fuse_study_write (fuse_req_t req, fuse_ino_t ino, const char *buf, size_t size, off_t off, struct fuse_file_info *fi);
void fuse_study_setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr, int to_set, struct fuse_file_info *fi);
void fuse_study_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
void fuse_study_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
void fuse_study_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);


static const struct fuse_lowlevel_ops fs_oper = {
	//.init           = fuse_study_init,
	.unlink		= fuse_study_unlink,
    .create = fuse_study_create,
    .setattr = fuse_study_setattr,
	.lookup = fuse_study_lookup,
    .readdir = fuse_study_readdir,
    .mkdir = fuse_study_mkdir,
    .rmdir = fuse_study_rmdir,
    .read = fuse_study_read,
    .write = fuse_study_write,
    .open = fuse_study_open,
    .getattr = fuse_study_getattr,
    .opendir = fuse_study_opendir,
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
	bound_send(serv_sd, &send_buf, _path, strlen(_path));
	
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

void fuse_study_write (fuse_req_t req, fuse_ino_t ino, const char *buf, size_t size, off_t off, struct fuse_file_info *fi) {
	
	struct pkt send_buf;
	unsigned short opcode = READ;
    char* buf_cpy = malloc(strlen(buf));
    memcpy(buf_cpy, buf, strlen(buf));
	
	bound_send(serv_sd, &send_buf, &opcode, sizeof(opcode));
	bound_send(serv_sd, &send_buf, _path, strlen(_path));
	
	bound_send(serv_sd, &send_buf, buf_cpy, strlen(buf));
	
}

void fuse_study_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    (void)ino;

    if (_path == NULL) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    // 1) opcode + path(NUL 포함) + flags 전송
    unsigned short opcode =
    #ifdef OPEN
        OPEN;          // 프로젝트에서 OPEN이 0x03으로 정의되어 있으면 사용
    #else
        0x03;          // 서버 스위치와 동일
    #endif

    struct pkt send_pkt, recv_pkt;
    int flags = fi ? fi->flags : O_RDONLY;

    bound_send(serv_sd, &send_pkt, &opcode, sizeof(opcode));
    bound_send(serv_sd, &send_pkt, _path, strlen(_path) + 1);  // ⬅ NUL 포함
    bound_send(serv_sd, &send_pkt, &flags, sizeof(flags));

    // 2) 서버 결과 수신 (0이면 OK, 음수면 -errno)
    if (bound_read(serv_sd, &recv_pkt) <= 0) {
        fuse_reply_err(req, EIO);
        return;
    }
    int result = 0;
    memcpy(&result, recv_pkt.buf, sizeof(int));
    if (result != 0) {
        fuse_reply_err(req, -result);  // -(-errno) → errno
        return;
    }

    // 3) 캐시/직접 I/O 정책 (현재 구현은 path 기반 I/O라 direct_io 켜도 무방)
    // fi->direct_io = 1;    // 페이지 캐시 사용 안 함 (원하면 주석 해제)
    // fi->keep_cache = 0;

    // 4) 성공 응답
    fuse_reply_open(req, fi);
}

void fuse_study_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    (void)fi;

    // 1) 루트 디렉토리 직접 응답 (서버 왕복 불필요)
    if (ino == FUSE_ROOT_ID) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino   = FUSE_ROOT_ID;
        st.st_mode  = S_IFDIR | 0755;
        st.st_nlink = 2;
        st.st_uid   = getuid();
        st.st_gid   = getgid();
        st.st_atime = time(NULL);
        st.st_mtime = st.st_atime;
        st.st_ctime = st.st_atime;
        fuse_reply_attr(req, &st, 1.0 /* attr cache sec */);
        return;
    }

    // 2) 서버에 GETATTR 질의
    struct pkt send_pkt, recv_pkt;
    unsigned short opcode =
    #ifdef GETATTR
        GETATTR;      // 프로젝트에 GETATTR가 정의돼 있다면 이걸 사용
    #else
        0x01;         // 위 서버 스위치와 동일하게 0x01 사용
    #endif

    // 현재 코드 구조상 _path에 lookup 이름만 들어가 있음
    // (권장: 전역 대신 각 요청마다 풀경로 구성)
    if (_path == NULL) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    // 문자열은 널 포함해서 보내는게 안전
    bound_send(serv_sd, &send_pkt, &opcode, sizeof(opcode));
    bound_send(serv_sd, &send_pkt, _path, strlen(_path) + 1);

    // 2-1) 먼저 결과코드(int) 수신 (0이면 OK, 음수면 -errno)
    if (bound_read(serv_sd, &recv_pkt) <= 0) {
        fuse_reply_err(req, EIO);
        return;
    }
    int result = 0;
    memcpy(&result, recv_pkt.buf, sizeof(int));
    if (result != 0) {
        // 서버가 -errno 보냄 → 양수 errno로 바꿔서 반환
        fuse_reply_err(req, -result);
        return;
    }

    // 2-2) OK면 stat 구조체 수신
    if (bound_read(serv_sd, &recv_pkt) <= 0) {
        fuse_reply_err(req, EIO);
        return;
    }
    struct stat st;
    if (recv_pkt.size < sizeof(struct stat)) {
        fuse_reply_err(req, EIO);
        return;
    }
    memcpy(&st, recv_pkt.buf, sizeof(struct stat));

    // 커널에 속성 전달
    // 캐시 타임아웃은 필요에 맞게 조정(여기선 1초)
    fuse_reply_attr(req, &st, 1.0);
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


void fuse_study_create(fuse_req_t req, fuse_ino_t parent, const char *path, mode_t mode, struct fuse_file_info *fi){

    //여기에서는 opcode와 path를 전달
    unsigned short opcode = CREATE;
    struct pkt send_buf;
    char* data;
    data = (char*)malloc(sizeof(char) * (strlen(path) + 1));
    strcpy(data, path);
    data[strlen(path)] = '\0';

    bound_send(serv_sd, &send_buf, &opcode, sizeof(unsigned short));
    bound_send(serv_sd, &send_buf, data, strlen(path));

    struct fuse_entry_param e;
    memset(&e, 0, sizeof(e));

    e.ino = 101;                        // 새로 생성된 파일 inode
    e.attr.st_mode = 0100000 | mode;    // regular file + mode
    e.attr.st_nlink = 1;
    e.attr.st_uid = getuid();
    e.attr.st_gid = getgid();
    e.attr.st_size = 0;
    e.attr_timeout = 1.0;
    e.entry_timeout = 1.0;

    // fi->fh 같은 경우 필요에 따라 파일 핸들 세팅
    fi->fh = e.ino;  // 예시
    fuse_reply_create(req, &e, fi);
}

void fuse_study_setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr, int to_set, struct fuse_file_info *fi) {
    printf("setattr called on ino=%lu\n", ino);

    // 여기서 서버 통신은 생략하고, 요청 자체를 항상 성공했다고 응답
    // 단, 커널에 돌려줄 stat 구조체는 최소한 채워줘야 함

    struct stat st;
    memset(&st, 0, sizeof(st));
    st.st_ino = ino;
    st.st_mode = 0100000 | 0644;  // 임시: 일반 파일
    st.st_nlink = 1;
    st.st_uid = getuid();
    st.st_gid = getgid();
    st.st_size = 0;
    st.st_atime = time(NULL);
    st.st_mtime = time(NULL);
    st.st_ctime = time(NULL);

    fuse_reply_attr(req, &st, 1.0);
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
    int opcode = READDIR;
    struct pkt *pkt_data = calloc(1, sizeof(struct pkt));
    if (pkt_data == NULL) {
        fuse_reply_err(req, ENOMEM);
        return;
    }
    
    bound_send(serv_sd, pkt_data, &opcode, sizeof(int));
    bound_send(serv_sd, pkt_data, _path, strlen(_path)+1);
    
    char *buffer = malloc(size);
    if (!buffer) {
        free(pkt_data);
        fuse_reply_err(req, ENOMEM);
        return;
    }
    
    size_t bufsize = 0;
    off_t current_offset = 0;

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
            size_t entry_size = fuse_add_direntry(req, buffer + bufsize, size - bufsize, pkt_data->buf, &st, current_offset + 1);
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
    bound_send(serv_sd,pkt_data,_path,strlen(_path));
	write(serv_sd,&mode,sizeof(mode_t));

    bound_read(serv_sd, pkt_data);
    int result;
    memcpy(&result, pkt_data->buf, sizeof(int));
    
    if (result == 0) {
        bound_read(serv_sd, pkt_data);
        struct stat st;
        memcpy(&st, pkt_data->buf, sizeof(struct stat));
        
        struct fuse_entry_param entry;
        memset(&entry, 0, sizeof(entry));
        entry.ino = st.st_ino;  
        entry.attr = st;       
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
    int opcode = RMDIR;
	struct pkt * pkt_data = calloc(1,sizeof(struct pkt));
    if (pkt_data == NULL) {
        fuse_reply_err(req, ENOMEM);
        return;
    }
    bound_send(serv_sd,pkt_data,&opcode,sizeof(int));
    bound_send(serv_sd,pkt_data,_path,strlen(_path));

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

void fuse_study_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
    fuse_reply_open(req, fi);
}