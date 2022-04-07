

#include <sys/time.h>
#include <stdlib.h>
#include "glog.h"
#include "rtp.h"
#include "utils.h"
#include "vid_unpack.h"
#include "jitter_buffer.h"
#include "dealrtpunpack.h"

#define AVC_SPS		7
#define RBUFF_SIZE 	VIDEO_PACKET_LENGTH*MAX_PACKET_NUMBER	//1500*2048 byte = 3000kb == 3M

pj_status_t ringbuffer_create(unsigned codecType, pj_bool_t resend_support, RingBuffer **pRbout)
{
	RingBuffer*pRingBuf = (RingBuffer*)malloc(sizeof(RingBuffer));//
	if(!pRingBuf)
	{
		log_error("allocate memory for _RingBuffer_ST_ failure.");
		return -1;
	}

	int bufferSize = RBUFF_SIZE;
	pRingBuf->xbuffer = (unsigned char*)malloc(bufferSize);
	if(!pRingBuf->xbuffer)
	{
		log_error("allocate memory for xbuffer failure.");
		return -1;
	}
	
	pj_bzero(pRingBuf->xbuffer, bufferSize);

    pRingBuf->resend_support = resend_support;
	pRingBuf->rtp_unpack_obj = Launch_CPlus(codecType);

	ringbuffer_reset(pRingBuf);

	*pRbout = pRingBuf;
	return PJ_SUCCESS;
}

void ringbuffer_destory(RingBuffer* pRingBuffer)
{
	//PJ_UNUSED_ARG(pRingBuffer);
	if(!pRingBuffer)
		return;

    if(pRingBuffer->xbuffer) {
		free(pRingBuffer->xbuffer);
        pRingBuffer->xbuffer = NULL;
    }

	if(pRingBuffer->rtp_unpack_obj)
		CRtpPackDecoderDestroy(pRingBuffer->rtp_unpack_obj);

    if(pRingBuffer) {
        free(pRingBuffer);
        pRingBuffer = NULL;
    }

	return;
}

void ringbuffer_reset(RingBuffer* pRingBuffer)
{
	if(!pRingBuffer)
	{
		log_error("pRingBuffer is null.");
		return;
	}

	pRingBuffer->need_idr		 = 0;

    pRingBuffer->uPrePos         = -1; //int
    pRingBuffer->uPreSeq         = 0;  //unsigned short
	pRingBuffer->uPreTime        = 0;
    pRingBuffer->uPreReadPos     = -1; //int
    pRingBuffer->uPreReadSeq     = 0;  //unsigned short
    pRingBuffer->uPreReadTime    = 0;

    
	pRingBuffer->uPacketSize = VIDEO_PACKET_LENGTH;//1500
	pRingBuffer->uPacketNum  = MAX_PACKET_NUMBER;//2048
	pRingBuffer->uMaxPktWin  = MAX_RTP_SEQ_WINDOW;//640

    pRingBuffer->uBufsizeTotal   = 0;
    pRingBuffer->uPacketCount    = 0;
	pRingBuffer->uCuPktLost 	 = 0;
    pRingBuffer->status          = SEQ_OK;
    
    pRingBuffer->uMaxPktTime = (!pRingBuffer->resend_support)?0:MAX_LOST_WAIT_TIME_MS;
	pj_bzero(pRingBuffer->xbuffer, VIDEO_PACKET_LENGTH*MAX_PACKET_NUMBER);
    
	pjmedia_unpack_reset_frame(pRingBuffer->rtp_unpack_obj);

	log_debug("finish,resend_support:%d,uMaxPktTime:%d MAX_PACKET_NUMBER:%d\n", pRingBuffer->resend_support, pRingBuffer->uMaxPktTime, pRingBuffer->uPacketNum);
}

int ringbuffer_write(RingBuffer* pRingBuffer, unsigned char* pRtpPkt, unsigned uRtpLen)
{
	int status = SEQ_OK;
	unsigned short packSeq = 0;			//the newest package sequence
	Packet_Store_ST_ *pStorePkt = NULL;
	int index = 0, getLostPack = 0;
	pj_ssize_t uCurTS;
	
	if(!pRingBuffer || !pRtpPkt)
	{
		log_error("parameter error, pRingBuffer:%p, pRtpPkt:%p", pRingBuffer, pRtpPkt);
		return SEQ_NORMAL_ERROR;
	}

	if(uRtpLen <= RTP_HEAD_LENGTH || uRtpLen > (pRingBuffer->uPacketSize - sizeof(Packet_Store_ST_)))
	{
		log_error("parameter error, uRtpLen:%u", uRtpLen);
		return SEQ_NORMAL_ERROR;
	}

	packSeq = pj_ntohs(*(unsigned short*)(pRtpPkt+2));

	/* check rtp packet recv delay */
	uCurTS = getCurrentTimeMs();
	if(0 == pRingBuffer->uPreTime)
	{
		pRingBuffer->uPreTime = uCurTS;
	}

	if(pRingBuffer->need_idr)
	{
		pjmedia_rtp_hdr *head = (pjmedia_rtp_hdr*)pRtpPkt;
		if(head->x)
		{
			char type = pRtpPkt[sizeof(pjmedia_rtp_hdr) + sizeof(rtp_extend)] & 0x1F;
			if(type == AVC_SPS)
			{
				pRingBuffer->need_idr = 0;
			}
		}
		else
		{
			char type = pRtpPkt[sizeof(pjmedia_rtp_hdr)] & 0x1F;
			(void)type;
			//log_debug("type--------------------:%d", type);
		}
	}

	/* store the rtp packet */
	index = packSeq%pRingBuffer->uPacketNum;
	pStorePkt = (Packet_Store_ST_*)(pRingBuffer->xbuffer + index*pRingBuffer->uPacketSize);	
	pStorePkt->uForRead 	= 1;
	pStorePkt->uResendFlg 	= 0;
    pStorePkt->uPktSeq 		= packSeq;		
    pStorePkt->uRecvTime 	= uCurTS;
	pStorePkt->uPktLen 		= uRtpLen;
	pj_memcpy(pStorePkt->xRtpBuf, pRtpPkt, uRtpLen);

	/* first packet or reset begin */
	if(-1 == pRingBuffer->uPrePos)  //packSeq first seq maybe 0
	{
		pRingBuffer->uPreReadSeq 	= packSeq; //first read sequence, packSeq >= 0 and < 0xffff

		pRingBuffer->uPrePos        = index;
        pRingBuffer->uPreSeq        = packSeq;    //current can read packet seq
        pRingBuffer->uPreTime       = uCurTS;

		pRingBuffer->uBufsizeTotal += uRtpLen;
    	pRingBuffer->uPacketCount++;

		return status;
	}

	if(packSeq - pRingBuffer->uPreSeq == 1)
	{
		//normal
	}
	else
	{
		LostedPackets *lostPack = &pRingBuffer->lostPack;
		//disorder or get the lost package
		if(packSeq < pRingBuffer->uPreSeq)
		{
			if(lostPack->lostLeft>0 && packSeq>=lostPack->bgnSeq && packSeq<=lostPack->endSeq)
			{
				getLostPack 		= 1;
				lostPack->lostLeft -= 1;
				log_debug("rtp lost package reget lost packet:%d", packSeq);
			}
		}
		else//lost package
		{
			if(MAX_RTP_SEQ_VALUE!=(pRingBuffer->uPreSeq-packSeq)) 
			{
				if(packSeq > (pRingBuffer->uPreSeq+1))
				{
					if(lostPack->lostLeft > 0)
						log_debug("rtp lost package process error, lostLeft is %d", lostPack->lostLeft);

					lostPack->bgnSeq 	= pRingBuffer->uPreSeq + 1;	//>=
					lostPack->endSeq 	= packSeq - 1;				//<=
					lostPack->lostTotal = lostPack->endSeq - lostPack->bgnSeq +1;
					lostPack->lostLeft  = lostPack->lostTotal;
					status 				= SEQ_DISCONTINUOUS;
					pRingBuffer->status = status;
					
					log_error("find discontinuous lost package diff:%d uPreSeq:%d packSeq:%d",
							   packSeq - pRingBuffer->uPreSeq-1, pRingBuffer->uPreSeq, packSeq);
				}
			}//else max seq
		}
	}
    
    if(!getLostPack && packSeq>pRingBuffer->uPreSeq)//important
    {
        pRingBuffer->uPrePos        = index;
        pRingBuffer->uPreSeq        = packSeq;    //current can read packet seq
        pRingBuffer->uPreTime       = uCurTS;
    }

    pRingBuffer->uBufsizeTotal += uRtpLen;
    pRingBuffer->uPacketCount++;
    
	log_debug("rtp seq:%u, len:%d, to buffer pos:%d", packSeq, pStorePkt->uPktLen, index);
	
	return status;
}

pj_status_t ringbuffer_read_frame(RingBuffer* pRingBuffer) 
{
	pj_status_t status = 0;

	int  curReadSeq = -1, curReadPos = -1, canRead = 0;
	Packet_Store_ST_ *pStorePkt = NULL;

	//important two steps, 1:uPrePos!=-1 2:uPreReadPos!=-1 (for the correct read position)
	if(-1 != pRingBuffer->uPreReadPos)	//normal read
	{
		curReadSeq = pRingBuffer->uPreReadSeq+1;
	}
	else	//first read
	{
		if(-1 != pRingBuffer->uPrePos) //1 steps(uPrePos != -1)
		{
			curReadSeq = pRingBuffer->uPreReadSeq;
            
            //2 steps(uPreReadPos!=-1 and read one package)
            pRingBuffer->uPreReadPos = curReadSeq%pRingBuffer->uPacketNum;
		}
		else //had not write package to pRingBuffer
		{
			return status;
		}
	}

	if(curReadSeq != -1) 
	{
		pj_ssize_t curTime = getCurrentTimeMs();
		curReadPos = curReadSeq%pRingBuffer->uPacketNum;
		pStorePkt  = (Packet_Store_ST_ *)(pRingBuffer->xbuffer + curReadPos*pRingBuffer->uPacketSize);
		if( pStorePkt->uForRead )
		{
			canRead = 1;
			//pRingBuffer->uPreReadTime = getCurrentTimeMs();//normal read time
		}
		else
		{
			if((curTime - pRingBuffer->uPreReadTime) <= MAX_LOST_WAIT_TIME_MS)
			{
				//something to do

				return status;
			}
			else
			{
				//canRead = 1;
				ringbuffer_reset(pRingBuffer);
				
				pRingBuffer->need_idr = 1;

				pRingBuffer->uCuPktLost++;
				pRingBuffer->status = SEQ_OK;
				log_warn("lost packet count:%d seq %d\n", pRingBuffer->uCuPktLost, curReadSeq);

				return SEQ_IDR;
			}
		}

		if(canRead)
		{
			pjmedia_vid_rtp_unpack* pUnpack = (pjmedia_vid_rtp_unpack *)pRingBuffer->rtp_unpack_obj;

			status = pjmedia_unpack_rtp_h264((unsigned char*)pStorePkt->xRtpBuf, pStorePkt->uPktLen, pUnpack);	//rtp package, package length, unpack object
			// if(EN_UNPACK_FRAME_FINISH == status)
			// {                                                
			// 		//*pOutFrameData = pUnpack->stPreFrame.pFrameBuf;                           
			// 		return pUnpack->stPreFrame.uFrameLen;                              
			// }

			pStorePkt->uForRead 		= 0;
			pStorePkt->uResendFlg 		= 0;

			pRingBuffer->uPreReadPos 	= curReadPos;
			pRingBuffer->uPreReadSeq 	= curReadSeq;
			pRingBuffer->uPreReadTime 	= curTime;	//pStorePkt->uRecvTime;

			log_debug("rtp seq:%u, len:%d, from buffer pos:%u",
                      pStorePkt->uPktSeq, pStorePkt->uPktLen, curReadPos);
		}

	}//curReadSeq != -1

	return status;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

pj_status_t ringbuffer_read(RingBuffer* pRingBuffer, unsigned char* pOutPkt, unsigned* pOutLen)
{
	pj_status_t status = PJ_FALSE;
	int  curReadSeq = -1, curReadPos = -1, canRead = 0;
	Packet_Store_ST_ *pStorePkt = NULL;
	
	if(!pRingBuffer || !pOutPkt || !pOutLen)
	{
		log_error("parameter error, pRingBuffer:%p, pRtpPkt:%p, pRtpLen:%p",
                  pRingBuffer, pOutPkt, pOutLen);
        
		return status;
	}

    //important two steps, 1:uPrePos!=-1 2:uPreReadPos!=-1 (for the correct read position)
	if(-1 != pRingBuffer->uPreReadPos)//normal read
	{
        //if(pRingBuffer->uPreReadSeq==pRingBuffer->uPreSeq)//not data
        //    return status;
        
		curReadSeq = pRingBuffer->uPreReadSeq+1;
	}
	else	//first read
	{
		if(-1 != pRingBuffer->uPrePos) //1 steps(uPrePos != -1)
		{
			curReadSeq = pRingBuffer->uPreReadSeq;
            
            //2 steps(uPreReadPos!=-1 and read one package)
            pRingBuffer->uPreReadPos = curReadSeq%pRingBuffer->uPacketNum;
		}
		else //had not write package to pRingBuffer
		{
			return status;
		}
	}

	if(curReadSeq != -1) 
	{
		curReadPos = curReadSeq%pRingBuffer->uPacketNum;
		pStorePkt  = (Packet_Store_ST_ *)(pRingBuffer->xbuffer + curReadPos*pRingBuffer->uPacketSize);
		if( pStorePkt->uForRead )
		{
			canRead = 1;
			//pRingBuffer->uPreReadTime = getCurrentTimeMs();//normal read time
		}
		else
		{
			if(((getCurrentTimeMs() - pRingBuffer->uPreReadTime) <= MAX_LOST_WAIT_TIME_MS)) 
			{
				//something to do

				return PJ_FALSE;
			}
			else
			{
				//canRead = 1;
				ringbuffer_reset(pRingBuffer);
				pRingBuffer->need_idr = 1;

				pRingBuffer->uCuPktLost++;
				pRingBuffer->status = SEQ_OK;
				log_warn("lost packet count:%d seq %d\n", pRingBuffer->uCuPktLost, curReadSeq);

				return SEQ_IDR;
			}
		}

		if(canRead)
		{
			pj_memcpy(pOutPkt, pStorePkt->xRtpBuf, pStorePkt->uPktLen);
			*pOutLen = pStorePkt->uPktLen;

			pStorePkt->uForRead 		= 0;
			pStorePkt->uResendFlg 		= 0;

			pRingBuffer->uPreReadPos 	= curReadPos;
			pRingBuffer->uPreReadSeq 	= curReadSeq;
			pRingBuffer->uPreReadTime 	= pStorePkt->uRecvTime;

			status = PJ_TRUE;

			log_debug("rtp seq:%u, len:%d, from buffer pos:%u",
                      pStorePkt->uPktSeq, pStorePkt->uPktLen, curReadPos);
		}

	}//curReadSeq != -1

	return status;
}

