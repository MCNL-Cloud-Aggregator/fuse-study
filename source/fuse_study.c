#include "../custom_include/custom.h"
#include "../custom_include/bound.h"
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERV_PORT 9000
#define SERV_IP "203.252.99.216"


void error_handling(char*);
int fuse_study_unlink(const char *path);

static const struct fuse_operations fs_oper = {
	.init           = fuse_study_init,
	.unlink		= fuse_study_unlink,
};

int serv_sd;

int main(int argc, char *argv[])
{
	int ret;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	/* Set defaults -- we have to use strdup so that
	   fuse_opt_parse can free the defaults if other
	   values are specified */

	/* Parse options */
	/*if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
		return 1;*/

	/* When --help is specified, first print our own file-system
	   specific help text, then signal fuse_main to show
	   additional help (by adding `--help` to the options again)
	   without usage: line (by setting argv[0] to the empty
	   string) */
	/*if (options.show_help) {
		show_help(argv[0]);
		assert(fuse_opt_add_arg(&args, "--help") == 0);
		args.argv[0][0] = '\0';
	}*/
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

	ret = fuse_main(args.argc, args.argv, &fs_oper, NULL);
	fuse_opt_free_args(&args);
	return ret;
}


void error_handling(char* message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

int fuse_study_unlink(const char *path){
    //여기에서는 opcode와 path를 전달
    struct pkt send_buf;
    struct pkt recv_buf;
    status stat;
    //int serv_sd;
    unsigned short opcode = UNLINK;

    printf("jkawhsdjkhfhjkashdfkjasdf\n");
    bound_send(serv_sd, &send_buf, &opcode, sizeof(unsigned short));
    bound_send(serv_sd, &send_buf, "/path", strlen("/path"));

    do{
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
        
    }while(stat.end != 1);

    /*
    return status code를 받고, 해당 code에 따라서 상태 메시지 출력 or 출력 x
    */

    return 0;
}

int fuse_study_create(const char *path, mode_t mode, struct fuse_file_info *fi){

    (void) fi;

    //여기에서는 opcode와 path를 전달
    struct pkt send_buf;
    struct pkt recv_buf;
    status stat;
    unsigned short opcode = CREATE;

    bound_send(serv_sd, &send_buf, opcode, sizeof(unsigned short));
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
}