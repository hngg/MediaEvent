
#include "glog.h"
#include "dealrtpunpack.h"
#include <arpa/inet.h>
#include "h264_packetizer.h"
#include "rtp.h"

typedef enum RFC3984_NALU_TYPE
{
     UNDEFINE = 0,
     //1-23 NAL unit Single NAL unit packet per H.264
     STAP_A   = 24, //Single-timne aggregation packet
     STAP_B   = 25, //Single-timne aggregation packet
     MTAP16   = 26, //Multi-time aggregation packet
     MTAP24   = 27, //Multi-time aggregation packet
     FU_A     = 28, //Fragmentation unit
     FU_B     = 29  //Fragmentation unit
     //30-31 undefined
}RFC3984_NALU_TYPE;

/* add by j33783 20190524 */
#define _______H264_UNPACK______

static const char* pjmedia_unpack_get_frame_name(int iFrmType)
{
      switch(iFrmType) {
         case 1: return "I";
         case 2: return "P";
         case 3: return "B";
         default: return "UNKONW";
      }
      return "UNKONW";
}

/**
 * @brief reset frame buffer
 * 
 * @param pUnpack 
 */
void pjmedia_unpack_reset_buf(pjmedia_vid_rtp_unpack* pUnpack)
{
     if(!pUnpack)
         return;

     pUnpack->iFrameType    = 0;
     memset(pUnpack->pFrameBuf, 0, sizeof(H264_FRAME_MAX_SIZE));
     pUnpack->uFrameLen     = 0;
     pUnpack->usRtpCnt      = 0;
     pUnpack->usRtpESeq     = -1;
     pUnpack->usRtpSSeq     = -1;
     pUnpack->bLenom        = 0;
     pUnpack->uTimeStamp    = 0;
}

/**
 * @brief reset unpacked frame parameter
 * 
 * @param pUnpack 
 */
void pjmedia_unpack_reset_frame(pjmedia_vid_rtp_unpack* pUnpack)
{
     if(!pUnpack)
         return;

     pUnpack->iFrameType = 0;
     if(pUnpack->pFrameBuf)
        memset(pUnpack->pFrameBuf, 0, sizeof(H264_FRAME_MAX_SIZE));
     pUnpack->uFrameLen  = 0;
     pUnpack->usRtpCnt   = 0;
     pUnpack->usRtpESeq  = -1;
     pUnpack->usRtpSSeq  = -1;
     pUnpack->bLenom     = 0;
     pUnpack->uTimeStamp = 0;

     pUnpack->bVPS = 0;
     pUnpack->bPPS = 0;
     pUnpack->bPPS = 0;
     pUnpack->bIDR = 0;
}

pjmedia_vid_rtp_unpack* pjmedia_unpack_alloc_frame()
{
     pjmedia_vid_rtp_unpack* pUnpack = NULL;
     pUnpack = (pjmedia_vid_rtp_unpack*)malloc(sizeof(pjmedia_vid_rtp_unpack));

     if(!pUnpack)
     {
          log_error("malloc pjmedia_vid_rtp_unpack failure.");
          return NULL;
     }

     pUnpack->pFrameBuf = (unsigned char*)malloc(H264_FRAME_MAX_SIZE);
     if(!pUnpack->pFrameBuf)
     {
          free(pUnpack);
          pUnpack = NULL;
          log_error("malloc frame buffer failure.");
          return NULL;
     }

     pUnpack->stPreFrame.uFrameLen = 0;
     pUnpack->stPreFrame.pFrameBuf = (unsigned char*)malloc(H264_FRAME_MAX_SIZE);
     if(!pUnpack->stPreFrame.pFrameBuf)
     {
          free(pUnpack->pFrameBuf);
          pUnpack->pFrameBuf = NULL;
          free(pUnpack);
          pUnpack = NULL;
          log_error("malloc frame buffer failure.");
          return NULL;
     }

     memset(pUnpack->pFrameBuf, 0, sizeof(H264_FRAME_MAX_SIZE));
     pUnpack->iFrameType      = 0;
     pUnpack->uFrameLen       = 0;
     pUnpack->usRtpCnt        = 0;
     pUnpack->usRtpESeq       = -1;
     pUnpack->usRtpSSeq       = -1;
     pUnpack->bVPS = 0;
     pUnpack->bPPS = 0;
     pUnpack->bPPS = 0;
     pUnpack->bIDR = 0;
     pUnpack->uTimeStamp = 0;

    return pUnpack;
}

void pjmedia_unpack_free_frame(pjmedia_vid_rtp_unpack* pUnpack)
{
     if(!pUnpack)
     {
          return;
     }

     if(pUnpack->pFrameBuf)
     {
          free(pUnpack->pFrameBuf);
          pUnpack->pFrameBuf = NULL;
     }

     if(pUnpack->stPreFrame.pFrameBuf)
     {
          free(pUnpack->stPreFrame.pFrameBuf);
          pUnpack->stPreFrame.pFrameBuf = NULL;
     }

     free(pUnpack);
     pUnpack = NULL;
}

static int pjmedia_parse_rtp_packet(unsigned char* pRtpPkt, unsigned int uRtpLen,  rtp_packet_info* pPktInfo, unsigned int nCodecID)
{
     unsigned int uRtpHdrLen = 0;
     unsigned int uCSRCCnt = 0;
     //unsigned short* pusSeq = NULL;
     //unsigned int* puTimeStamp = NULL;
     //unsigned int* puSSRC = NULL;
     unsigned char* pH264Buf = NULL;
     unsigned char* pH265Buf = NULL;
     int nExtFlag;
     
     if(!pRtpPkt || uRtpLen<= RTP_BASE_HEADLEN )
     {
          return -1;
     }

     uRtpHdrLen = RTP_BASE_HEADLEN;

     memset(pPktInfo, 0, sizeof(rtp_packet_info));

     pjmedia_rtp_hdr *rtpHead = (pjmedia_rtp_hdr*)pRtpPkt;
     uCSRCCnt = rtpHead->cc;
     nExtFlag = rtpHead->x;
     pPktInfo->ucMarker       = rtpHead->m; //一帧的结束
     pPktInfo->ucPayloadType  = rtpHead->pt;
     pPktInfo->usSeq          = ntohs(rtpHead->seq);
     pPktInfo->uTimeStamp     = ntohl(rtpHead->ts);
     pPktInfo->uSSRC          = ntohl(rtpHead->ssrc);

     //////////////////////////////////

     //uCSRCCnt default is zero
     if(uCSRCCnt && uRtpLen > uCSRCCnt*4 + RTP_BASE_HEADLEN)
     {
          uRtpHdrLen += uCSRCCnt*4;
     }

     if(nExtFlag &&  uRtpLen >  uRtpHdrLen + 4 )
     {
          uRtpHdrLen += sizeof(rtp_extend);    //((pRtpPkt [uRtpHdrLen + 2] << 8) | pRtpPkt [uRtpHdrLen + 3] + 1)*4; // for transport_send_rtp2
     }

     if(uRtpHdrLen >= uRtpLen)
     {
          log_error("rtp packet parse failure. packet len:%u, header len:%u", uRtpLen, uRtpHdrLen);
          return -1;
     }
     
     if(MAX_RTP_PACKET_LEN < uRtpLen - uRtpHdrLen)
     {
          log_error("rtp packet parse failure. over max MTU limit, packet len:%u, header len:%u", uRtpLen, uRtpHdrLen);
          return -1;
     }

     pPktInfo->usPayloadLen = uRtpLen - uRtpHdrLen;
     pPktInfo->pPayloadBuf  = pRtpPkt + uRtpHdrLen;

     switch(nCodecID)
     {
          case PJMEDIA_FORMAT_H264:
               pH264Buf = pPktInfo->pPayloadBuf;
               pPktInfo->ucNalType = pH264Buf[0] & 0x1F;  
               fu_header *head = (fu_header*)(pH264Buf+1);
               switch(pPktInfo->ucNalType)
               {
                    case 28:
                         if(head->E && !pPktInfo->ucMarker)
                         {
                              log_warn("this packet is end of a frame, but rtp marker is not set.");
                              //pPktInfo->ucMarker = 1;
                         }
                         pPktInfo->ucFrmStart = head->S;
                    break;

                    case 5:
                    case 1:
                         pPktInfo->ucFrmStart = head->S;
                    break;
               }

               if(6 <=  pPktInfo->ucNalType && pPktInfo->ucNalType <= 8)
               {
                    pPktInfo->ucMarker = 0; /* add by j33783 20190815 force to unmark */ 
               }
          break;//h264

          case PJMEDIA_FORMAT_H265:
               pH265Buf = pPktInfo->pPayloadBuf;
               pPktInfo->ucNalType = GET_H265_NAL_TYPE(pH265Buf[0]);  //((pH265Buf[0])>>1) & 0x3F 
               switch(pPktInfo->ucNalType)
               {
                    case 49:             //fu
                         pPktInfo->ucFrmStart = ((pH265Buf[2]&0x80)>0)?1:0;
                    break;

                    case 19:            //idr
                    case 1:
                         pPktInfo->ucFrmStart = ((pH265Buf[2]&0x80)>0)?1:0;
                         // if(pH265Buf[2]&0x80) 
                         // {
                         //      pPktInfo->ucFrmStart = 1;
                         // }
                         // else
                         // {
                         //      pPktInfo->ucFrmStart = 0;
                         // }
                    break;
               }
          break;//h265
     }
    
    return 0;
}

static int pjmedia_unpack_h264(unsigned char* pPayload,  unsigned int uPayloadLen, unsigned char ucNalType,  
                               unsigned char* pFrameBuf, unsigned int uBufLen, unsigned int* pFrameLen)
{
     unsigned char* pPos  = NULL;
     unsigned char* pTemp = NULL;
          
     unsigned char   acStartCode[] = {0x00, 0x00, 0x00, 0x01};
     
     if(!pPayload || !uPayloadLen || !pFrameBuf ||!pFrameBuf)
     {
          log_error("invalid parameter, Payload=%p, Len:%u, FrameBuf=%p, FrameLen=%p", pPayload, uPayloadLen, pFrameBuf, pFrameBuf);
          return -1;
     }

     if(H264_MIN_PAYLOAD_SIZE >= uPayloadLen)
     {
          log_error("invalid payload size:%u", uPayloadLen);
          return -1;
     }

     if(1 <= ucNalType && 23 >= ucNalType) /* single NAL unit packet */
     {
          pPos = pFrameBuf + *pFrameLen;
          /* fill startcode  */
          memcpy(pPos, acStartCode, H264_START_CODE_SIZE);
          pPos += H264_START_CODE_SIZE;
          memcpy(pPos, pPayload, uPayloadLen);
          (*pFrameLen) += (H264_START_CODE_SIZE + uPayloadLen);            
     }
     else if(28 == ucNalType) /* FU-A */
     {
          pPos = pFrameBuf + *pFrameLen;
          pTemp = pPayload;

          if(*(pTemp+1) & 0x80)  /* Start Bit */
          {
               memcpy(pPos, acStartCode, H264_START_CODE_SIZE);
               pPos += H264_START_CODE_SIZE;
               *pPos++ = (*pTemp & 0x60) | (*(pTemp+1) & 0x1f);
          }

          pTemp += 2;
          memcpy(pPos, pTemp, uPayloadLen - 2);
          pPos += (uPayloadLen - 2);

          *pFrameLen =  (unsigned) (pPos - pFrameBuf);
     }
     else if(24 == ucNalType) /* STAP-A */
     {
          unsigned char* pStart = NULL;
          unsigned char* pEnd = NULL;
          unsigned char* qStart = NULL;
          unsigned char* qEnd = NULL;
          unsigned nal_size = 0;

          if(uBufLen - *pFrameLen < uPayloadLen+32)
          {
               log_error("Insufficient buffer for STAP-A packet. ");
               return -1;
          }

          pStart =  pFrameBuf + *pFrameLen;
          pEnd = pFrameBuf + uBufLen;

          qStart = pPayload + 1;
          qEnd = pPayload + uPayloadLen;

          while((pStart < pEnd) && (qStart < qEnd))
          {
               memcpy(pStart, acStartCode, H264_START_CODE_SIZE);
               pStart += H264_START_CODE_SIZE;

               nal_size =  (*qStart << 8) | *(qStart+1);
               qStart += 2;
               if(qStart + nal_size > qEnd)
               {
                    log_error("parse STAP-A packet failure. not enough buffer");
                    return -1;
               }

               memcpy(pStart, qStart, nal_size);
               pStart += nal_size;
               qStart += nal_size;

               *pFrameLen = (unsigned)(pStart - pFrameBuf);                    
          }
     }
     else
     {
          *pFrameLen = 0;
          log_error("unknow nal type:%d", ucNalType);
          return -1;
     }      

     return 0;
}

static int pjmedia_unpack_check_h264_nal(rtp_packet_info* pkginfo, pjmedia_vid_rtp_unpack* pUnpack)
{
      if(!pkginfo || !pUnpack)
      {
            return -1;
      }

      switch(pkginfo->ucNalType)
      {
          case 1:  /* P slice */
          {
               if(pkginfo->pPayloadBuf[1]&0x80) 
               {
                    pjmedia_unpack_reset_buf(pUnpack);
                    pUnpack->usRtpSSeq = pkginfo->usSeq;
                    log_debug("recv P frame start packet, rtp seq:%d", pkginfo->usSeq);
                    if(pkginfo->ucMarker) /* add by j33783 20190612, muti-slice, but one slice is a full frame */
                    {
                         pUnpack->usRtpESeq = pkginfo->usSeq;
                         log_debug("Also P frame end packet, rtp seq:%d", pkginfo->usSeq);
                    }
               }
               else if(pkginfo->ucMarker)
               {
                    pUnpack->usRtpESeq = pkginfo->usSeq;
                    log_debug("recv P frame end packet, rtp seq:%d", pkginfo->usSeq);
               }
               else
               {
                    log_debug("recv P frame middle packet, rtp seq:%d", pkginfo->usSeq);
               }
               pUnpack->iFrameType = 2;
               break;
          }
          case 5: /* I slice */
          {
               if(pkginfo->pPayloadBuf[1]&0x80)  /* IDR first packet */
               {
                    pUnpack->usRtpSSeq = pkginfo->usSeq;
                    log_debug("recv I frame start packet, rtp seq:%d", pkginfo->usSeq);
                    if(pkginfo->ucMarker) /* add by j33783 20190612, muti-slice, but one slice is a full frame */
                    {
                         pUnpack->usRtpESeq = pkginfo->usSeq;
                         log_debug("Also I frame end packet, rtp seq:%d", pkginfo->usSeq);
                    }
               }
               else if(pkginfo->ucMarker)
               {
                    pUnpack->usRtpESeq = pkginfo->usSeq;
                    log_debug("recv I frame end packet, rtp seq:%d", pkginfo->usSeq);
               }
               else
               {
                    log_debug("recv I frame middle packet, rtp seq:%d", pkginfo->usSeq);
               }
               pUnpack->iFrameType = 1;
               break;
          }
          case 6: /* SEI */
          {
               log_debug("recv SEI, rtp seq:%d", pkginfo->usSeq);
               break;
          }
          case 7: /* SPS */
          {    
               pjmedia_unpack_reset_frame(pUnpack);
               log_debug("recv SPS, rtp seq:%d", pkginfo->usSeq);                
               pUnpack->bSPS = 1;
               pUnpack->iFrameType = 1;
               break;
          }
          case 8: /* PPS */
          {
               pUnpack->bPPS = 1;
               log_debug("recv PPS, rtp seq:%d", pkginfo->usSeq);
               break;
          }
          case 28: /* FU-A */
          {
               int  iFrameType = 0;
               if(5 == (pkginfo->pPayloadBuf[1]&0x1f))
               {
                    iFrameType = 1;
		          pUnpack->iFrameType = iFrameType;
               }

                /* modify by j33783 20190731 */
               if((1 == (pkginfo->pPayloadBuf[1]&0x1f)))
               {       
                    if(pkginfo->pPayloadBuf[2]&0x40)  /* P frame slice */
                    {
                         iFrameType = 2;
                    }
                    else if(((pkginfo->pPayloadBuf[2]&0x70)>>4) == 2)  /* B frame slice */
                    {
                         iFrameType = 3;
                    }
                    else
                    {
                         iFrameType = 2;
                    }

			     pUnpack->iFrameType = iFrameType;
               }

               if(pkginfo->pPayloadBuf[1]&0x80)  /* first packet */
               {
                    if( (iFrameType == 2 || iFrameType == 3) && !pUnpack->bLenom 
                        && (pUnpack->uTimeStamp != pkginfo->uTimeStamp))
                    {
                         pjmedia_unpack_reset_buf(pUnpack);
			          pUnpack->iFrameType = iFrameType;
                    }                     

                    if(-1 == pUnpack->usRtpSSeq)
                    {
                         pUnpack->usRtpSSeq = pkginfo->usSeq;
                    }
                    
                    log_debug("recv FU-A %s start packet, rtp seq:%d", pjmedia_unpack_get_frame_name(pUnpack->iFrameType), pkginfo->usSeq);
               }
               else if(pkginfo->pPayloadBuf[1]&0x40)
               {
                    if(1== pkginfo->ucMarker)
                    {
                         pUnpack->usRtpESeq = pkginfo->usSeq;
			          pUnpack->bLenom = 0;
                    }
                    else
                    {
                         pUnpack->bLenom = 1;
                    }
                         log_debug("recv FU-A %s end packet, rtp seq:%d", pjmedia_unpack_get_frame_name(pUnpack->iFrameType), pkginfo->usSeq);
               }
               else
               {
                    log_debug("recv FU-A %s middle packet, rtp seq:%d", pjmedia_unpack_get_frame_name(pUnpack->iFrameType), pkginfo->usSeq);
               }
               
               break;
          }//28
          case 24: /* STAP-A */
          {
               log_debug("recv STAP-A, rtp seq:%d", pkginfo->usSeq);

               /* add by j33783 20190826 support  STAP-A Parsing */
               unsigned char* qStart = NULL;
               unsigned char* qEnd = NULL;
               unsigned nal_size = 0;
               unsigned char ucNalType;                

               qStart = pkginfo->pPayloadBuf + 1;
               qEnd = pkginfo->pPayloadBuf + pkginfo->usPayloadLen;

               while(qStart < qEnd)
               {
                    nal_size =  (*qStart << 8) | (*(qStart+1));
                    qStart += 2;

                    if(qStart + nal_size > qEnd)
                    {
                         break;
                    }
               
                    ucNalType = (*qStart)&0x1F;
                    if(6 == ucNalType)
                    {
                         log_debug("recv STAP-A SEI, rtp seq:%d", pkginfo->usSeq);
                    }
                    else if(7 == ucNalType)
                    {
                         pjmedia_unpack_reset_frame(pUnpack);
                         log_debug("recv STAP-A SPS, rtp seq:%d", pkginfo->usSeq);                
                         pUnpack->bSPS = 1;
                         pUnpack->iFrameType = 1;
                    }
                    else if(8 == ucNalType)
                    {
                         pUnpack->bPPS = 1;
                         log_debug("recv STAP-A PPS, rtp seq:%d", pkginfo->usSeq);
                    }
                    else
                    {
                         log_debug("recv STAP-A, NAL:%d, rtp seq:%d", ucNalType, pkginfo->usSeq);
                    }

                    qStart += nal_size;
               }
               break;
          }
          default:
          {
               log_error("recv unsupported nal:%d, rtp seq:%d", pkginfo->ucNalType, pkginfo->usSeq);
               return -1;
          }
     }//switch

     pUnpack->uTimeStamp = pkginfo->uTimeStamp;

    return 0;
}

/**
 * @brief api total
 * 
 * @param pRtpPkt 
 * @param uRtpLen 
 * @param pUnpack 
 * @return unpack_frame_status 
 */
unpack_frame_status pjmedia_unpack_rtp_h264(unsigned char* pRtpPkt, unsigned int uRtpLen,  pjmedia_vid_rtp_unpack* pUnpack)
{
     rtp_packet_info pkginfo;
     int status = -1;
     unpack_frame_status unpack_status = EN_UNPACK_FRAME_ERROR;
     
     if(!pRtpPkt || uRtpLen<= RTP_BASE_HEADLEN || !pUnpack)
     {
          log_error("invalid parameter.");
          return EN_UNPACK_FRAME_ERROR;
     }
      
     status = pjmedia_parse_rtp_packet(pRtpPkt, uRtpLen, &pkginfo, pUnpack->nCodecID);
     if(0 != status)
     {
          log_error("parse rtp packet failure.");
          return unpack_status;
     }
     
     log_debug("recv rtp, pt:%u, seq:%u, m:%d, ts:%u, ssrc:%d, naltype:%d, len:%d codecId:%s", pkginfo.ucPayloadType, pkginfo.usSeq,
                pkginfo.ucMarker, pkginfo.uTimeStamp, pkginfo.uSSRC,pkginfo.ucNalType, pkginfo.usPayloadLen, (char*)(&pUnpack->nCodecID));

     if(PJMEDIA_FORMAT_H264 == pUnpack->nCodecID)
     {
          pjmedia_unpack_check_h264_nal(&pkginfo, pUnpack);
          status = pjmedia_unpack_h264(pkginfo.pPayloadBuf, pkginfo.usPayloadLen, pkginfo.ucNalType,
                                        pUnpack->pFrameBuf, H264_FRAME_MAX_SIZE, &pUnpack->uFrameLen);
          if(0 != status)
          {
               log_error("unpack h264 failure.");
               pjmedia_unpack_reset_frame(pUnpack);
               return unpack_status;
          }

          if(6 <= pkginfo.ucNalType && 8 >= pkginfo.ucNalType)
          {
               unpack_status = EN_UNPACK_FRAME_START;
          }
          else
          {
               pUnpack->usRtpCnt++;
          }
          //log_debug("---------------------h264\n");
    }
    else
    {
          pjmedia_unpack_check_h265_nal(&pkginfo, pUnpack);
          status = pjmeida_h265_unpacketize(pRtpPkt, uRtpLen, pUnpack->pFrameBuf, (int*)&pUnpack->uFrameLen);
          if(status < 0)
          {
               log_error("unpack h265 failure, status:%d", status);
               pjmedia_unpack_reset_frame(pUnpack);
               return unpack_status;
          }
          else if(status == 0)
          {
               
          }
          else
          {
               pUnpack->uFrameLen = status;
          }
          
          if(32 == pkginfo.ucNalType || 33 == pkginfo.ucNalType
               || 34 == pkginfo.ucNalType || 39 == pkginfo.ucNalType)
          {
               unpack_status = EN_UNPACK_FRAME_START;
          }
          else
          {
               pUnpack->usRtpCnt++;
          }
     }
   
     log_debug("pack frame, buf:%p, len:%d, type:%s, rtp cnt:%d, start:%d, end:%d, vps:%d, sps:%d, pps:%d, idr:%d",
                    pUnpack->pFrameBuf, pUnpack->uFrameLen, pjmedia_unpack_get_frame_name(pUnpack->iFrameType),
                    pUnpack->usRtpCnt, pUnpack->usRtpSSeq, pUnpack->usRtpESeq,  pUnpack->bVPS, pUnpack->bSPS, pUnpack->bPPS, pUnpack->bIDR);

     if(1 == pUnpack->iFrameType) /* check I frame */
     {
          if(pkginfo.ucMarker)
          {
               status = pjmedia_unpack_check_frame(pUnpack);
               if(0 != status)
               {
                    pjmedia_unpack_reset_frame(pUnpack);
                    unpack_status = EN_UNPACK_FRAME_FAILURE;
                    return unpack_status;
               }
               else
               {
                    pUnpack->bIDR = 1;
               }
               unpack_status =  EN_UNPACK_FRAME_FINISH;
          }
          else
          {
               unpack_status =  EN_UNPACK_FRAME_MIDDLE;
          }
     }

     if(2 == pUnpack->iFrameType || 3 ==pUnpack->iFrameType ) /* check P/B frame */
     {
          /* add by j33783 20190731 兼容硬编码输出存在连帧打包错误的现象 */
          //if(pkginfo.pPayloadBuf[1]&0x40)
          if(pkginfo.ucMarker)
          {
               pUnpack->usRtpESeq = pkginfo.usSeq;
               //pkginfo.ucMarker = 1;
          }
            
          if(pkginfo.ucMarker)
          {
               status = pjmedia_unpack_check_frame(pUnpack);
               if(0 != status)
               {
                    //pjmedia_unpack_reset_buf(pUnpack);
                    pjmedia_unpack_reset_frame(pUnpack);
                    unpack_status = EN_UNPACK_FRAME_FAILURE;
                    return unpack_status;
               }

               if(!pUnpack->bIDR)
               {
                    //pjmedia_unpack_reset_buf(pUnpack);
                    pjmedia_unpack_reset_frame(pUnpack);
                    unpack_status = EN_UNPACK_FRAME_FAILURE;
                    return unpack_status;
               }
               unpack_status =  EN_UNPACK_FRAME_FINISH;
          }
          else
          {
               unpack_status =  EN_UNPACK_FRAME_MIDDLE;
          }
     }

     if(EN_UNPACK_FRAME_FINISH == unpack_status)
     {
          memcpy(pUnpack->stPreFrame.pFrameBuf, pUnpack->pFrameBuf, pUnpack->uFrameLen);
          pUnpack->stPreFrame.uFrameLen = pUnpack->uFrameLen;

          log_debug("video frame unpack, frame size=%d, type:%d, start seq:%d, end seq:%d, cnt:%d, sps:%d, pps:%d, idr:%d",
                         pUnpack->uFrameLen, pUnpack->iFrameType, pUnpack->usRtpSSeq, pUnpack->usRtpESeq, pUnpack->usRtpCnt,
                         pUnpack->bSPS, pUnpack->bPPS, pUnpack->bIDR);
          
          pjmedia_unpack_reset_buf(pUnpack);
     }

    return unpack_status;
}


void test_pjmedia_h264_pack(unsigned char* pRtpPkt, unsigned int uRtpLen)
{
     static pjmedia_vid_rtp_unpack* pUnpack = NULL;
     static FILE* fp = NULL;
     unpack_frame_status status;

     if(!pUnpack)
     {
          pUnpack = pjmedia_unpack_alloc_frame();
     }

     if(!fp)
     {
          fp = fopen("/sdcard/test.h264", "wb");
     }

     if(pUnpack)
     {
          status = pjmedia_unpack_rtp_h264(pRtpPkt, uRtpLen, pUnpack);
          if(EN_UNPACK_FRAME_FINISH == status)
          {
               log_debug("finish a h264 frame unpack, frame size=%d, type:%d, start seq:%d, end seq:%d, cnt:%d, sps:%d, pps:%d, idr:%d",
                    pUnpack->uFrameLen, pUnpack->iFrameType, pUnpack->usRtpSSeq, pUnpack->usRtpESeq, pUnpack->usRtpCnt,
                    pUnpack->bSPS, pUnpack->bPPS, pUnpack->bIDR);
               if(fp)
               {
                    fwrite(pUnpack->pFrameBuf, 1, pUnpack->uFrameLen, fp);
               }
               pjmedia_unpack_reset_buf(pUnpack);
          }
     }
     
}

////////////////////////////////////////////////////////////////h265//////////////////////////////////////////////////////////////////////////

int pjmedia_unpack_check_h265_nal(rtp_packet_info* pkginfo, pjmedia_vid_rtp_unpack* pUnpack)
{
     if(!pkginfo || !pUnpack)
     {
           return -1;
     }

      switch(pkginfo->ucNalType)
      {
          case 1:  /* hevc P frame packet*/
          {
               if(pkginfo->ucFrmStart) 
               {
                    pjmedia_unpack_reset_buf(pUnpack);
                    pUnpack->usRtpSSeq = pkginfo->usSeq;
                    log_debug("recv P frame start packet, rtp seq:%d", pkginfo->usSeq);
                    if(pkginfo->ucMarker) 
                    {
                         pUnpack->usRtpESeq = pkginfo->usSeq;
                         log_debug("also P frame end packet, rtp seq:%d", pkginfo->usSeq);
                    }
               }
               else if(pkginfo->ucMarker)
               {
                    pUnpack->usRtpESeq = pkginfo->usSeq;
                    log_debug("recv P frame end packet, rtp seq:%d", pkginfo->usSeq);
               }
               else
               {
                    log_debug("recv P frame middle packet, rtp seq:%d", pkginfo->usSeq);
               }
               pUnpack->iFrameType = 2;

               break;
          }
          case 19: /* hevc I frame packet*/
          {
               if(pkginfo->ucFrmStart)  /* IDR first packet */
               {
                    pUnpack->usRtpSSeq = pkginfo->usSeq;
                    log_debug("recv I frame start packet, rtp seq:%d", pkginfo->usSeq);
                    if(pkginfo->ucMarker)
                    {
                         pUnpack->usRtpESeq = pkginfo->usSeq;
                         log_debug("Also I frame end packet, rtp seq:%d", pkginfo->usSeq);
                    }
               }
               else if(pkginfo->ucMarker)
               {
                    pUnpack->usRtpESeq = pkginfo->usSeq;
                    log_debug("recv I frame end packet, rtp seq:%d", pkginfo->usSeq);
               }
               else
               {
                    log_debug("recv I frame middle packet, rtp seq:%d", pkginfo->usSeq);
               }
               pUnpack->iFrameType = 1;
               break;
          }
          case 32: /* hevc VPS packet */
          {
               //log_debug("recv VPS, rtp seq:%d", pkginfo->usSeq); 
               log_error("recv VPS, rtp seq------------------------------------------:%d", pkginfo->usSeq); 
               pjmedia_unpack_reset_frame(pUnpack);
               pUnpack->iFrameType = 1;
               pUnpack->bVPS = 1; 
               break;
          }
          case 33: /* hevc SPS packet */
          {                   
               log_debug("recv SPS, rtp seq:%d", pkginfo->usSeq);                
               pUnpack->bSPS = 1;               
               break;
          }
          case 34: /* hevc PPS packet */
          {
               pUnpack->bPPS = 1;
               log_debug("recv PPS, rtp seq:%d", pkginfo->usSeq);
               break;
          }
          case 39: /* hevc SEI packet */
          {
               log_debug("recv SEI, rtp seq:%d", pkginfo->usSeq);
               break;
          }
          case 48: /* hevc AP packet */
          {
               log_debug("recv STAP-A, rtp seq:%d", pkginfo->usSeq);
               /* add by j33783 20190926 support  STAP-A Parsing */
               unsigned char* qStart = NULL;
               unsigned char* qEnd = NULL;
               unsigned nal_size = 0;
               unsigned char ucNalType;                

               qStart = pkginfo->pPayloadBuf + 1;
               qEnd = pkginfo->pPayloadBuf + pkginfo->usPayloadLen;

               while(qStart < qEnd)
               {
                    nal_size =  (*qStart << 8) | (*(qStart+1));
                    qStart += 2;

                    if(qStart + nal_size > qEnd)
                    {
                         break;
                    }
                    
                    ucNalType = ((*qStart)>>1)&0x3F;
                    if(39 == ucNalType)
                    {
                         log_debug("recv STAP-A SEI, rtp seq:%d", pkginfo->usSeq);
                    }
                    else  if(32 == ucNalType)
                    {
                         log_debug("recv VPS, rtp seq:%d", pkginfo->usSeq); 
                         pjmedia_unpack_reset_frame(pUnpack);
                         pUnpack->iFrameType = 1;
                         pUnpack->bVPS = 1; 
                    }
                    else  if(33 == ucNalType)
                    {                           
                         log_debug("recv STAP-A SPS, rtp seq:%d", pkginfo->usSeq);                
                         pUnpack->bSPS = 1;
                    }
                    else if(34 == ucNalType)
                    {
                         pUnpack->bPPS = 1;
                         log_debug("recv STAP-A PPS, rtp seq:%d", pkginfo->usSeq);
                    }
                    else
                    {
                         log_debug("recv STAP-A, NAL:%d, rtp seq:%d", ucNalType, pkginfo->usSeq);
                    }

                    qStart += nal_size;
               }
               
               break;
          }
          case 49: /* hevc FU packet*/
          {
               unsigned char ucNalType = (pkginfo->pPayloadBuf[2])&0x3F;
               if(19 == ucNalType)
               {
                    pUnpack->iFrameType = 1;
               }

               if(1 == ucNalType)
               {
                    pUnpack->iFrameType = 2;
               }

               if( (pkginfo->pPayloadBuf[2])&0x80)
               {
                    if( (pUnpack->iFrameType == 2 || pUnpack->iFrameType == 3) && !pUnpack->bLenom 
                    && (pUnpack->uTimeStamp != pkginfo->uTimeStamp))
                    {
                         int iFrameType = pUnpack->iFrameType;
                         pjmedia_unpack_reset_buf(pUnpack);
                         pUnpack->iFrameType = iFrameType;
                    }                     

                    if(-1 == pUnpack->usRtpSSeq)
                    {
                         pUnpack->usRtpSSeq = pkginfo->usSeq;
                    }
                    
                    log_debug("recv FU-A %s start packet, rtp seq:%d", pjmedia_unpack_get_frame_name(pUnpack->iFrameType), pkginfo->usSeq);
               }
               else if(pkginfo->pPayloadBuf[2]&0x40)
               {
                    if(1== pkginfo->ucMarker)
                    {
                         pUnpack->usRtpESeq = pkginfo->usSeq;
                         pUnpack->bLenom = 0;
                    }
               else
               {
                    pUnpack->bLenom = 1;
               }
                    log_debug("recv FU-A %s end packet, rtp seq:%d", pjmedia_unpack_get_frame_name(pUnpack->iFrameType), pkginfo->usSeq);
               }
               else
               {
                    log_debug("recv FU-A %s middle packet, rtp seq:%d", pjmedia_unpack_get_frame_name(pUnpack->iFrameType), pkginfo->usSeq);
               } 
               break;
          }
          default:
          {
               log_error("recv unsupported nal:%d, rtp seq:%d", pkginfo->ucNalType, pkginfo->usSeq);
               return -1;
          }
      }
    
    pUnpack->uTimeStamp = pkginfo->uTimeStamp;
    return 0;
}

int pjmedia_unpack_check_frame(pjmedia_vid_rtp_unpack* pUnpack)
{
     unsigned short  usSeqGap = 0;
     if(!pUnpack)
     {
          return -1;
     }

     if(PJMEDIA_FORMAT_H265 == pUnpack->nCodecID && !pUnpack->bVPS)
     {
          log_error("VPS was not received, ignore this GoP.");
          return -1;
     }

     if(!pUnpack->bSPS)
     {
          log_error("SPS was not received, ignore this GoP.");
          return -1;
     }

     if(!pUnpack->bPPS)
     {
          log_error("PPS was not received, ignore this GoP.");
          return -1;
     }     

     usSeqGap = (pUnpack->usRtpESeq + 1 + 65536 - pUnpack->usRtpSSeq)%65536;

     if(1 == pUnpack->iFrameType) /* packet count for I frame*/
     {
          //if((usSeqGap != pUnpack->usRtpCnt) && (usSeqGap+2) != pUnpack->usRtpCnt  && (usSeqGap+3) != pUnpack->usRtpCnt) /* exclude SPS, PPS, SEI */
          if(pUnpack->usRtpCnt < usSeqGap)
          {
               log_error("I frame may lost packet, rtp start:%d, end:%d, cnt:%d", pUnpack->usRtpSSeq, pUnpack->usRtpESeq, pUnpack->usRtpCnt);
               //return -1;
          }
     }
     else
     {
          if(usSeqGap != pUnpack->usRtpCnt)
          {
               log_error("P frame may lost packet, rtp start:%d, end:%d, cnt:%d", pUnpack->usRtpSSeq, pUnpack->usRtpESeq, pUnpack->usRtpCnt);
               return -1;
          }
     }
       
      return 0;   
}





