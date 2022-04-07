

#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>


#include "utils.h"
#include "basedef.h"
#include "vid_unpack.h"

#include "vid_stream.h"
#include "video_rtc_api.h"

#ifdef __ANDROID__
#define SAVE_VIDEO_RAW 1
#endif 

typedef enum {
	_SOFT_CODING_SOFT_DECODING_,
	_SOFT_CODING_HARD_DECODING_,
	_HARD_CODING_SOFT_DECODING_,
	_HARD_CODING_HARD_DECODING_
}pjsua_call_hardware_e;

/*add for h264 and h265 define by 180732700, 20191113*/
typedef enum {
	AVC_NAL_SINGLE_NAL_MIN	= 1,
	AVC_NAL_UNIT_IDR = 5,
    AVC_NAL_UNIT_SEI = 6,
	AVC_NAL_UNIT_SPS = 7,
	AVC_NAL_UNIT_PPS = 8,
    AVC_NAL_SINGLE_NAL_MAX	= 23,
    AVC_NAL_STAP_A		= 24,
    AVC_NAL_FU_TYPE_A	= 28,
}avc_nalu_type_e;

typedef enum {
	HEVC_NAL_UNIT_IDR = 19,
	HEVC_NAL_UNIT_VPS = 32,	//32
	HEVC_NAL_UNIT_SPS = 33,	//33
	HEVC_NAL_UNIT_PPS = 34,	//34
	HEVC_NAL_FU_TYPE  = 49,
}hevc_nalu_type_e;
/*end for h264 and h265 define by 180732700, 20191113*/



pjmedia_vid_stream g_vid_stream;

static void on_rx_rtp(void *useData, void *pkt, pj_ssize_t bytes_read)
{
	//log_error("on_rx_rtp bytes_read:%d.", bytes_read);
	pjmedia_vid_stream *stream = (pjmedia_vid_stream *)useData;
	//rtp_packet_recv(stream, pkt, bytes_read);
	//calculation_video_rtp_loss

	int result = ringbuffer_write(stream->ringbuf, (unsigned char*)pkt, (unsigned)bytes_read);
	if(SEQ_DISCONTINUOUS == result) {
		//send nack
		LostedPackets *lp = &stream->ringbuf->lostPack;
		log_error("on_rx_rtp discontinuous lost package number:%d begin:%d end:%d\n", lp->lostTotal, lp->bgnSeq, lp->endSeq);

        char rtcp[100] = {0};
        pj_size_t size = 0;
        rtcp_build_rtcp_nack(rtcp, &size, lp->bgnSeq, lp->lostTotal);
        transport_send_rtcp(&stream->trans_udp, rtcp, (pj_uint32_t)size);
	}
}

static void on_rx_rtcp(void *useData, void *pkt, pj_ssize_t bytes_read)
{
    long start_time = get_currenttime_us();
	
	pj_size_t size = 0;
    char rtcp[100] = {0};
    
    pjmedia_rtcp_common *common = (pjmedia_rtcp_common*)pkt;
    pjmedia_rtcp_sr_pkt *sr = NULL;
    pjmedia_rtcp_rr_pkt *rr = NULL;
    pjmedia_rtcp_nack_pkg *nack = NULL;
    pjmedia_vid_stream  *stream = (pjmedia_vid_stream *)useData;
	log_debug("on_rx_rtcp bytes_read:%d package size:%d.", (int)bytes_read, common->length);

    switch(common->pt)
    {
        case RTCP_SR:
            sr = (pjmedia_rtcp_sr_pkt*)pkt;
            rtcp_build_rtcp_rr(rtcp, &size, sr->rr.lsr, (int)(get_currenttime_us()-start_time));
            if(size>0)
			{
                transport_send_rtcp(&stream->trans_udp, rtcp, (pj_uint32_t)size);
                log_debug("rtcp recv sr and send size:%d lsr:%d", size, sr->rr.lsr);
            }
            break;
            
        case RTCP_RR:
			rr = (pjmedia_rtcp_rr_pkt*)pkt;
			int rrtus = 0;
			if(stream->status_cb) {
				rrtus = (int)(get_currenttime_us() - rr->rr.lsr);
				stream->status_cb(NET_RTP_STATUS, rrtus - rr->rr.dlsr, stream->vframe_ps.frameRate, 0, 0);
			}
            log_debug("rtcp recv rr rtt time:%d ms lsr:%d us delay:%d us", rrtus/1000, rr->rr.lsr, rr->rr.dlsr );
            break;
            
        case RTCP_NACK:
            nack = (pjmedia_rtcp_nack_pkg *)pkt;
            unsigned base_seq 	= nack->nack.base_seq;
            unsigned count 		= nack->nack.flag;
            resend_losted_package(&stream->trans_udp, base_seq, count);
            log_debug("rtcp recv nack begin_seq:%d count:%d", base_seq, count);
            break;

		case RTCP_FIR:
			break;
    }
	//rtcp_nack_packet *nack = (rtcp_nack_packet*)pkt;
	//log_error("on_rx_rtcp recv number:%d startseq:%d endseq:%d\n", nack->number, nack->start_seq, nack->end_seq);
}

int stream_create(const char*localAddr, unsigned short localRtpPort, int codecType)
{
    int status = 0;
    
    memset(&g_vid_stream, 0, sizeof(pjmedia_vid_stream));
    
    g_vid_stream.codecType              = codecType;
    g_vid_stream.rtp_session.out_extseq = 0;
    g_vid_stream.rtp_session.out_pt     = (codecType==H264_HARD_CODEC)?RTP_PT_H264:RTP_PT_H265;
    g_vid_stream.fmt_id 				= (codecType==H264_HARD_CODEC)?PJMEDIA_FORMAT_H264:PJMEDIA_FORMAT_H265;
	g_vid_stream.vframe_slice 			= (codecType==H264_HARD_CODEC)?&get_h264_package:&get_h265_package;  //define in vid_codec.c
    
    status = ringbuffer_create(g_vid_stream.fmt_id, RESEND_SUPPORT, &g_vid_stream.ringbuf);
    
    memset(&g_vid_stream.trans_udp, 0, sizeof(transport_udp));
    g_vid_stream.trans_udp.user_stream  = &g_vid_stream;    //udp transport user data
    g_vid_stream.vid_port.useStream 	= &g_vid_stream;    //vid_port user data
    
    status = transport_udp_create(&g_vid_stream.trans_udp, localAddr, localRtpPort, &on_rx_rtp, &on_rx_rtcp);
    if(status<0) {
        log_error("transport_udp_create failed.");
        return status;
    }
    
    return status;
}

#ifdef SAVE_VIDEO_RAW
FILE* rvFp = NULL;
const char*filename = "/sdcard/raw_vid.h264";
#endif

RTC_API
int vid_stream_create(const char*localAddr, unsigned short localRtpPort, int codecType) {
	int status = 0;
    
	status = stream_create(localAddr, localRtpPort, codecType);

#ifdef SAVE_VIDEO_RAW
	rvFp = fopen(filename, "wb");
#endif

	return status;
}

RTC_API
int vid_stream_destroy() {
	int result = -1;
	transport_udp_destroy(&g_vid_stream.trans_udp);  	//release transport_udp* trans
    
	if (g_vid_stream.ringbuf) {
		ringbuffer_destory(g_vid_stream.ringbuf); 		//release IBaseRunLib* jb_opt
		g_vid_stream.ringbuf = NULL;
	}
    
    //release undecoded frame buffer
    //if(g_vid_stream.rtp_unpack_obj)
    //    CRtpPackDecoderDestroy(g_vid_stream.rtp_unpack_obj);
    
#ifdef SAVE_VIDEO_RAW
	if(rvFp) {
		fclose(rvFp);
		rvFp = NULL;
	}
#endif

	return result;
}


RTC_API
int vid_stream_network_callback(on_network_status net_cb) {
    g_vid_stream.status_cb = net_cb;
    return  0;
}

RTC_API
int vid_stream_recv_start_ios(on_rtp_frame frame_cb) 
{
	int status = -1;
	//for ios
    g_vid_stream.vid_port.rtp_cb = frame_cb;
	status = vid_port_start(&g_vid_stream.vid_port);
	status = transport_udp_start( &g_vid_stream.trans_udp);
	return status;
}

RTC_API
int vid_stream_recv_start_android(void* surface)
{
	int status = -1;
	#ifdef __ANDROID__
    g_vid_stream.vid_port.decoder = NULL;
    g_vid_stream.vid_port.surface = surface;
    #endif

	status = vid_port_start(&g_vid_stream.vid_port);
	status = transport_udp_start( &g_vid_stream.trans_udp);

	return status;
}

RTC_API
int vid_stream_recv_stop()
{
	int status = -1;
	status = transport_udp_stop( &g_vid_stream.trans_udp);
	status = vid_port_stop(&g_vid_stream.vid_port);
	return status;
}

RTC_API
int vid_stream_remote_connect(const char*remoteAddr, unsigned short remoteRtpPort)
{
	int status = -1;
	status = transport_udp_remote_connect(&g_vid_stream.trans_udp, remoteAddr, remoteRtpPort);
	return status;
}

RTC_API
int packet_and_send(char* frameBuffer, int frameLen) {
	#ifdef SAVE_VIDEO_RAW
		if(rvFp)
			fwrite(frameBuffer, 1, frameLen, rvFp);
    #endif
    return packet_and_send_(&g_vid_stream, frameBuffer, frameLen);
}

int packet_and_send_(pjmedia_vid_stream*stream, char* frameBuffer, int frameLen) 
{
	int result = -1, ext_len = 0, mark = 0;
	char rtpBuff[2000] = {0};
	pjmedia_frame package_out = {0};

	//one frame set the same timestamp
	long timestamp = get_currenttime_us();
	
	//first package add extend info
	char ori_ext = pjmedia_rtp_rotation_ext(90);
	ext_len = pjmedia_video_add_rtp_exten(rtpBuff + sizeof(pjmedia_rtp_hdr), 0xEA, ori_ext, frameLen);
	package_out.buf = rtpBuff + sizeof(pjmedia_rtp_hdr) + ext_len;

	/* Loop while we have frame to send */
	do
	{
		pj_status_t status = 0;
		//get one package
		status = stream->vframe_slice(frameBuffer, frameLen, &package_out);
		if(status<0) {
			//do sth
		}
		else
		{
			char *buffer = (char*)package_out.buf;
			if( (AVC_NAL_FU_TYPE_A==(*buffer & 0x1F)) && (*(buffer+1) & 0x40) && (package_out.enc_packed_pos<frameLen) )
			{
				//multi slice
				log_warn("multi slice_______");
				buffer[1] = buffer[1]&0xBF;
			}
		}
		//print log
		if(ext_len>0) {
			char *buffer = (char*)package_out.buf;
			log_warn("package_out.buf :%02x", buffer[0]);
		}
		
		//update head
		mark = !package_out.has_more;
		rtp_update_hdr(&stream->rtp_session, rtpBuff, mark, package_out.size, timestamp );//set rtp head parameter and out_extseq++
		pjmedia_rtp_hdr *hdr_data = (pjmedia_rtp_hdr*)rtpBuff;
		hdr_data->x = (ext_len>0)?1:0;

		//send one package
        result = transport_priority_send_rtp(&stream->trans_udp, rtpBuff, package_out.size + sizeof(pjmedia_rtp_hdr) + ext_len);
		
		if(package_out.enc_packed_pos>=frameLen)
			break;
		
		package_out.buf = rtpBuff + sizeof(pjmedia_rtp_hdr); //address point to payload
		ext_len = 0;

		usleep((result>0) ? 20 : 100);

	}while(1);

	return result;
}

