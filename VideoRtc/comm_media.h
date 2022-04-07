

#ifndef __MED_COMMAND_H__
#define __MED_COMMAND_H__

#include <stdio.h>
#include "types.h"
#include "files.h"
#include "transport_udp.h"
#include "video_rtc_api.h"

#define CONNECTED_SHAKE  1001

#define COMM_MAKR 0X3c3c3c3c
#define COMM_VERS 1

//commond head info ->8byte
typedef struct pjmedia_common_head
{
#if defined(PJ_IS_BIG_ENDIAN) && PJ_IS_BIG_ENDIAN!=0
    unsigned	    version:2;	/**< packet type            */
    unsigned	    p:1;	    /**< padding flag           */
    unsigned	    count:5;	/**< varies by payload type */
    unsigned	    pt:8;	    /**< payload type           */
#else
    unsigned	    count:5;	/**< varies by payload type */
    unsigned	    p:1;	    /**< padding flag           */
    unsigned	    version:2;	/**< packet type            */
    unsigned	    pt:8;	    /**< payload type ->256     */
#endif
    unsigned	    length:16;	/**< packet length          */
    pj_uint32_t	    ssrc;	    /**< SSRC identification    */
} pjmedia_common_head;


typedef struct pjmedia_vid_command
{
    trans_channel       comm_chan;
    on_comm_recv        comm_cb;
    char                send_buff[PJMEDIA_MAX_MTU];  /**< Outgoing packet.        */

    void                *user_data;    //struct point to self
    //pj_bool_t           attached;
    int                 maxFds;

    pjmedia_files_task  ftask;
}pjmedia_vid_command;


int  med_file_data_send(char* buffer, unsigned int size);

#endif


/**
select函数的接口比较简单：
int select(int nfds, fd_set *readset, fd_set *writeset, fd_set* exceptset, struct tim *timeout);

功能：
测试指定的fd可读？可写？有异常条件待处理？    

参数：nfds   
需要检查的文件描述字个数（即检查到fd_set的第几位），数值应该比三组fd_set中所含的最大fd值更大，一般设为三组fd_set中所含的最大fd值加1
（如在readset,writeset,exceptset中所含最大的fd为5，则nfds=6，因为fd是从0开始的）。设这个值是为提高效率，使函数不必检查fd_set的所有1024位。

timeout
有三种可能：
1. timeout=NULL（阻塞：直到有一个fd位被置为1函数才返回）
2. timeout所指向的结构设为非零时间（等待固定时间：有一个fd位被置为1或者时间耗尽，函数均返回）
3. timeout所指向的结构，时间设为0（非阻塞：函数检查完每个fd后立即返回）

select 返回值：
负值：select错误
正值：某些文件可读写或出错
0：等待超时，没有可读写或错误的文件

* 
* 
操作fd_set的四个宏：
fd_set set;
FD_ZERO(&set);    
FD_SET(0, &set);  用于在文件描述符集合中增加一个新的文件描述符。
FD_CLR(4, &set);  用于在文件描述符集合中删除一个文件描述符。
FD_ISSET(5, &set);  用于测试指定的文件描述符是否在该集合中
*/

