
#ifndef _FILES_H
#define _FILES_H

#include <stdio.h>

#include "types.h"
#include "transport_udp.h"
#include "video_rtc_api.h"


#define FILENAME_MAX_LEN    256
#define LAST_NAIL_NAME      "lastnail.jpg"
#define RECV_NAIL_DIR       "/recvnail/"
#define RECV_PHOTO_DIR      "/recvphoto/"
#define SAVE_FILE_DIR       "/savefile/"

//file status callback
#define FILESTATUS_SEND_DONE 1
#define FILESTATUS_RECV_DONE 2

//file type
#define TASKTYPE_THUMBNAIL  101
#define TASKTYPE_PHOTOVIEW  102
#define TASKTYPE_NORMALFILE 103

//file communicate session
#define FILE_SEND_REQ_SYN   1
#define FILE_SEND_REQ_ACK   2
#define FILE_RECV_REQ_SYN   11
#define FILE_RECV_REQ_ACK   12
#define FILE_SEND_DONE_SYN  21  //send done
#define FILE_SEND_DONE_ACK  22
#define FILE_RECV_DONE_SYN  31  //recv done
#define FILE_RECV_DONE_ACK  32
#define FILE_NACK_REQ_SYN   51  //nack
#define FILE_NACK_REQ_ACK   52

//file head info ->8byte
#pragma pack(1)
typedef struct pjmedia_file_hdr
{
#if defined(PJ_IS_BIG_ENDIAN) && (PJ_IS_BIG_ENDIAN!=0)
    pj_uint16_t v:2;		/**< packet type/version	    */
    pj_uint16_t p:1;		/**< padding flag		    */
    pj_uint16_t x:1;		/**< extension flag		    */
    pj_uint16_t cc:4;		/**< CSRC count			    */
    pj_uint16_t m:1;		/**< marker bit			    */
    pj_uint16_t pt:7;		/**< payload type		    */
#else
    pj_uint16_t cc:4;		/**< CSRC count			    */
    pj_uint16_t x:1;		/**< header extension flag	    */ 
    pj_uint16_t p:1;		/**< padding flag		    */
    pj_uint16_t v:2;		/**< packet type/version	    */
    pj_uint16_t pt:7;		/**< payload type		    */
    pj_uint16_t m:1;		/**< marker bit			    */
#endif

    pj_uint16_t seq;		/**< sequence number		    */
    pj_uint32_t ts;		    /**< timestamp			    */

}pjmedia_file_hdr;
#pragma pack()


#define FILE_HEAD_LENGTH 8
#define RECV_MARK_SIZE   1024 //1024*4*8 packages, 32*1024*1420=46,530,560 byte

//file info  ->2+2+8+256+256+4+4 = 532byte
typedef struct pjmedia_files_info
{
    pj_uint16_t         maxSeq;         //收发文件最大的序列号，由文件大小计算得到
    pj_uint16_t         lastSeq;        //begin from 0
    double              fileSize;       //浮点数类型不能与unsigned关键字组合成 无符号浮点数
    char                filepath[FILENAME_MAX_LEN]; //文件的绝对路径
    char                filename[FILENAME_MAX_LEN]; //文件的名称
    int                 filetype;       //file type,like thumbnail or normal files
    unsigned int        fileid;         //file task id,every file will had a id number
} pjmedia_files_info;

typedef struct pjmedia_files_task
{
    trans_channel       file_chan;
    pjmedia_files_info  file_info;
    char                savedir[FILENAME_MAX_LEN];
    char                buffer[PJMEDIA_MAX_MTU];
    FILE*               fd;
    on_file_status      fstatus_cb;
    pj_uint16_t         isrecv;         //任务是接收端还是发送端

    pj_uint16_t         recvSeqCount;   //在65535的范围内接收到的seq数量
    int                 recvMark[RECV_MARK_SIZE];   //收到文件数据包数在相应的bit标记为1,可收包数为RECV_MARK_SIZE*sizeof(int)*8
    //unsigned int        markIndex;    //丢包的第几个double, 20211102备用

} pjmedia_files_task;

//file nack node ->8byte
typedef struct pjmedia_file_nack_info
{
    pj_uint16_t	id;
	pj_uint16_t	node_count;
    int         big_seg;    //用于大文件分段
} pjmedia_file_nack_info;

//file nack node ->4byte
typedef struct pjmedia_file_nack_node
{
    pj_uint16_t	baseSeq;   //lost package begin seq
	pj_uint16_t	lostNum;   //lost package number
} pjmedia_file_nack_node;
#define FILE_NACK_NODE_LENGTH 4

inline int setBitOn(int *value, int pos);

int build_comm_file_head(char*buffer, int pt);
int add_comm_file_nack_node(char*buffer, pj_uint16_t baseSeq, pj_uint16_t lostNum);
int buildLostPackage(int *markRow, int maxSeq, char*buffer);//return package length

//just for test
int  showIntBit(int value, int length);
void printBitMark(int *markRow, int maxSeq);


#endif

/**
 * https://www.cnblogs.com/yedushusheng/p/4333833.html
 * 
    int fseek(FILE *stream, long offset, int fromwhere);

    fseek 用于二进制方式打开的文件,移动文件读写指针位置.
    
    int fseek( FILE *stream, long offset, int origin );
    第一个参数stream为文件指针
    第二个参数offset为偏移量，整数表示正向偏移，负数表示负向偏移
    第三个参数origin设定从文件的哪里开始偏移,可能取值为：SEEK_CUR、 SEEK_END 或 SEEK_SET
    SEEK_SET： 文件开头
    SEEK_CUR： 当前位置
    SEEK_END： 文件结尾
    其中SEEK_SET,SEEK_CUR和SEEK_END和依次为0，1和2.
    　
    简言之：
    　　fseek(fp,100L,0);把fp指针移动到离文件开头100字节处；
    　　fseek(fp,100L,1);把fp指针移动到离文件当前位置100字节处；
    　  fseek(fp,100L,2);把fp指针退回到离文件结尾100字节处。

    返回值 成功，返回0，失败返回非0值，并设置error的值，可以用perror()函数输出错误
 *
 */


