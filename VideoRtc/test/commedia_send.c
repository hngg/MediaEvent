
#include <stdio.h>
#include "files.h"


#define COMM_PORT 2019

#define FILE_TASK_ID 100

int string_command_call(void*user, char* buffer, int size)
{
	int status = 0;
	//struct pjmedia_vid_command *media = (struct pjmedia_vid_command *) user;
	if(buffer[0]=='o') {
		med_file_sender("IMG_20210924_153452.jpg", "IMG_20210924_153452.jpg", TASKTYPE_NORMALFILE, FILE_TASK_ID);
	}
	printf("recv length:%d client addr:%s\n", size, med_command_client_addr());
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

	med_command_recv_start("0.0.0.0", 2016, &string_command_call);
	med_command_connect_set("0.0.0.0", 2019);
	med_file_connect_set("0.0.0.0", 2020, ".", &file_status_call);

	getchar();
	if(argc==2)
		med_file_sender(argv[1], argv[1], TASKTYPE_NORMALFILE, FILE_TASK_ID);//"IMG_20210924_153452.jpg"
	else
		if(argc==3)
			med_file_sender(argv[1], argv[1], TASKTYPE_THUMBNAIL, FILE_TASK_ID);

	getchar();
	med_command_send("2", 2);
	getchar();
    med_command_recv_stop();

	printf("exit done.\n");

    return 0;
}