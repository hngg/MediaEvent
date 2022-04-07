
#include "glog.h"
#include "vid_unpack.h"
#include "dealrtpunpack.h"


void * Launch_CPlus(unsigned int nCodecID)
{
        pjmedia_vid_rtp_unpack* pUnpack = pjmedia_unpack_alloc_frame();
        if(pUnpack)
        {
            pjmedia_unpack_reset_frame(pUnpack);
            pUnpack->nCodecID = nCodecID;
            log_debug("create unpack frame:%p, codec is:%.*s", pUnpack, 4, (char*)(&nCodecID));
        }

        return (void *)pUnpack;
}

int RTPUnpackH264(void *obj, char *pInputRtpPack, int nInputLen, void **pOutFrameData)
{
        unpack_frame_status status;
        pjmedia_vid_rtp_unpack* pUnpack = (pjmedia_vid_rtp_unpack *)obj;
        if(pUnpack && pOutFrameData)
        {
                status = pjmedia_unpack_rtp_h264((unsigned char*)pInputRtpPack, (unsigned int)nInputLen, pUnpack);
                if(EN_UNPACK_FRAME_FINISH == status)
                {                                                
                        *pOutFrameData = pUnpack->stPreFrame.pFrameBuf;                           
                        return pUnpack->stPreFrame.uFrameLen;                              
                }
                else if(EN_UNPACK_FRAME_FAILURE == status)
                {
                        log_error("unpack video frame failure, need to send rtcp fir.");
                        return -1;
                }
                else
                {
                        return 0;
                }
        }
        else
        {
                return -2;
        }
}

void CRtpPackDecoderDestroy(void *obj)
{
        pjmedia_vid_rtp_unpack* pUnpack = (pjmedia_vid_rtp_unpack *)obj;
        pjmedia_unpack_free_frame(pUnpack);
}


