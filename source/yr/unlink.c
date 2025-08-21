#include "../../custom_include/bound.h"
#include "../../custom_include/custom.h"

int fuse_study_unlink(const char *path){
    //여기에서는 opcode와 path를 전달
    struct pkt send_buf;
    struct pkt recv_buf;
    status stat;
    //int serv_sd;
    unsigned short opcode = UNLINK;

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

/*
server는 opcode를 검사, 0x00~0x09까지 없으면 그냥 default 혹은 help message를 띄움
*/