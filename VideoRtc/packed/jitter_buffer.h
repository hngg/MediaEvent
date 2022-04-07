#ifndef __JITTER_BUFFER_H__
#define __JITTER_BUFFER_H__

#include "types.h"

//#define _slow_rtp_head_flag				0xcccc			//慢包标记
//#define _lost_rtp_head_flag				0xff00			//丢包标记

#define MAX_RTP_SEQ_VALUE				(unsigned short)0xffff //65535   //unsigned short max value 0xffff
#define MAX_RTP_SEQ_WINDOW			    640

#define MAX_LOST_WAIT_TIME_MS 			800     //max lost packet to resend wait time //500ms
#define MAX_LOST_WAIT_TIME_UNKNOW		1000	//150毫秒 未知终端类型的最大等待时间

#define MAX_PACKET_NUMBER               2048	//ringbuffer package size
#define VIDEO_PACKET_LENGTH             1500


//read and write packet to ringbuffer order by seq
#define SEQ_OK							0
#define SEQ_NORMAL_ERROR				-10000
#define SEQ_DISCONTINUOUS				-10001
#define SEQ_DISORDER					-10002
#define SEQ_IDR							-10003


typedef struct
{
    unsigned short bgnSeq;      //begin seq
    unsigned short endSeq;      //end seq
    unsigned int   lostTotal;   //packet number
	unsigned int   lostLeft;
    unsigned char  frameType;   //frame type
}LostedPackets;

//pj_ssize_t is long
typedef struct
{
	int 			resend_support; /*for support resend*/
	int 			need_idr;

	unsigned int   	uPacketSize;         /* 单packet最大大小 */
	unsigned int   	uPacketNum;          /* 最大缓存Packet数量 */
	unsigned int   	uMaxPktWin;          /* 最大packet守候窗口大小 */
	unsigned int   	uMaxPktTime;         /* 最大packet等待窗口时间 */
	unsigned int   	uPacketCount;        /* ringbuffer接收packet总数 */
	
	int				uPrePos;             /* 前一个rtp包写入的位置 */ 	//maybe -1
    unsigned short  uPreSeq;             /* 前一个rtp包写入的seq */ 	//unsigned short
	pj_ssize_t      uPreTime;            /* 前一个rtp包写入的时间 */
	int				uPreReadPos;         /* 前一个rtp包读取位置 */   	//maybe -1
	unsigned short  uPreReadSeq;         /* 前一个rtp包读取的seq */ 	//unsigned short
	pj_ssize_t		uPreReadTime;        /* 前一个rtp包读取的时间 */

	LostedPackets	lostPack;
	int             uCuPktLost;          /* 等待连续丢包数 */
	unsigned int   	uBufsizeTotal;		 //total buffer size
	unsigned char* 	xbuffer;
    
	void*           rtp_unpack_obj; 	 //had packed frame buffer,may thinking this as NAL frame, equal struct pjmedia_vid_rtp_unpack
	void*           rtp_unpacked_buf;	 //temp pointer, unpack frame buffer， point to pUnpack->stPreFrame.pFrameBuf

    int             status;
}RingBuffer;

typedef struct
{
	unsigned short uForRead:1;
	unsigned short uResendFlg:3;
	unsigned short uPktLen:12;         	/* rtp包大小 */
	unsigned short uPktSeq;            	/* 包序列号 */
	pj_ssize_t     uRecvTime;       	/* 收包时间戳 */
	unsigned char  xRtpBuf[0];         	/* rtp包数据 */
}Packet_Store_ST_;


pj_status_t ringbuffer_create(unsigned codecType, pj_bool_t resend_support, RingBuffer **p_jb);
void ringbuffer_destory(RingBuffer* pRingBuffer);
void ringbuffer_reset(RingBuffer* pRingBuffer);

int ringbuffer_write(RingBuffer* pRingBuffer, unsigned char* pRtpPkt, unsigned uRtpLen);
pj_status_t ringbuffer_read(RingBuffer* pRingBuffer, unsigned char* pOutPkt, unsigned* pOutLen);
pj_status_t ringbuffer_read_frame(RingBuffer* pRingBuffer);

#endif
