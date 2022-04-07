
#ifndef VIDEO_RTC_API_H_
#define VIDEO_RTC_API_H_

#define VERSION "1.1.1"


typedef int (*on_rtp_frame)(char* frameBuffer, int frameLen);
typedef int (*on_network_status)(int type, int rtt, int frame_rate, long lost_rate, int byte_count);
typedef int (*on_comm_recv)(void*user, char* buffer, int size);
typedef int (*on_file_status)(char*filename, int filetype, int fileid, double totalsize, int status);


#define RTC_API

//////////////////////////////////////commands and media/////////////////////////////////////////
RTC_API
int med_command_recv_start(const char*localAddr, unsigned short localPort, on_comm_recv comm_cb);
RTC_API
int med_command_recv_stop(void);
RTC_API //set the remote address to send
int med_command_connect_set(const char* remoteAddr, unsigned short remotePort); 
RTC_API
int med_command_send(char* buffer, unsigned int size);
RTC_API
char* med_command_client_addr();
// RTC_API //remove the remote address
// int med_command_connect_rmv(void);

RTC_API
int med_file_connect_set(const char* remoteAddr, unsigned short remotePort, const char*save_filedir, on_file_status file_status_cb);

RTC_API
int med_file_sender(const char*filepath, const char* filename, int fileType, int taskId);//fopen the filepath
RTC_API
int med_file_receiver(const char*filepath, const char* filename, int fileType, int taskId);


/////////////////////////////////////////rtp////////////////////////////////////////////////
RTC_API
int vid_stream_create(const char*localAddr, unsigned short localRtpPort, int codecType);
RTC_API
int vid_stream_destroy(void);

RTC_API
int vid_stream_network_callback(on_network_status net_cb);

RTC_API //for ios
int vid_stream_recv_start_ios(on_rtp_frame frame_cb);
RTC_API //for android
int vid_stream_recv_start_android(void* surface);
RTC_API
int vid_stream_recv_stop(void);

RTC_API
int vid_stream_remote_connect(const char*remoteAddr, unsigned short remoteRtpPort);
RTC_API
int packet_and_send(char* frameBuffer, int frameLen);


////////////////////////////////////////utils////////////////////////////////////////
RTC_API
char* getVersion(void);

#define ROW     32
#define COLUMN  128
RTC_API
int splitStr(char *src, char *format, char (*substr)[COLUMN]);
RTC_API
int getSubkeyValue(char *src, char*format, char*subform, char*inKey, char*outValue);

#endif
