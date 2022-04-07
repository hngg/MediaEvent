

#include "rtcp.h"
#include "utils.h"
#include "os.h"
#include "glog.h"

pj_status_t rtcp_build_rtcp_sr(void *buf, pj_size_t *length)
{
    pjmedia_rtcp_sr_pkt *rtcp_pkt = (pjmedia_rtcp_sr_pkt*)buf;
    /* Init common RTCP SR header */
    rtcp_pkt->common.version  = 2;
    rtcp_pkt->common.count    = 1;
    rtcp_pkt->common.pt       = RTCP_SR;
    rtcp_pkt->common.length   = sizeof(struct pjmedia_rtcp_sr_pkt);
    rtcp_pkt->rr.lsr = (pj_uint32_t)get_currenttime_us();
    
    *length = sizeof(struct pjmedia_rtcp_sr_pkt);
    log_debug("rtcp sr genaner lsr:%d", rtcp_pkt->rr.lsr);
    return PJ_SUCCESS;
}

pj_status_t rtcp_build_rtcp_rr(void *buf, pj_size_t *length, int lsr, int dlsr)
{
    pjmedia_rtcp_rr_pkt *rtcp_pkt = (pjmedia_rtcp_rr_pkt*)buf;
    /* Init common RTCP SR header */
    rtcp_pkt->common.version  = 2;
    rtcp_pkt->common.count    = 1;
    rtcp_pkt->common.pt       = RTCP_RR;
    rtcp_pkt->common.length   = sizeof(struct pjmedia_rtcp_rr_pkt);
    rtcp_pkt->common.ssrc     = SPACAIL_SSRC;
    rtcp_pkt->rr.lsr  = lsr;
    rtcp_pkt->rr.dlsr = dlsr;
    
    *length = sizeof(struct pjmedia_rtcp_rr_pkt);
    
    return PJ_SUCCESS;
}

pj_status_t rtcp_build_rtcp_fir(void *buf, pj_size_t *length) {
    pjmedia_rtcp_common *rtcp_pkt = (pjmedia_rtcp_common*)buf;
    /* Init common RTCP SR header */
    rtcp_pkt->version  = 2;
    rtcp_pkt->count    = 1;
    rtcp_pkt->pt       = RTCP_FIR;
    rtcp_pkt->length   = sizeof(struct pjmedia_rtcp_common);
    rtcp_pkt->ssrc     = SPACAIL_SSRC;
    
    *length = rtcp_pkt->length;
    log_debug("rtcp fir genaner.");
    return PJ_SUCCESS;
}

pj_status_t rtcp_build_rtcp_nack(  void *buf, pj_size_t *length, unsigned begin_seq, unsigned seq_num)
{
    pjmedia_rtcp_nack_pkg *rtcp_pkt = (pjmedia_rtcp_nack_pkg *)buf;
    rtcp_pkt->common.version     = 2;
    rtcp_pkt->common.count       = 1;
    rtcp_pkt->common.pt          = RTCP_NACK;
    rtcp_pkt->common.length      = sizeof(pjmedia_rtcp_nack_pkg);
    rtcp_pkt->nack.base_seq      = begin_seq;
    rtcp_pkt->nack.flag          = seq_num;
    
    *length = rtcp_pkt->common.length;
    
    return PJ_SUCCESS;
}

