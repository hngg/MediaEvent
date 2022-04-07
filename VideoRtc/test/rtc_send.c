#include <string.h>
#include "NALDecoder.h"
#include "video_rtc_api.h"

//send port 38123, recv port 36123

int media_frame_cb(char* buffer, int size, int type)
{
	packet_and_send(buffer, size);
	printf("file_frame size:%d type:%d\n", size, type);
	return 0;
}

int rtp_frame_cb(char* frameBuffer, int frameLen) 
{
	printf("rtp_frame_cb size:%d\n", frameLen);
	return 0;
}

int start_send(char*addr, int localPort, int remotePort, int codecType)
{
	vid_stream_create(addr, localPort, codecType);
	vid_stream_recv_start_ios(rtp_frame_cb);
	vid_stream_remote_connect(addr, remotePort);

	return 0;
}

int stop_send()
{
	vid_stream_recv_stop();
	vid_stream_destroy();
	return 0;
}

int main( int argc, char ** argv )
{
	if ((argc != 2)) {
		printf("Usage: exe bareflow_filename.\n");
		return 0;
	}

    char extenName[12] = {0};
    file_exten(argv[1], extenName);

    int frameType = H264_HARD_CODEC;	//define in types.h
    if(0==strcmp("h265", extenName));
        frameType = H265_HARD_CODEC;
    if(0==strcmp("h264", extenName))
        frameType = H264_HARD_CODEC;
		
	printf("frame type:%d\n", frameType);

	start_send("0.0.0.0", 38123, 36123, frameType);

	start_file_frame_out(argv[1], frameType, media_frame_cb);
	getchar();
	stop_send();

	return 0;
}

