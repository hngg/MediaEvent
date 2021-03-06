


#include <time.h>
#include <stdint.h>
#include <stdio.h>

#include "basedef.h"
#include "glog.h"
#include "vid_codec.h"
#include "vid_stream_test.h"
#include "video_rtc_api.h"
#include "NALDecoder.h"

extern pjmedia_vid_stream g_vid_stream;

//-----------------------------------------------------------------------testing-----------------------------------------------------------------------------
#ifndef __ANDROID__
#ifndef TARGET_OS_IPHONE

int gCount = 0;
int rtp_packet_recv_264(pjmedia_vid_stream*stream, char* packetBuffer, int packetLen) {

	unsigned pk_len = 0;
	int status = 0, frame_len;
	pj_int8_t rtpPack[PJMEDIA_MAX_MTU] = {0};
	
	//calculation_video_rtp_loss
	ringbuffer_write(stream->ringbuf, (pj_uint8_t*)packetBuffer, packetLen);

	//pk_len = readPacketfromSub(stream->jb_opt, rtpPack, stream->fmt_id);
	ringbuffer_read(stream->ringbuf, rtpPack, &pk_len);
	if(pk_len>0) {
		frame_len = RTPUnpackH264(stream->ringbuf->rtp_unpack_obj, (char*)rtpPack, pk_len, &stream->ringbuf->rtp_unpacked_buf);
		if(frame_len > 0)
		{
			gCount++;
			log_error("RTPUnpackH264 count:%d frame_len:%d\n", gCount, frame_len);
			// if(fp) {
			// 	fwrite(stream->rtp_unpack_buf, 1, frame_len, fp);
			// 	fflush(fp);
			// }
		}
	}

	return status;
}

int rtp_packet_and_unpack_notnet_test(pjmedia_vid_stream*stream, char* frameBuffer, int frameLen) {
	int result = -1, ext_len = 0, mark = 0;
	char rtpBuff[2000] = {0};
	pjmedia_frame package_out = {0};

	long timestamp = get_currenttime_us();
	char ori_ext = pjmedia_rtp_rotation_ext(90);
	ext_len = pjmedia_video_add_rtp_exten(rtpBuff + sizeof(pjmedia_rtp_hdr), 0xEA, (pj_uint8_t*)&ori_ext, sizeof(pj_uint16_t));
	package_out.buf = rtpBuff + sizeof(pjmedia_rtp_hdr) + ext_len;

	/* Loop while we have frame to send */
	do
	{
		pj_status_t res = get_h264_package(frameBuffer, frameLen, &package_out);
        UNUSED(res);
		mark = !package_out.has_more;
		rtp_update_hdr(&stream->rtp_session, rtpBuff, mark, package_out.size, timestamp );
		pjmedia_rtp_hdr *hdr_data = (pjmedia_rtp_hdr*)rtpBuff;
		hdr_data->x = (ext_len>0)?1:0;

		result = rtp_packet_recv_264(stream, rtpBuff, package_out.size + sizeof(pjmedia_rtp_hdr) + ext_len);
		if(package_out.enc_packed_pos>=frameLen)
			break;
		package_out.buf = rtpBuff + sizeof(pjmedia_rtp_hdr);
		ext_len = 0;

	}while(1);

	return result;
}


int stream_send_test(const char *localAddr, unsigned short localPort, const char*remoteAddr, unsigned short remotePort, const char*sendFile, int codecType) {
	int status = vid_stream_create(localAddr, localPort, codecType);//local
	 if(status>=0)
	 	vid_stream_remote_connect( remoteAddr, remotePort);//remote
	getchar();
#ifdef SENDTEST
	//vid_stream_test_file(sendFile);
#endif
	getchar();
	vid_stream_destroy();
	log_error("vid_stream_stop done.");
	getchar();
	vid_stream_destroy();
	log_error("vid_stream_destroy done.");
	return status;
}

int stream_recv_test(const char *localAddr, short localPort, int codecType) {
	int status = 0;
	status = vid_stream_create(localAddr, localPort, codecType);//local
	if(status>=0)
		vid_stream_remote_connect( localAddr, localPort);//remote

	getchar();
	vid_stream_destroy();
	log_error("vid_stream_stop done.");
	getchar();
	vid_stream_destroy();
	log_error("vid_stream_destroy done.");
	return status;
}

int stream_send_rtcp_test(const char *localAddr, short localPort, const char*remoteAddr,  short remotePort, int codecType) 
{
	int status = vid_stream_create(localAddr, localPort, codecType);//local
	if(status>=0)
		vid_stream_remote_connect( remoteAddr, remotePort);//remote
	getchar();
#ifdef SENDTEST
//		char buffer[PJMEDIA_MAX_MTU] = {0};
//		rtcp_nack_packet *nack = (rtcp_nack_packet*)buffer;
//		nack->number 	= 2;
//		nack->start_seq = 10000;
//		nack->end_seq	= 10001;
//		transport_send_rtcp(g_vid_stream.trans, buffer, sizeof(rtcp_nack_packet));
//		log_error("on_rx_rtp resend rtcp length:%d\n", sizeof(rtcp_nack_packet));
#endif
	getchar();
	vid_stream_destroy();
	log_error("vid_stream_stop done.");
	getchar();
	vid_stream_destroy();
	log_error("vid_stream_destroy done.");
	return status;
}

void rtp_packet_and_unpack_notnet_test_264(pj_uint8_t *frame, pj_size_t frame_len) {
	rtp_packet_and_unpack_notnet_test(&g_vid_stream, frame, frame_len);
}

void packet_and_send_test(pj_uint8_t *bits, pj_size_t bits_len) {
	//vid_stream_create();
	packet_and_send_(&g_vid_stream, bits, bits_len);
}

void h264_package_test(pj_uint8_t *bits, pj_size_t bits_len) {

	pj_uint8_t payload[PJMEDIA_MAX_VID_PAYLOAD_SIZE] = {0};
	pjmedia_frame package_out = {0};
	package_out.buf = payload;
	while(package_out.enc_packed_pos<bits_len) {
		pj_status_t res = get_h264_package(bits, bits_len, &package_out);
		log_debug("------get size:%d payload_len:%d enc_packed_pos:%d indicator:%02X res:%d\n", 
			(int)bits_len, (int)package_out.size, package_out.enc_packed_pos, payload[1], PJMEDIA_MAX_VID_PAYLOAD_SIZE);
	}
}

#endif //TARGET_OS_IPHONE
#endif //__ANDROID__





// void get_file_exten(const char*fileName, char*outExten) {
//     int i=0,length= strlen(fileName);
//     while(fileName[i]) {
//         if(fileName[i]=='.')
//             break;
//             i++;
//     }
//     if(i<length)
//         strcpy(outExten, fileName+i+1);
//     else
//         strcpy(outExten, "\0");
// }

// void vid_stream_test_file(char*filename) {
//     FILE			*pFile;
// 	NALU_t 			*mNALU;
//     char idrBuf[2*1024*1024] = {0};
// 	pFile = OpenBitstreamFile( filename ); //camera_640x480.h264 //camera_1280x720.h264
// 	if(pFile==NULL) {
// 		GLOGE("open file:%s failed.", filename);
// 		return ;
// 	}

//     //vid_stream_create();

//     char extenName[12] = {0};
//     get_file_exten(filename, extenName);
//     GLOGE("filename exten:%s\n", extenName);

//     int frameType = 0;
//     if(0==strcmp("h265", extenName));
//         frameType = H265_FRAME;
//     if(0==strcmp("h264", extenName))
//         frameType = H264_FRAME;

// 	mNALU  = AllocNALU(8000000);

// 	int count = 0, cpyPos = 0;
// 	do {
// 		count++;
// 		int size=GetAnnexbNALU(pFile, mNALU);//??????????????????????????????????????????????????????NALU???????????????????????????????????????NALU????????????0x000001
// 		GLOGE("GetAnnexbNALU type:0x%02X size:%d count:%d\n", mNALU->buf[4], size, count);
// 		if(size<4) {
// 			GLOGE("get nul error!\n");
// 			continue;
// 		}

//         int type = mNALU->buf[4] & 0x1F;
//         if(type==7 || type==8) {
//             memcpy(idrBuf+cpyPos, mNALU->buf, size);
//             cpyPos += size;
//             continue;
//         }

//         memcpy(idrBuf+cpyPos, mNALU->buf, size);
//         cpyPos += size;
// 		//GLOGE("GetAnnexbNALU frame size:%d\n", cpyPos);

//         switch(frameType) {
//             case H264_FRAME:
//                 //h264_test(mNALU->buf, size);
//                 //h264_package_test(idrBuf, cpyPos);
//                 packet_and_send_test(idrBuf, cpyPos);
// 				//rtp_packet_recv_test(idrBuf, cpyPos);
//                 break;
//             case H265_FRAME:
//                 h265_test(mNALU->buf, size);
//                 break;
//         }
//         cpyPos = 0;
// 	}while(!feof(pFile));

// 	if(pFile != NULL)
// 		fclose(pFile);

// 	FreeNALU(mNALU);

//     //vid_stream_destroy();
// }














// #ifdef PJMEDIA_VIDEO_RESEND_OPTIMIZE
// static void resend_rtp(pjmedia_vid_stream *stream, pjmedia_vid_buf  *resend, pj_uint16_t pid_val,pj_uint16_t blp_val, pj_bool_t standard);

// static void create_and_send_heartbreak(pjmedia_vid_stream *stream,void *rtphdr);

// static pj_status_t send_rtcp_resend(pjmedia_vid_stream *stream, pj_uint16_t flag);
// #endif

// /* add by j33783 20190805 */
// static pj_status_t send_rtcp_fir(pjmedia_vid_stream *stream)
// {
// 	pj_status_t status = PJ_FALSE;
// 	pj_time_val tv;
// 	pj_uint64_t ts = 0;

// 	pj_gettimeofday(&tv);

// 	ts = tv.sec*1000 + tv.msec;
// 	if((ts - stream->send_fir_ts ) >= 500)
// 	{
// 		status = send_rtcp_resend(stream,RTCP_SDES_FALG | RTCP_FIR_FALG);
// 		log_debug("send RTCP FIR packet status:%d", status);
// 		stream->send_fir_ts = ts;
// 	}

// 	return status;
// }


// #ifdef PJMEDIA_VIDEO_RESEND_OPTIMIZE
// // \B7\A2\CB\CDrtcp\BF\D8\D6\C6????\BD??\DA
// static void SendRtcpNackPacket(pjmedia_vid_stream *stream, pjmedia_resend_basereq_info*base_req_arr,pj_uint16_t usLastSeq, pj_uint16_t usCurSeq)
// {
// 	// \C5????????\C4\D0\F2\C1??\C5\D3\D0??\D0\D4
// 	int nSeqNum = usCurSeq - (usLastSeq + 1);
// 	if (nSeqNum <= 0)
// 	{
// 		return;
// 	}
// 	pjmedia_vid_channel *dec = stream->dec;

// 	//\BC\C7??\C6\F0??\B6\AA??\B5\C4\D0\F2\C1??\C5
// 	unsigned short usSeq = usLastSeq + 1;

// 	// \D7\EE\B6\E0??\C4??\A2\CB\CD16\B8\F6\D0\F2\C1??\C5????
// 	if (nSeqNum > RTP_PACKET_SEQ_MAP_MAX)
// 	{
// 		nSeqNum = RTP_PACKET_SEQ_MAP_MAX;
// 	}

// 	// \C9???\D0\F2\C1??\C5????
// 	unsigned short usSeqMap = 0;
// 	int i;
// 	for (i = 0; i < nSeqNum - 1; i ++)
// 	{
// 		usSeqMap += (1 << i);
// 	}


// 	int idx = usSeq % RESEND_REQ_BASESEQ_BUFF_LEN;
// 	if(base_req_arr[idx].base_seq != usSeq || base_req_arr[idx].seq_map != usSeqMap){
// 	//refresh the base_seq and seq_map
// 		base_req_arr[idx].base_seq = usSeq;
// 		base_req_arr[idx].req_times = 0;
// 		base_req_arr[idx].seq_map = usSeqMap;
// 	}else if(base_req_arr[idx].req_times > RESEND_TIMES_MAX){
// 	   //if the times of resend req is more than RESEND_TIMES_MAX, then return
// 		return;
// 	}
// 	dec->base_seq = usSeq;
// 	dec->flag = usSeqMap;
// 	base_req_arr[idx].req_times++;
// 	send_rtcp_resend(stream,RTCP_SDES_FALG | RTCP_NACK_FALG);

// 	return;
// }


// //\BC\EC\B2???\B0\FC\A3\AC\B2\A2\B7\A2\CB\CDRTCP\B0\FC\B8\F8\B7\A2\CB??\CB
// //void OnRtpLostHandleCallBack(void* rri, void * handle)
// static void resend_req_proc(pjmedia_vid_stream *stream)
// {
// 	BASE_MEDIA_CACHE_CH *pmci;
// 	if(stream == NULL || stream->jb_opt == NULL) return;
// 	pjmedia_resend_basereq_info base_req_arr[RESEND_REQ_BASESEQ_BUFF_LEN];
// 	memset(base_req_arr, 0x00, sizeof(base_req_arr));
// 	pmci = &(stream->jb_opt->catch_info); 
// 	pmci->lastRtcpTime = GetCurrentMsec();
// 	pjmedia_vid_channel *dec = stream->dec;

// 	/* begin, delete by l27913, 2019.12.24 */
// 	//pj_thread_sleep(100);
// 	/* end */

// 	while(stream->jbuf_stop !=  PJ_TRUE){
// 		// \B6\A8??\B7\A2\CB\CDrtcp\BF\D8\D6??\FC, 50ms??\D3\D0\CA??\BD\A3\AC\D6??\A2
// 		if (GetCurrentMsec() - pmci->lastRtcpTime > _max_resend_wait_time)
// 		{
// 			// \B6\A8\D2\E5\BB\F1??\B6\AA??\B5??\FC\CA\FD\D7\E9
// 			int arrSeq[RTCP_RESEND_ARR_MAX_NUM];
// 			memset(arrSeq, 0x00, sizeof(arrSeq));
// 			// \BB\F1??\D0\E8??\D6??\AB\B5??\FC
// 			int nCount = getLostPacket(stream->jb_opt, pmci, arrSeq, RTCP_RESEND_ARR_MAX_NUM,  _max_resend_wait_time,  _max_lost_wait_time);
// 			if (nCount > 0)
// 			{
// 				int i;			
// 				for (i = 0; i < nCount; i++)
// 				{
// 					// \BB\F1??\BF\AA??\D0\F2\C1??\C5
// 					unsigned short  nSeqBgn = (unsigned short)(arrSeq[i] >> 16);
// 					// \BB\F1??\BD\E1\CA\F8
// 					unsigned short  nSeqEnd = (unsigned short)(arrSeq[i] & 0xffff);//the end seq mask

// 					//\BC\C7??\C6\F0??\B6\AA??\B5\C4\D0\F2\C1??\C5	
// 					unsigned short usSeq = nSeqBgn;
// 					// \C9???\D0\F2\C1??\C5????
// 					unsigned short usSeqMap = 0;

// 					//??\D3??\C8\D3\DA17\A3\AC\D4??\B1??\B9\B9\D4\ECseq??\C9\E4
// 					if ((nSeqEnd - nSeqBgn < RTP_PACKET_SEQ_MAP_MAX) && (nSeqBgn <= nSeqEnd))
// 					{
// 						// \B9\B9\D4\EC??\C9\E4
// 						if(nSeqBgn < nSeqEnd){
// 							int nMap;
// 							for (nMap = nSeqBgn+1; nMap <= nSeqEnd; nMap++)
// 							{
// 								usSeqMap += (1 << (nMap - nSeqBgn - 1));
// 							}
// 						}

// 						// \BA??\A2\BF\C9\D2\D4\C8????\C4seqmap
// 						if(i < nCount - 1){
// 							int j;
// 							for (j = i + 1; j < nCount; j++)
// 							{
// 								// \BB\F1??\BF\AA??\D0\F2\C1??\C5
// 								unsigned short  nSeqBgn2 = (unsigned short)(arrSeq[j] >> 16);
// 								// \BB\F1??\BD\E1\CA\F8
// 								unsigned short  nSeqEnd2 = (unsigned short)(arrSeq[j] & 0xffff);

// 								// \C5??\CF\D3\D0??\D0\D4
// 								if ((nSeqEnd2 - nSeqBgn < RTP_PACKET_SEQ_MAP_MAX) && (nSeqEnd < nSeqBgn2))
// 								{
// 									// \B9\B9\D4\EC??\C9\E4
// 									int nMap;
// 									for (nMap = nSeqBgn2; nMap <= nSeqEnd2; nMap++)
// 									{
// 										usSeqMap += (1 << (nMap - nSeqBgn - 1));
// 									}

// 									// \B8\B3??i =j
// 									i = j;

// 								}
// 								// \C8\E7\B9\FB\B4\F3\D3\DA17\CD??\F6\B5\B1????\BB\B7
// 								else
// 								{
// 									break;
// 								}

// 							}
// 						}

// 						// \B7\A2\CB\CDrtcp\B7\B4\C0\A1\B0\FC
// 						int idx = usSeq % RESEND_REQ_BASESEQ_BUFF_LEN;
// 						if(base_req_arr[idx].base_seq != usSeq || base_req_arr[idx].seq_map != usSeqMap){
// 							//refresh the base_seq and seq_map
// 							base_req_arr[idx].base_seq = usSeq;
// 							base_req_arr[idx].req_times = 0;
// 							base_req_arr[idx].seq_map = usSeqMap;
// 						}else if(base_req_arr[idx].req_times > 2){
// 						 //if the times of resend req is more than RESEND_TIMES_MAX, then return
// 							continue;
// 						}
// 						dec->base_seq = usSeq;
// 						dec->flag = usSeqMap;
// 						base_req_arr[idx].req_times++;
// 						log_debug("check_lost_rtp_proc_cycle  dec->base_seq=%d,dec->flag=%d",dec->base_seq,dec->flag);
// 						send_rtcp_resend(stream,RTCP_SDES_FALG | RTCP_NACK_FALG);
// 					}
// 					else
// 					{
						
// 						//\B7\D6\C5\FA\B7\A2\CB\CD
// 						if(nSeqEnd - nSeqBgn > DISCARD_RESEND_NUM_MAX)break;
// 						int nMap = 0;
// 						for (nMap = nSeqBgn; nMap <= (nSeqEnd - RTP_PACKET_SEQ_MAP_MAX); )
// 						{
// 							// \BB\F1??\BD\E1\CA\F8\D0\F2\C1??\C5
// 							SendRtcpNackPacket(stream,base_req_arr,nMap - 1, nMap + RTP_PACKET_SEQ_MAP_MAX);
// 							// map \BC\D317
// 							nMap += RTP_PACKET_SEQ_MAP_MAX;
// 						}

// 						// \BB\F1??\BD\E1\CA\F8\D0\F2\C1??\C5
// 						if (nMap < nSeqEnd)
// 						{
// 							SendRtcpNackPacket(stream,base_req_arr,nMap - 1, nSeqEnd + 1);
// 						}
// 					}

// 				}
// 			}

// 		}
// 		pj_thread_sleep(_max_resend_wait_time);

// 		}
// }


// #ifdef PJMEDIA_VIDEO_RESEND_OPTIMIZE
// static pj_status_t send_rtcp_resend(pjmedia_vid_stream *stream,
// 			     pj_uint16_t flag)
// {
// 	void *sr_rr_pkt;
// 	pj_uint8_t *pkt;
// 	int len, max_len;
// 	pj_status_t status;

// 	/* Build RTCP RR/SR packet */
// //	  pjmedia_rtcp_build_rtcp(&stream->rtcp, &sr_rr_pkt, &len);
// 	pjmedia_rtcp_build_rtcp_rr(&stream->rtcp, &sr_rr_pkt, &len);

// 	if((flag & RTCP_SDES_FALG) != 0 || (flag & RTCP_BYTE_FALG) != 0)
// 	{ 
// 		pkt = (pj_uint8_t*) stream->out_rtcp_pkt;
// 		pj_memcpy(pkt, sr_rr_pkt, len);
// 		max_len = stream->out_rtcp_pkt_size;
// 	} 
// 	else 
// 	{
// 		pkt = (pj_uint8_t*)sr_rr_pkt;
// 		max_len = len;
// 	}
	
// 	log_debug("max_len=%d, out_rtcp_pkt_size=%d,len=%d", max_len,stream->out_rtcp_pkt_size,len);

// 	/* Build RTCP SDES packet */
// 	if((flag & RTCP_SDES_FALG) != 0)
// 	{	
// 		pjmedia_rtcp_sdes sdes;
// 		pj_size_t sdes_len;

// 		pj_bzero(&sdes, sizeof(sdes));
// 		sdes.cname = stream->cname;
// 		sdes_len = max_len - len;
// 		status = pjmedia_rtcp_build_rtcp_sdes(&stream->rtcp, pkt+len,	  &sdes_len, &sdes);
// 		if (status != PJ_SUCCESS)
// 		{
// 			log_error("Error generating RTCP SDES, status:%d", status);
// 		} 
// 		else 
// 		{
// 			len += (int)sdes_len;
// 		}
		
// 		 log_debug("rtcp sdes packet, max_len=%d, sdes_len=%d,len=%d", max_len,sdes_len,len);
// 	}

// /* Build RTCP NACK packet */
// 	if((flag & RTCP_NACK_FALG) != 0)
// 	{
// 		pjmedia_rtcp_nack nack;
// 		pj_size_t nack_len;
// 		pj_bzero(&nack_len, sizeof(nack_len));
// 		nack.ssrc = pj_htonl(stream->rtcp.peer_ssrc);
// 		nack.base_seq = pj_htons(stream->dec->base_seq);
// 		nack.flag = pj_htons(stream->dec->flag);
// 		nack_len = max_len - len;
// 		log_debug("rtcp nack pakcet, ssrc=%x, nack_len=%d, base_seq=%d, flag=%d", 
// 					stream->rtcp.peer_ssrc, nack_len,nack.base_seq,nack.flag);
// 		status = pjmedia_rtcp_build_rtcp_nack(&stream->rtcp, pkt+len, &nack_len, &nack);
// 		if (status != PJ_SUCCESS) 
// 		{
// 			log_error("Error generating RTCP NACK, status:%d", status);
// 		} 
// 		else 
// 		{
// 			len += (int)nack_len;
// 		}
// 		log_debug("rtcp nack packet, len=%d, nack_len=%d, status=%d", len,nack_len,status);
// 	}
	
// 	/* Build RTCP BYE packet */
// 	if((flag & RTCP_BYTE_FALG) != 0)
// 	{
// 		pj_size_t bye_len;

// 		bye_len = max_len - len;
// 		status = pjmedia_rtcp_build_rtcp_bye(&stream->rtcp, pkt+len,	 &bye_len, NULL);
// 		if (status != PJ_SUCCESS) 
// 		{
// 			log_error("Error generating RTCP BYE, status:%d", status);
// 		}
// 		else
// 		{
// 			len += (int)bye_len;
// 		}	
// 		log_debug("rtcp bye packet, len=%d, max_len=%d,bye_len=%d", len,max_len,bye_len);
// 	}

// 	/* add by j33783 20190805 Build RTCP FIR packet */
// 	if((flag & RTCP_FIR_FALG) != 0)
// 	{
// 		pj_size_t bye_len;

// 		bye_len = max_len - len;
// 		status = pjmedia_rtcp_build_fir(&stream->rtcp, pkt+len, &bye_len);
// 		if (status != PJ_SUCCESS) 
// 		{
// 			log_error("Error generating RTCP FIR, status:%d", status);
// 		}
// 		else
// 		{
// 			len += (int)bye_len;
// 		}	
// 		log_debug("rtcp fir packet, len=%d, max_len=%d,bye_len=%d", len,max_len,bye_len);
// 	}

// 	/* Send! */
// 	status = pjmedia_transport_send_rtcp(stream->transport, pkt, len);

// 	return status;
// }
// #endif

// static pj_status_t send_rtcp(pjmedia_vid_stream *stream,
// 			     pj_bool_t with_sdes,
// 			     pj_bool_t with_bye)
// {
//     void *sr_rr_pkt;
//     pj_uint8_t *pkt;
//     int len, max_len;
//     pj_status_t status;

//     /* Build RTCP RR/SR packet */
//     pjmedia_rtcp_build_rtcp(&stream->rtcp, &sr_rr_pkt, &len);

//     if (with_sdes || with_bye) 
//     {
// 	 pkt = (pj_uint8_t*) stream->out_rtcp_pkt;
// 	 pj_memcpy(pkt, sr_rr_pkt, len);
// 	 max_len = stream->out_rtcp_pkt_size;
//     } 
//     else 
//     {
// 	 pkt = (pj_uint8_t*)sr_rr_pkt;
// 	 max_len = len;
//     }

//     /* Build RTCP SDES packet */
//     if (with_sdes) 
//    {
// 	pjmedia_rtcp_sdes sdes;
// 	pj_size_t sdes_len;

// 	pj_bzero(&sdes, sizeof(sdes));
// 	sdes.cname = stream->cname;
// 	sdes_len = max_len - len;
// 	status = pjmedia_rtcp_build_rtcp_sdes(&stream->rtcp, pkt+len, &sdes_len, &sdes);
// 	if (status != PJ_SUCCESS) 
// 	{
// 		log_error("Error generating RTCP SDES, status:%d",status);
// 	} 
// 	else 
// 	{
// 	len += (int)sdes_len;
// 	}
//     }

//     /* Build RTCP BYE packet */
//     if (with_bye) 
//    {
// 	pj_size_t bye_len;
// 	bye_len = max_len - len;
// 	status = pjmedia_rtcp_build_rtcp_bye(&stream->rtcp, pkt+len,  &bye_len, NULL);
// 	if (status != PJ_SUCCESS) 
// 	{
// 		log_error("Error generating RTCP BYE, status:%d",status);
// 	} 
// 	else 
// 	{
// 	    len += (int)bye_len;
// 	}
//      }

//     /* Send! */
//     status = pjmedia_transport_send_rtcp(stream->transport, pkt, len);

//     return status;
// }


// /**
//  * check_tx_rtcp()
//  *
//  * This function is can be called by either put_frame() or get_frame(),
//  * to transmit periodic RTCP SR/RR report.
//  */
// static void check_tx_rtcp(pjmedia_vid_stream *stream, pj_uint32_t timestamp)
// {
//     /* Note that timestamp may represent local or remote timestamp, 
//      * depending on whether this function is called from put_frame()
//      * or get_frame().
//      */


//     if (stream->rtcp_last_tx == 0)
//     {	
// 	stream->rtcp_last_tx = timestamp;
//     } 
//    else if (timestamp - stream->rtcp_last_tx >= stream->rtcp_interval) 
//    {
// 	pj_status_t status;
	
// 	status = send_rtcp(stream, !stream->rtcp_sdes_bye_disabled, PJ_FALSE);
// 	if (status != PJ_SUCCESS) 
// 	{
// 	    log_error("Error sending RTCP, status:%d", status);
// 	}

// 	stream->rtcp_last_tx = timestamp;
//     }
// }

// #ifdef PJMEDIA_VIDEO_RESEND_OPTIMIZE 
// static void resend_rtp(pjmedia_vid_stream *stream, pjmedia_vid_buf  *resend, pj_uint16_t pid_val,pj_uint16_t blp_val,pj_bool_t standard){
//        unsigned blp_bit_idx, num_max;
// 	pj_uint16_t    rtp_seq, rtp_pkt_len, index;
// 	pj_status_t status = 0;
// 	void *rtp_pkt;
// 	if(standard){
// 		num_max = 17;
// 	}else{
// 		num_max = 16;
// 	}
// 	for(blp_bit_idx = 0; blp_bit_idx < num_max; blp_bit_idx++){
// 		if((blp_bit_idx == 0 && standard) || (blp_val & 0x01) != 0){
// 			 rtp_seq = pid_val + blp_bit_idx;
// 			 index = rtp_seq % resend->buf_size;
// 			 if(resend->resend_times[index] < RESEND_TIMES_MAX){
// 			 	 resend->resend_times[index]++;
// 				 rtp_pkt = resend->buf + index * PJMEDIA_MAX_MTU;
// 				 rtp_pkt_len = resend->pkt_len[index];
// 				log_debug("rtp_pkt_len=%d,rtp_seq=%d",rtp_pkt_len,rtp_seq);
// 				 /* Resend the RTP packet to the transport. */
// 				 if(rtp_pkt && rtp_pkt_len > 0){
// 					 status = pjmedia_transport_priority_send_rtp(stream->transport,
// 							 (char*)rtp_pkt,rtp_pkt_len);
// 					 if(status != PJ_SUCCESS){
// 						 pjmedia_transport_priority_send_rtp(stream->transport,
// 								 (char*)rtp_pkt,rtp_pkt_len);
// 					 }
// 				}
// 			 }
// 		}
// 		if(standard){
// 			if(blp_bit_idx >0) blp_val >>= 1;
// 		}else{
// 			blp_val >>= 1;
// 		}
// 	}
// }

// static void create_and_send_heartbreak(pjmedia_vid_stream *stream,void *rtphdr){
// 	int mlen = 0;
// 	unsigned idx;
// 	pj_status_t status = 0;
// 	char mbuff[RESEND_HEARTBREAK_MAX_LEN];
// 	memset(mbuff,0x00,sizeof(mbuff));
// 	pj_memcpy(mbuff,rtphdr,sizeof(pjmedia_rtp_hdr));
// 	mlen = sizeof(pjmedia_rtp_hdr);
// 	*(int *)(mbuff + mlen) = htonl(RESEND_HEARTBREAK_TYPE);
// 	mlen += sizeof(pj_uint32_t);
// 	*(int *)(mbuff + mlen )= htonl(0);
// 	mlen += sizeof(pj_uint32_t);
// 	*(pj_uint16_t *)(mbuff + mlen) =  htons(0);
// 	mlen += sizeof(pj_uint16_t);
// 	*(pj_uint16_t *)(mbuff + mlen) =  htons(5);
// 	mlen += sizeof(pj_uint16_t);
// 	((pjmedia_rtp_hdr *) mbuff)->pt = HEARTBREAK_RTP_PT; 
	
// 	for(idx = 0; idx < RESEND_BREAKBEART_TIMES;idx++){
// 		status = pjmedia_transport_send_rtp(stream->transport,
// 				(char*)mbuff,mlen);
// 		if(status != PJ_SUCCESS){
// 			pjmedia_transport_send_rtp(stream->transport,
// 					(char*)mbuff,mlen);
// 		}
// 	}
	
// } 

// static pj_status_t check_rtp_nack_resend(pjmedia_vid_stream *stream,
//                         void *pkt, pj_ssize_t pkt_size){
// 	pj_status_t status = 0;
//     pj_uint8_t *p,blp_bit_idx,*pt, type;
//     pj_uint16_t rtp_seq,index,*p16,blp_val,pid_val,rtcp_len;
// 	unsigned rtp_pkt_len;
// 	unsigned rtcp_count = 0;
// 	void *rtp_pkt;
//      pj_bool_t nack_found = PJ_FALSE;
//     pj_uint32_t *nack_ptr; 
// 	pjmedia_vid_buf  *resend;

//    if(((pjmedia_rtp_hdr*)pkt)->pt == 127 && pkt_size > 12 && pkt_size <= 24){
// 	    p = (pj_uint8_t*)pkt;
// 	    p += 12;
// 	    nack_ptr = (pj_uint32_t *)p;
// 	    log_debug(" ntohs(*nack_ptr) =%d ",ntohl(*nack_ptr));
// 	    if(ntohl(*nack_ptr) == RTCP_NACK){

// 		//if(nack_found){
// 			p += 8 ;
// 			p16 = (pj_uint16_t*)p;
// 			pid_val = ntohs(*p16);
// 			p16++;
// 			blp_val = ntohs(*p16);

// 			//if(pid_val || blp_val){ 
// 			resend = stream->enc->resend_vbuf;
// 			resend_rtp(stream,resend,pid_val,blp_val,1); 
// 			return PJ_SUCCESS; // NACK
// 	    		//}
// 	    	}else{
// 	    	       return 5;//NOT NACK
// 		}
		
//    	}else {
// 		return 5;//NOT NACK
// 	}
   	
// }
// #endif


// static void calculation_video_rtp_loss(pjmedia_vid_stream *stream, const pjmedia_rtp_hdr *hdr)
// {
//     pj_uint16_t now_seq = pj_ntohs(hdr->seq);
   
//     if(!stream || !hdr)
//     {
//         log_error("the stream or hdr is NULL");
//         return;
//     }

// 	//log_debug("rtp seq:%d, mark:%d, loss_rtp:%d,all_rtp:%d",
// 	//	now_seq,hdr->m, stream->loss_rtp,stream->all_rtp);

// 	if((hdr->m && 0 == stream->last_seq) || (now_seq<stream->now_seq))
// 	{
// 	    return;
// 	}

//     if(0 == stream->last_seq)
//     {
//         stream->last_seq = now_seq;
// 		stream->start_seq = now_seq;
// 		stream->now_seq = now_seq;
// 		stream->all_rtp = 0;
// 		stream->loss_rtp = 0;
// 		return;
//     }

// 	if(now_seq - stream->last_seq>1)
// 	{
// 	    stream->loss_rtp += (now_seq - stream->last_seq - 1);
// 	    log_debug("loss video packet,last seq:%d, new seq:%d, loss rtp count:%d",
// 			stream->last_seq,now_seq,stream->loss_rtp);
// 	}

// 	stream->last_seq = now_seq;
	
// 	if(hdr->m)
// 	{
// 	     stream->last_seq = 0;
// 		 stream->all_rtp = now_seq - stream->start_seq + 1;
// 		 log_debug("after rtp loss_rtp:%d,all_rtp:%d",stream->loss_rtp,stream->all_rtp);
// 	}
// 	stream->now_seq = now_seq;
// }


// /*
//  * This callback is called by stream transport on receipt of packets
//  * in the RTP socket. 
//  */
// static void on_rx_rtp_process( void *data, void *pkt, pj_ssize_t bytes_read)

// {
//     pjmedia_vid_stream *stream = (pjmedia_vid_stream*) data;
//     pjmedia_vid_channel *channel = stream->dec;
//     const pjmedia_rtp_hdr *hdr = (pjmedia_rtp_hdr*)pkt;

// 	/* Check for errors and Ignore keep-alive packets */
// 	if ((bytes_read < 0) || (bytes_read < (pj_ssize_t) sizeof(pjmedia_rtp_hdr))) 
// 	{
// 		log_error("name:%.*s, RTP recv() error, bytes_read:%d", 
// 			channel->port.info.name.slen,
// 			channel->port.info.name.ptr, bytes_read);
// 		return;
// 	}

//     if (hdr->pt == HEARTBREAK_RTP_PT)	
// 	{		
// 		int nRHeartId = pj_ntohl(*((int*)pkt+3));
// 		if (307 == nRHeartId) {
// 			unsigned int unSeq = pj_ntohl(*((int*)pkt+4));
// 			unsigned int unJamAskTime = pj_ntohl(*((int*)pkt+5));
			
//             pj_int32_t downlink_bitrate=0; // kbps
//             float pLostRate=0.0000;
//             float pFrameRate=0.00;

//             if (stream->jb_opt) {
//             	pjmedia_jbuf_packet_recv_report(stream->jb_opt, &downlink_bitrate, &pLostRate, &pFrameRate); // from jbuf
//             }
// 			create_and_send_317(stream,hdr,unSeq,pLostRate,unJamAskTime);
// 		}
// 	}

//     /* add by t35599*/
//     if (stream->cctrl && stream->cc_irtp) 
// 	{
//        stream->cc_irtp(stream->cctrl, pkt, sizeof(pjmedia_rtp_hdr), bytes_read - sizeof(pjmedia_rtp_hdr));
//     }
//     /* end */

// #ifdef PJMEDIA_VIDEO_RESEND_OPTIMIZE
// 	if(check_rtp_nack_resend(stream,pkt,bytes_read) == PJ_SUCCESS)
// 	{
// 		return;
// 	}
// #endif

// 	/*ignore heartbreak rtp or nat rtp*/ 
// 	if (hdr->pt == HEARTBREAK_RTP_PT)	
// 	{		
// 		log_debug("ignore video rtp, pt:%d, seq:%d",hdr->pt,pj_ntohs(hdr->seq));		
// 		return; 
// 	}

//     calculation_video_rtp_loss(stream, hdr);

// 	writePackettoSub( stream->jb_opt, /*pj_int32_t id*/0, (pj_int8_t*)pkt, bytes_read,	pj_ntohs(hdr->seq), _max_lost_wait_time);
// }

// /*
//  * This callback is called by stream transport on receipt of packets
//  * in the RTP socket. 
//  */
// static void on_rx_rtp( void *data, void *pkt, pj_ssize_t bytes_read)

// {
//     pjmedia_vid_stream *stream = (pjmedia_vid_stream*) data;
//     pjmedia_vid_channel *channel = stream->dec;
//     pj_status_t status;

// 	/* Check for errors */
// 	if (bytes_read < 0) {
// 		log_error("name:%.*s, RTP recv() error, status:%d", 
// 			channel->port.info.name.slen,
// 			channel->port.info.name.ptr, status);
// 		return;
// 	}

// 	/* Ignore keep-alive packets */
// 	if (bytes_read < (pj_ssize_t) sizeof(pjmedia_rtp_hdr) && !stream->rstp_ip)
// 	return;

// #ifdef HYT_PTT
// 	if (((pjmedia_rtp_hdr*)pkt)->pt == 127 && 
// 		 pj_ntohs(((pjmedia_rtp_hdr*)pkt)->seq) < 30)	{		
// 		log_debug("receive vid rtp correction packet");		
// 		return; 
// 	}
// #endif

// 	if(PJMEDIA_RTP_PT_HYT_FEC == ((pjmedia_rtp_hdr*)pkt)->pt) return;
// 	on_rx_rtp_process(data, pkt, bytes_read);
// }

// /*
//  * This callback is called by stream transport on receipt of packets
//  * in the RTCP socket. 
//  */
// static void on_rx_rtcp( void *data,
//                         void *pkt, 
//                         pj_ssize_t bytes_read)
// {
//     pjmedia_vid_stream *stream = (pjmedia_vid_stream*) data;
// #ifdef PJMEDIA_VIDEO_RESEND_OPTIMIZE
// 	pj_status_t status = 0;
//     pj_uint8_t *p,blp_bit_idx,*pt, type;
//     pj_uint16_t rtp_seq,index,*p16,blp_val,pid_val,rtcp_len;
// 	unsigned rtp_pkt_len;
// 	unsigned rtcp_count = 0;
// 	void *rtp_pkt;
//      pj_bool_t nack_found = PJ_FALSE; 
//      pj_bool_t standard = PJ_FALSE; 	
//      pjmedia_vid_buf *resend = stream->enc->resend_vbuf;

// #endif

//     /* Check for errors */
//     if (bytes_read < 0) {
// 	log_error("RTCP recv() error, return:%d", (pj_status_t)-bytes_read);	
// 	return;
//     }

//     /* add by t35599*/
//     if (stream->cctrl && stream->cc_irtcp) {
//       stream->cc_irtcp(stream->cctrl, pkt, bytes_read);
//     }
//     /* end */

// #ifdef PJMEDIA_VIDEO_RESEND_OPTIMIZE
//     p = (pj_uint8_t*)pkt;
//     pjmedia_rtcp_common *common = (pjmedia_rtcp_common*)p;
//     if(common->pt == RTCP_RR || common->pt == RTCP_SR){
// 	   rtcp_count = 1;
// 	   rtcp_len = ntohs(common->length);
// 	   while(rtcp_count < 16){
// 		    p += (rtcp_len  + 1)* 4 ;

// 		 /* add by j33783 20190508 */
// 		  if(p > (pj_uint8_t*)pkt + bytes_read - 4)
// 		  {
// 		  	log_debug("read buffer out of packet range, rtcp count:%u,  packet:%p, bytes:%u, p:%p", rtcp_count, pkt, bytes_read, p);
// 			break;
// 		  }

// 		  pj_uint16_t rtcp_len_next = ntohs(*((pj_uint16_t*)(p+2)));
// 		   if((p + (rtcp_len_next +1)*4) > (pj_uint8_t*)pkt + bytes_read)
// 		   {
// 		   	 log_debug("read buffer out of packet range, rtcp count:%u, packet:%p, bytes:%u, p:%p, next_rtcp_len:%u", rtcp_count,  pkt, bytes_read, p, (rtcp_len_next+1)*4);
// 		   	 break;
// 		   }
		   
// 		    pt = p + 1;
// 		    log_debug("parse rtcp packet pt=%d,rtcp_count=%d, rtcp_len=%d",*pt,rtcp_count,rtcp_len);
// 		    if(*pt == RTCP_NACK){
// 			   p16 = (pj_uint16_t *)(pt + 1);
// 		          rtcp_len = ntohs(*p16);
// 			   nack_found = PJ_TRUE;
// 			   standard = (*p & 0x1f) == 1 ? PJ_TRUE : PJ_FALSE;
// 			   break;
// 		    }
// 		    else if(*pt == 192 || *pt == 206) /* add by j33783 20190805 support FIR parsing */
// 		    {
// 		    	  log_debug("found rtcp fir, need to send force keyframe.");
// 			  pjmedia_vid_stream_send_keyframe(stream);
// 		    	  break;
// 		    }
// 		    else if(*pt >= 203  || *pt < 200){
// 		          break;
// 		    }
// 		    p16 = (pj_uint16_t*)(pt + 1);
// 		    rtcp_len = ntohs(*p16);
// 		    rtcp_count++;
// 	   }

//     }

// 	if(nack_found){
// 		p += rtcp_len * 4 ;
// 		p16 = (pj_uint16_t*)p;
// 		pid_val = ntohs(*p16);
// 		p16++;
// 		blp_val = ntohs(*p16);
// 		if(standard){
// 			resend_rtp(stream,resend, pid_val,blp_val,1); 

// 		}else{
// 			if(pid_val || blp_val){
// 				resend_rtp(stream,resend, pid_val,blp_val,0); 
// 			}else{
// 				pjmedia_rtcp_rx_rtcp(&stream->rtcp, pkt, bytes_read); 
// 			}
// 		}
	
// 	}else{
// 		pjmedia_rtcp_rx_rtcp(&stream->rtcp, pkt, bytes_read);
// 	} 
// #else
//     pjmedia_rtcp_rx_rtcp(&stream->rtcp, pkt, bytes_read);
// #endif

// /** Begin add by h15130 for when video call network is wrong we need close video only keep audio */
// #ifdef HYT_PTT
// 	log_debug("transmit_need_end:%d", stream->rtcp.transmit_need_end);
// 	if(stream->rtcp.transmit_need_end)
// 	{
// 		log_debug("enc paused:%d", stream->enc->paused);
// 		if(!stream->enc->paused && stream->ext_user_data.user_cb.on_rx_video_transmit_end_cb != NULL)
// 		{
// 			(*stream->ext_user_data.user_cb.on_rx_video_transmit_end_cb)(stream, stream->ext_user_data.user_data, 0, 0, 0);
// 		}
// 		stream->rtcp.transmit_need_end = PJ_FALSE;
// 	}
// #endif
// /** End add by h15130 for when video call network is wrong we need close video only keep audio */
// }

// #ifdef PJMEDIA_VIDEO_RESEND_OPTIMIZE
// static void resend_save_rtp( pjmedia_vid_channel *channel, pj_uint16_t pkt_len){
// 	pj_uint16_t index;  
// 	pjmedia_vid_buf *resend = channel->resend_vbuf; 
// 	index = channel->rtp.out_extseq % resend->buf_size; 
// 	pj_memcpy(resend->buf + index * PJMEDIA_MAX_MTU, channel->buf, pkt_len);
// 	resend->pkt_len[index] = pkt_len; 
// 	resend->resend_times[index] = 0;
// 	log_debug("save rtp, seq:%d, len:%d", channel->rtp.out_extseq,pkt_len);
// }
// #endif

// static pj_bool_t determine_rtp_orientation_extension(pj_uint8_t fu0, pj_uint8_t fu1)
// {
// 	pj_uint8_t FU_Type;
// 	log_trace("%02x %02x", fu0, fu1);
// 	FU_Type = fu0 & 0x1F;
	
// 	/*modify by 180732700*/
//     switch(FU_Type)
//     {
// 	 case AVC_NAL_FU_TYPE_A: 	
// 		  return (fu1 & 0x80)?(1):(0); break;
// 	 case AVC_NAL_UNIT_SPS: 
// 		  return 1; break;
// 	 case AVC_NAL_UNIT_IDR: 
// 		  return (fu1 & 0x80)?(1):(0); break;
// 	 case AVC_NAL_SINGLE_NAL_MIN: 
// 		  return (fu1 & 0x80)?(1):(0); break;
// 	 default: 
// 		  return 0; break;	
//     }

//     log_trace("No case matched,return 0;");
// 	return 0;
// }

// static pj_bool_t determine_rtp_orientation_extension_H265(pj_uint8_t fu0, pj_uint8_t fu1, pj_uint8_t fu2)
// {
// 	pj_uint8_t FU_Type;
// 	FU_Type = (fu0 & 0x7E)>>1;
	
// 	/*modify by 180732700*/
// 	switch(FU_Type)
// 	{
// 	  case HEVC_NAL_FU_TYPE:
// 		return (fu2 & 0x80)?(1):(0); break;
// 	  /*case HEVC_NAL_UNIT_VPS: return 1; break;*/
// 	  case HEVC_NAL_UNIT_SPS:
// 		return 1; break;
// 	  default:
// 		return 0; break;
// 	}

// 	log_debug("%02x %02x %02x   No case matched,return 0;", fu0, fu1, fu2);

// 	return 0;

// }




