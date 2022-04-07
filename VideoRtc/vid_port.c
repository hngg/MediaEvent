
#include <unistd.h>
#include "vid_port.h"
#include "vid_stream.h"
#include "glog.h"
#include "utils.h"
#include "vid_unpack.h"
//#include "jbuf_opt.h"
#include "jitter_buffer.h"
#include "dealrtpunpack.h"

//#define SAVE_RECV_FILE 1

#ifdef SAVE_RECV_FILE
    FILE *mwFile = NULL;
    const char *FILE_PATH = "save.h264";
#endif

int tount = 0;
static void worker_thread_jbuf(void *arg)
{	
    pjmedia_vid_port *port = (pjmedia_vid_port *)arg;
    pjmedia_vid_stream *stream = (pjmedia_vid_stream *)port->useStream;

	int status = 0, frame_len, continumDec = 0;
	//pj_uint8_t rtpPack[PJMEDIA_MAX_MTU] = {0};
    while(!port->jbuf_recv_thread.thread_quit)
	{
		if (continumDec > 2)
		{
			usleep(1000);
		}
	
        void* rtp_unpack_obj = stream->ringbuf->rtp_unpack_obj;
        status = ringbuffer_read_frame(stream->ringbuf);
        if(status == 0) 
        {
            continumDec = 0;
			usleep(5000);
        }
        else if(status == SEQ_IDR)
        {
            //pjmedia_unpack_reset_frame(rtp_unpack_obj);
            if(stream->status_cb)
                stream->status_cb(NET_FIR_REQ, 0, 0, 0, 0);

            continue;
        }
        else if(status == EN_UNPACK_FRAME_FINISH)
        {
            pjmedia_vid_rtp_unpack* pUnpack = (pjmedia_vid_rtp_unpack *)rtp_unpack_obj;
            unsigned char* rtp_unpacked_buf = pUnpack->stPreFrame.pFrameBuf;
            frame_len = pUnpack->stPreFrame.uFrameLen;
            if(pUnpack->stPreFrame.uFrameLen > 0)
            {
                tount++;
                vframe_psecond *vframe_ps = &stream->vframe_ps;
                log_warn("------RTPUnpackH264__ count:%d frame_len:%d ftype:%d codecType:%d framerate:%d------\n", 
                        tount, frame_len, (stream->codecType==H264_HARD_CODEC)?rtp_unpacked_buf[4]&0x1F:GET_H265_NAL_TYPE(rtp_unpacked_buf[4]), 
                        stream->codecType, vframe_ps->frameRate);
                
                //framerate statistic
                if(((stream->codecType==H264_HARD_CODEC) && ((rtp_unpacked_buf[4]&0x1F)==7)) || ((stream->codecType==H265_HARD_CODEC)&&(GET_H265_NAL_TYPE(rtp_unpacked_buf[4])==32)))
                {
                    long currentTime = getCurrentTimeMs();
                    if(vframe_ps->lastTimeMs!=0) {
                        vframe_ps->frameRate = vframe_ps->frameCount*1000/(currentTime-vframe_ps->lastTimeMs);
                    }
                    vframe_ps->lastTimeMs = currentTime;
                    vframe_ps->frameCount = 0;
                }
                vframe_ps->frameCount++;

                #ifdef __ANDROID__
                	struct timeval tv_start, tv_end;
					gettimeofday(&tv_start,NULL);
                    if(port->decoder)
                        mediacodec_decoder_render(port->decoder, (uint8_t*)rtp_unpacked_buf, frame_len);
                    gettimeofday(&tv_end,NULL);
					uint64_t ts = tv_end.tv_sec*1000000 + tv_end.tv_usec - (tv_start.tv_sec*1000000 + tv_start.tv_usec );
                    log_error("mediacodec_decoder_render use time:%lld us", ts);
                #else
                    if(port->rtp_cb && rtp_unpacked_buf) //config at vid_stream_create_ios
                        port->rtp_cb((char*)rtp_unpacked_buf, frame_len);
                #endif
                
                #ifdef SAVE_RECV_FILE
                    if(mwFile)
                        fwrite((uint8_t*)rtp_unpacked_buf, 1, frame_len, mwFile);
                #endif
            }
        }
    }
    log_error("worker_thread_jbuf thread exit threadid:%d\n", (int)port->jbuf_recv_thread.threadId);
}

int vid_port_start(pjmedia_vid_port *vidPort)
{
    pj_status_t status = 0;
    #ifdef SAVE_RECV_FILE
        mwFile = fopen(FILE_PATH, "w");
    #endif
    
#ifdef __ANDROID__
    if(vidPort && vidPort->surface) 
    {
        pjmedia_vid_stream *stream = (pjmedia_vid_stream *)vidPort->useStream;
        int codecType = (stream->codecType == H264_HARD_CODEC)?AVC_CODEC:HEVC_CODEC;
        vidPort->decoder = mediacodec_decoder_alloc4(codecType, vidPort->surface);//AVC_CODEC 0, HEVC_CODEC 1
        mediacodec_decoder_open(vidPort->decoder);
    }
#endif

    pj_thread_create("jbuf_recv", (thread_proc *)&worker_thread_jbuf, vidPort, 0, 0, &vidPort->jbuf_recv_thread);
    return status;
}

int vid_port_stop(pjmedia_vid_port *vidPort) 
{
    pj_status_t status = 0;
    vidPort->jbuf_recv_thread.thread_quit = 1;

#ifdef __ANDROID__
    if(vidPort->decoder) 
    {
        mediacodec_decoder_close(vidPort->decoder);
		mediacodec_decoder_free(vidPort->decoder);
		vidPort->decoder = NULL;
	}
#endif

#ifdef SAVE_RECV_FILE
    if(mwFile != NULL) {
        fclose(mwFile);
        mwFile = NULL;
    }
#endif

    return status;
}
