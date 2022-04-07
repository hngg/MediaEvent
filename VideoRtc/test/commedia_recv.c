
#include <stdio.h>
#include "comm_media.h"
#include "video_rtc_api.h"

#define COMM_PORT 2019

int string_command_call(void*user, char* buffer, int size) 
{
	int status = 0;
	struct pjmedia_vid_command *media = (struct pjmedia_vid_command *) user;
	if(buffer[0]=='1') {
		media->ftask.fd = fopen("save.jpg", "w");
		med_command_send("ok", 3);
		printf("fopen done.\n");
	}
	if(buffer[0]=='2') {
		if(media->ftask.fd != NULL) {
			fclose(media->ftask.fd);
			media->ftask.fd = NULL;
			printf("fclose done.\n");
		}
	}

	printf("recv length:%d client addr:%s %s\n", size, med_command_client_addr(), buffer);
	return status;
}

int file_status_call(char*filename, int filetype, int fileid, double totalsize, int status)
{
	printf("_____file_status_call filetype:%d fileid:%d totalsize:%lf status:%d\n", 
		   filetype, fileid, totalsize, status);
	return 0;
}

int main( int argc, char ** argv )
{
	if ((argc != 2)) {
		printf("Usage: exe bareflow_filename.\n");
		//return 0;
	}

	med_command_recv_start("0.0.0.0", 2019, &string_command_call);
	med_command_connect_set("0.0.0.0", 2016);
	med_file_connect_set("0.0.0.0", 2017, ".", &file_status_call);

	if(argc==2)
	{
		med_file_receiver(argv[1], argv[1], TASKTYPE_PHOTOVIEW, 1000);
	}

    getchar();
    med_command_recv_stop();

	printf("exit done.\n");

    return 0;
}

