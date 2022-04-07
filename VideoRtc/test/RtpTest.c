
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <stdarg.h>

#include "h264.h"
#include "basedef.h"
#include "rtp.h"
#include "list.h"
#include "files.h"

#include "video_rtc_api.h"
#include "vid_stream.h"
#include <h264_packetizer.h>

#include "sps_pps.h"

#define UNUSED(var) (void)((var)=(var))

#define	FILE_PATH	"bare720p.h264"


/*
void h265_nal_test() {
	
	// * h265 nal type:
	// * vps:0x40 01    nal type:0x20  32
	// * sps:0x42 01    nal type:0x21  33
	// * pps:0x44 01    nal type:0x22  34
	// * sei:0x4e 01    nal type:0x27  39
	// * ifr:0x26 01    nal type:0x13  19
	// * pfr:0x02 01    nal type:0x01  1
	
	printf("naltype vps:%02x sps:%02x pps:%02x sei:%02x ifr:%02x, pfr:%02x\n", 
		GET_H265_NAL_TYPE(0x40), GET_H265_NAL_TYPE(0x42), GET_H265_NAL_TYPE(0x44), 
		GET_H265_NAL_TYPE(0x4e), GET_H265_NAL_TYPE(0x26), GET_H265_NAL_TYPE(0x02));
}


typedef enum frame_type {
	H264_FRAME = 1,
	H265_FRAME = 2
}frame_type;

void threadFunc( void *arg ) {
	//long seconds = (long) arg;
}

void h264_test(pj_uint8_t *bits, pj_size_t bits_len) {
	static pjmedia_h264_packetizer pktz;
	//if(!pktz) {
	//	pktz = (pjmedia_h264_packetizer*)malloc(sizeof(pjmedia_h264_packetizer));
		memset(&pktz, 0, sizeof(pjmedia_h264_packetizer));
		pktz.cfg.mode = PJMEDIA_H264_PACKETIZER_MODE_NON_INTERLEAVED;
		pktz.cfg.mtu  = PJMEDIA_MAX_VID_PAYLOAD_SIZE;
	//}

	pj_size_t payload_len = 0;
	unsigned int enc_packed_pos = 0;
	pj_uint8_t payload[PJMEDIA_MAX_VID_PAYLOAD_SIZE] = {0};
	while(enc_packed_pos<bits_len) {
		pj_status_t res =  pjmedia_h264_packetize(&pktz,
										bits,
										bits_len,
										&enc_packed_pos,
										payload,
										&payload_len);
		GLOGE("------get size:%d payload_len:%d enc_packed_pos:%d indicator:%02X res:%d\n", 
			(int)bits_len, (int)payload_len, enc_packed_pos, payload[1], res);
	}
}

void h265_test(pj_uint8_t *bits, pj_size_t bits_len) {
	static pjmedia_h265_packetizer *pktz;
	if(!pktz) {
		pktz = (pjmedia_h265_packetizer*)malloc(sizeof(pjmedia_h265_packetizer));
		memset(pktz, 0, sizeof(pjmedia_h265_packetizer));
		//pktz->cfg.mode = PJMEDIA_H264_PACKETIZER_MODE_NON_INTERLEAVED;
		pktz->cfg.mtu  = PJMEDIA_MAX_VID_PAYLOAD_SIZE;
	}

	pj_size_t payload_len = 0;
	unsigned int	enc_packed_pos = 0;
	pj_uint8_t payload[PJMEDIA_MAX_VID_PAYLOAD_SIZE] = {0};
	while(enc_packed_pos<bits_len) {
		pj_status_t res =  pjmedia_h265_packetize(pktz,
										bits,
										bits_len,
										&enc_packed_pos,
										payload,
										&payload_len);
        UNUSED(res);
		//GLOGE("------get size:%d payload_len:%d enc_packed_pos:%d indicator:%02X\n", bits_len, payload_len, enc_packed_pos, payload[0]);
	}
}

void get_file_exten(const char*fileName, char*outExten) {
    int i=0,length= strlen(fileName);
    while(fileName[i]) {
        if(fileName[i]=='.')
            break;
            i++;
    }
    if(i<length)
        strcpy(outExten, fileName+i+1);
    else
        strcpy(outExten, "\0");
}


void vid_stream_test_file(char*filename) {
    FILE			*pFile;
	NALU_t 			*mNALU;
    char idrBuf[2*1024*1024] = {0};
	pFile = OpenBitstreamFile( filename ); //camera_640x480.h264 //camera_1280x720.h264
	if(pFile==NULL) {
		GLOGE("open file:%s failed.", filename);
		return;
	}

    //vid_stream_create();

    char extenName[12] = {0};
    get_file_exten(filename, extenName);
    GLOGE("filename exten:%s\n", extenName);

    int frameType = 0;
    if(0==strcmp("h265", extenName));
        frameType = H265_FRAME;
    if(0==strcmp("h264", extenName))
        frameType = H264_FRAME;

	mNALU  = AllocNALU(8000000);

	int count = 0, cpyPos = 0;
	do {
		count++;
		int size=GetAnnexbNALU(pFile, mNALU);//每执行一次，文件的指针指向本次找到的NALU的末尾，下一个位置即为下个NALU的起始码0x000001
		GLOGE("GetAnnexbNALU type:0x%02X size:%d count:%d\n", mNALU->buf[4], size, count);
		if(size<4) {
			GLOGE("get nul error!\n");
			continue;
		}

        int type = mNALU->buf[4] & 0x1F;
        if(type==7 || type==8) {
            memcpy(idrBuf+cpyPos, mNALU->buf, size);
            cpyPos += size;
            continue;
        }

        memcpy(idrBuf+cpyPos, mNALU->buf, size);
        cpyPos += size;
		//GLOGE("GetAnnexbNALU frame size:%d\n", cpyPos);

        switch(frameType) {
            case H264_FRAME:
                //h264_test(mNALU->buf, size);
                //h264_package_test(idrBuf, cpyPos);
                packet_and_send_test(idrBuf, cpyPos);
				//rtp_packet_recv_test(idrBuf, cpyPos);
                break;
            case H265_FRAME:
                h265_test(mNALU->buf, size);
                break;
        }
        cpyPos = 0;
	}while(!feof(pFile));

	if(pFile != NULL)
		fclose(pFile);

	FreeNALU(mNALU);

    //vid_stream_destroy();
}

int test_split_str()
{
    char str[COLUMN];
    char outValue[ROW] = {};
    strcpy(str, "dev=wifi&ssid=AndroidShard_001&pw=0a9bc352h9");
    getSubkeyValue(str, "&", "=", "pw", outValue);

    printf("get the last value:%s\n", outValue);

    char http[COLUMN];
    char mainType[ROW][COLUMN] = {};
    strcpy(http, "http://greatwit.com?app=greatlive");
    int count = splitStr(http, "?", mainType);
	int i=0;
    for(;i<count;i++)
        printf("tyep:%s\n", mainType[i]);
    return 0;
}

int test_list()
{
	list_node* head = list_create("head");
    list_insert_after(head, "hello,first");
    list_insert_after(head, "hello,second");
    list_insert_after(head, "cc");
    list_insert_after(head, "dd");
    list_insert_after(head, "ee");
    list_insert_after(head, "ea");
    list_insert_after(head, "eb");
    list_insert_after(head, "ec");
    list_insert_after(head, "ed");
    list_insert_after(head, "ee");
	
	list_node *node = list_find_by_data(head, "ee");
	printf("node :%s\n", (char*)node->data);
	return 0;
}
*/

#include <event.h>

int called = 0;

#define NEVENT	3

struct event *ev[NEVENT];

void
time_cb(int fd, short event, void *arg)
{
	struct timeval tv;
	int i, j;

	called++;

	if (called < 10*NEVENT) {
		for (i = 0; i < 10; i++) {
			j = random() % NEVENT;
			tv.tv_sec = 0;
			tv.tv_usec = random() % 50000L;
			if (tv.tv_usec % 2)
				evtimer_add(ev[j], &tv);
			else
				evtimer_del(ev[j]);
		}
	}
	printf("time_cb.\n");
}

int testTimeEvent ()
{
	struct timeval tv;
	int i;

	/* Initalize the event library */
	event_init();

	for (i = 0; i < NEVENT; i++) {
		ev[i] = malloc(sizeof(struct event));

		/* Initalize one event */
		evtimer_set(ev[i], time_cb, ev[i]);
		tv.tv_sec = 0;
		tv.tv_usec = random() % 50000L;
		evtimer_add(ev[i], &tv);
	}

	event_dispatch();

	return (called < NEVENT);
}

int lasttime;

void
timeout_cb(int fd, short event, void *arg)
{
	struct timeval tv;
	struct event *timeout = arg;
	int newtime = time(NULL);

	printf("%s: called at %d: %d\n", __func__, newtime, newtime - lasttime);
	lasttime = newtime;

	timerclear(&tv);
	tv.tv_sec = 2;
	event_add(timeout, &tv);
}

int testTimer()
{
	struct event timeout;
	struct timeval tv;
 
	/* Initalize the event library */
	event_init();
	printf("method:%s\n", event_get_method());
	/* Initalize one event */
	evtimer_set(&timeout, timeout_cb, &timeout);

	timerclear(&tv);
	tv.tv_sec = 2;
	event_add(&timeout, &tv);

	lasttime = time(NULL);
	
	event_dispatch();

	return (0);
}

void bitMarkTest()
{
	int full = 0xffffffff;
	printf("value:%d\n", full);	//18446744073709551616.000000

	//int  recvMark[32] = {0};

	int a = 0;
	setBitOn(&a, 3);
	printf("a value:%d\n", a);

	
	int value = 0xab9898ec;
	showIntBit(value, 32);
	printf("\ndone\n");

	int markRow[3]={0};
	markRow[0]=0x3b9898ec;
	markRow[1]=0x3fffffff;
	markRow[2]=0xe35678da;

	printBitMark(markRow, 92);
	printBitMark(markRow, 93);

    char outBuffer[1500] = {0};
	int result = buildLostPackage(markRow, 91, outBuffer);
	printf("buff length:%d\n", result);
}

void sps_pps_test(char*filepath)
{
	unsigned char data_buffer[512]= {0};
	FILE* file = fopen(filepath, "rb");
	fread(data_buffer, 47, 1, file);
	if(file) {
		fclose(file);
		file = NULL;
	}

	printf("type:%02X\n", data_buffer[4]);

	// Parse SPS 
	// uint8_t sps_buffer[100] = {0};
	// memcpy(sps_buffer, data_buffer+h264_pc.sps_start_pos, h264_pc.sps_size);
	
	get_bit_context  buffer;
	SPS h264_sps_context = {0};

	memset(&buffer, 0, sizeof(get_bit_context));
	buffer.buf      = data_buffer+5;
	buffer.buf_size = 34;
	h264dec_seq_parameter_set(&buffer, &h264_sps_context);
		
	printf("Pic width : %d\n", h264_get_width(&h264_sps_context) );
	printf("Pic height: %d\n", h264_get_height(&h264_sps_context) );


	// Parse PPS 
	PPS h264_pps_context = {0};

	memset(&buffer, 0, sizeof(get_bit_context));
	buffer.buf      = data_buffer+44;
	buffer.buf_size = 3;

	h264dec_picture_parameter_set(&buffer, &h264_pps_context);

	printf("slice number:%d\n", h264_pps_context.deblocking_filter_control_present_flag);
}

void printfH265Type()
{
	printf("%d %d %d %d\n", (0x40&0x7E)>>1, (0x42&0x7E)>>1, (0x44&0x7E)>>1, (0x26&0x7E)>>1);
}

int main( int argc, char ** argv )
{
	// unsigned short seq = 0xffff;
	// printf("seq value:%d\n", ++seq);
	// printf("seq value:%d\n", 65536%2048);

	// if ((argc != 2)) {
	// 	printf("Usage: exe bareflow_filename.\n");
	// 	return 0;
	// }
	// test_split_str();

    //vid_stream_test_file(argv[1]);
	//stream_trans_test();

	//h265_nal_test();

	//test_list();


	//sps_pps_test(argv[1]);
	//testTimeEvent ();
	printfH265Type();
	//testTimer();

	return 0;
}



