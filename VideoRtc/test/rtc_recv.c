
#include <stdio.h>
#include <stdlib.h>
#include "NALDecoder.h"
#include "video_rtc_api.h"

//send port 38123, recv port 36123



int rtp_recv_cb(char* frameBuffer, int frameLen) 
{
	printf("rtp_frame_cb size:%d\n", frameLen);
	return 0;
}

int start_recv(char*addr, int localPort, int remotePort, int codecType)
{
	vid_stream_create(addr, localPort, codecType);
	vid_stream_recv_start_ios(rtp_recv_cb);
	vid_stream_remote_connect(addr, remotePort);

	return 0;
}

int stop_recv()
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

	int codecType = (int)atoi(argv[1]);
	printf("recv codecType:%d and 10 is h264 20 is h265\n", codecType);
	start_recv("0.0.0.0", 36123, 38123, codecType);//H264_FRAME==1 H265_FRAME==2

    getchar();

	stop_recv();

	return 0;
}

