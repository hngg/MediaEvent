
#ifndef __PJMEDIA_VID_STREAM_H__
#define __PJMEDIA_VID_STREAM_H__


#include "types.h"
#include "rtp.h"
#include "rtcp.h"
#include "vid_port.h"
#include "vid_codec.h"

#include "jitter_buffer.h"
#include "transport_udp.h"


//#ifdef PJMEDIA_VIDEO_RESEND_OPTIMIZE

//#define RTP_PACKET_SEQ_MAP_MAX 17
//#define HEARTBREAK_RTP_PT 127
//#define TERMINAL_TYPE 5

#define RESEND_SUPPORT  1
//#define RESEND_ARR_MAX_NUM  64
//#define RESEND_HEARTBREAK_TYPE 306
//#define RESEND_HEARTBREAK_MAX_LEN 32
//#define RESEND_BREAKBEART_TIMES 3
//#define RESEND_TIMES_MAX 2
//#define RESEND_REQ_BASESEQ_BUFF_LEN 1024
//#define RESEND_BUFF_SIZE  1024


//#ifdef PJMEDIA_VIDEO_JBUF_OPTIMIZE
#define DISCARD_RESEND_NUM_MAX  30
#define DEC_FRAME_ARR_MAX       20
#define FRAME_LEN_MAX           720*1280*3/2
#define DELAY_FRAME_NUM         5
#define DELAY_FRAME_NUM_MAX     10
#define BEGIN_RENDER_FRAME_IDX  3
//#endif

//video framerate per second
typedef struct vframe_psecond
{
    long lastTimeMs;
    int  frameCount;
    int  frameRate;
}vframe_psecond;

typedef struct pjmedia_vid_stream
{
    transport_udp           trans_udp;
    
    int                     codecType;
    pjmedia_format_id       fmt_id;
//    unsigned                clock_rate;
//    uint64_t                first_pts;
//    float                   pts_ratio;
    pjmedia_rtp_session     rtp_session;
    pjmedia_vid_port        vid_port;
    RingBuffer*             ringbuf;            //Jitter buffer optimize.
    //void*                   rtp_unpack_buf;   //temp pointer, unpack frame buffer， point to pUnpack->stPreFrame.pFrameBuf
    //void*                   rtp_unpack_obj;   //had packed frame buffer,may thinking this as NAL frame
    
    vframe_psecond          vframe_ps;
    on_network_status       status_cb;          //network status callback
    on_vframe_slice         vframe_slice;       //video frame slice function

    //unsigned		        out_rtcp_pkt_size;
    char		            out_rtcp_pkt[PJMEDIA_MAX_MTU];  /**< Outgoing RTCP packet.	    */
    
}pjmedia_vid_stream;


int packet_and_send_(struct pjmedia_vid_stream*stream, char* frameBuffer, int frameLen);


#endif	/* __PJMEDIA_VID_STREAM_H__ */
