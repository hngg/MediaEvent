

#include "comm_media.h"
#include "utils.h"
#include "os.h"
#include "glog.h"
#include "rtcp.h"
#include "files.h"

#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <math.h>

pjmedia_vid_command g_vid_command;

//declare function
static void on_rx_file(void *user, void *pkt, pj_ssize_t bytes_read);

//command thread
static void worker_comm_recvfrom(void *arg)
{
    struct pjmedia_vid_command *udp = (struct pjmedia_vid_command *) arg;
    struct trans_channel *comm_chan = &udp->comm_chan;
    struct trans_channel *file_chan = &(udp->ftask.file_chan);
    pj_uint32_t  value = 0;
    if (ioctl(comm_chan->sockfd, FIONBIO, &value))
        return ;
    
    ssize_t status = 0;
    pj_ssize_t  bytes_read = 0;
    socklen_t addr_len =sizeof(struct sockaddr_in);
    fd_set  recv_set;   
    struct timeval timeout; //, tv_start = {0, 0}, tv_end = {0, 0};

    on_comm_recv comm_cb    = udp->comm_cb;        //from video_rtc_api.h
    on_recv_data comm_byte  = comm_chan->recv_cb;  //from transport_udp.h
    on_recv_data file_cb    = file_chan->recv_cb;  //from transport_udp.h
    while(comm_chan->sockfd != PJ_INVALID_SOCKET && !comm_chan->recv_tid.thread_quit)  /* modify by j33783 20190509 */
    {
        //special, use the struct pjmedia_vid_stream
        // if(chann->is_rtcp && chann->connecting)
        // {
        //     gettimeofday(&tv_end, NULL);
        //     if(tv_end.tv_sec-tv_start.tv_sec>=1)
        //     {
        //         gettimeofday(&tv_start, NULL);
        //         char rtcp[100] = {0};
        //         pj_size_t size = 0;
        //         rtcp_build_rtcp_sr(rtcp, &size);
        //         status = sendto(chann->sockfd, rtcp, size, 0,
        //                     (const struct sockaddr *)&chann->remote_addr, sizeof(struct sockaddr_in));
        //         if(status < 0) {
        //             log_error("rtcp sendto failed sockid:%d errno:%d", chann->sockfd, errno);
        //         }
        //         log_debug("rtcp send begin size:%d rtcp:%d addr:%s port:%d", size, chann->is_rtcp,
        //                   inet_ntoa(chann->remote_addr.sin_addr), htons(chann->remote_addr.sin_port));
                
        //     }
        // }    //rtcp sr heartbeat
        
        timeout.tv_sec  = 0;
        timeout.tv_usec = 200000;   //100ms
        FD_ZERO(&recv_set);
        FD_SET(comm_chan->sockfd, &recv_set);
        FD_SET(file_chan->sockfd, &recv_set);
        status = select(udp->maxFds, &recv_set, NULL, NULL, &timeout);
        if(status>0)
        {
            if(FD_ISSET(comm_chan->sockfd, &recv_set)) {
                bytes_read = RTP_LEN;
                status = recvfrom(comm_chan->sockfd, comm_chan->recv_buf, bytes_read, 0, (struct sockaddr *)&comm_chan->client_addr, &addr_len);
                if (status>0 && comm_cb)
                {
                    pjmedia_common_head *head = (pjmedia_common_head*)comm_chan->recv_buf;
                    if(head->ssrc!=0X3c3c3c3c)
                        (*comm_cb)(udp, comm_chan->recv_buf, (int)status);
                    else
                        (*comm_byte)(udp, comm_chan->recv_buf, (int)status);
                    log_warn("recvfrom comm size:%d address:%s", status, inet_ntoa(comm_chan->client_addr.sin_addr));
                }
                else //EAGAIN
                {
                     //to do
                }
            }
            if(FD_ISSET(file_chan->sockfd, &recv_set)) {
                bytes_read = RTP_LEN;
                status = recvfrom(file_chan->sockfd, file_chan->recv_buf, bytes_read, 0, (struct sockaddr *)&file_chan->client_addr, &addr_len);
                
                if (status>0 && file_cb)
                {
                    (*file_cb)(file_chan->user_udp, file_chan->recv_buf, (int)status);
                    log_warn("recvfrom file size:%d address:%s", status, inet_ntoa(file_chan->client_addr.sin_addr));
                }
                else //EAGAIN
                {
                     //to do
                }
            }
        }
        else if(0 == status)//
        {
            usleep(10);//10000->10ms 10us
            continue;
        }
        else if(status < 0)
        {
            //log_error("thread will exit, pj_sock_select return:%d", status);
            break;
        }
        
    }//while
}

//read command bytes
static void on_rx_comm(void *user, void *pkt, pj_ssize_t bytes_read)
{
    //int result = 0;
    struct pjmedia_vid_command* media       = (struct pjmedia_vid_command *) user;
    struct pjmedia_common_head* commhead    = (pjmedia_common_head*)pkt;
    struct pjmedia_files_task* ftask        = &media->ftask;

    switch(commhead->pt)
    {
        case FILE_SEND_REQ_SYN://acker->recv
        {
            pjmedia_files_info* finfo = (pjmedia_files_info*)(pkt+sizeof(pjmedia_common_head));
            log_error("prepare recv file info name:%s filesize:%lf maxSeq:%d filetype:%d\n", 
                        finfo->filename, finfo->fileSize, finfo->maxSeq, finfo->filetype);
            ftask->isrecv = 1;
            ftask->recvSeqCount = 0;
            memset(&media->ftask.file_info, sizeof(pjmedia_files_info), 0);
            memcpy(&media->ftask.file_info, finfo, sizeof(pjmedia_files_info));
            
            char wpath[FILENAME_MAX_LEN] = {0};
            switch(finfo->filetype) {
                case TASKTYPE_THUMBNAIL:
                    sprintf(wpath, "%s%s%s", media->ftask.savedir, RECV_NAIL_DIR, LAST_NAIL_NAME);//eg:root/recvnail/lastnail.jpg
                    media->ftask.fd = fopen(wpath, "w");
                break;

                case TASKTYPE_PHOTOVIEW:
                    sprintf(wpath, "%s%s%s", media->ftask.savedir, RECV_PHOTO_DIR, finfo->filename);//eg:root/recvnail/lastnail.jpg
                    media->ftask.fd = fopen(wpath, "w");
                break;

                case TASKTYPE_NORMALFILE:
                    sprintf(wpath, "%s%s%s", media->ftask.savedir, SAVE_FILE_DIR, finfo->filename);//eg:root/savefile/photo.mp4
                    media->ftask.fd = fopen(wpath, "w");
                break;
            }
            log_error("fopen filepath:%s result:%s.", wpath, media->ftask.fd?"success":"failed");

            char*send_buff  = media->send_buff;
            build_comm_file_head(send_buff, FILE_SEND_REQ_ACK); //payload type
            med_command_send(send_buff, sizeof(pjmedia_common_head));
        }
        break;
        case FILE_SEND_REQ_ACK://acker->send
        {
            int seq = 0;
            int taskId = ftask->file_info.fileid;
            if(ftask->fd > 0)
            {
                while(!feof(ftask->fd))
                {
                    int readsize = 0, sendres = 0;

                    pjmedia_file_hdr *head = (pjmedia_file_hdr*)ftask->buffer;
                    head->pt    = taskId;
                    head->seq   = seq++;
                    readsize    = fread (ftask->buffer+FILE_HEAD_LENGTH, 1, PJMEDIA_MAX_VID_PAYLOAD_SIZE, ftask->fd);//return 0 is end of the file
                    head->m     = feof(ftask->fd);
                    head->p     = 0;
                    if(readsize>0/* && head->seq%100!=0*/)  //&& head->seq!=100
                    {
                        sendres = med_file_data_send(ftask->buffer, FILE_HEAD_LENGTH+readsize);
                        log_error("readsize:%d sendto:%d seq:%d\n", readsize, sendres, head->seq);
                    }
                    usleep(20);
                }
                char*send_buff = media->send_buff;
                build_comm_file_head(send_buff, FILE_SEND_DONE_SYN);//payload type
                med_command_send(send_buff, sizeof(pjmedia_common_head));
            }
        }
        break;
        case FILE_NACK_REQ_SYN://acker->send, resend the losted packages
        {
            pjmedia_file_nack_info* nackinfo = (pjmedia_file_nack_info*)(pkt+sizeof(pjmedia_common_head));
            int nodeNum = nackinfo->node_count, i=0, j=0, nodePos = sizeof(pjmedia_common_head)+sizeof(pjmedia_file_nack_info);
            log_error("lost package nodeNum:%d", nodeNum);
            for(i=0; i<nodeNum; i++)
            {
                pjmedia_file_nack_node* nackNode = (pjmedia_file_nack_node*)(pkt+nodePos);
                for(j=0; j<nackNode->lostNum; j++)
                {
                    int result = fseek(ftask->fd, (nackNode->baseSeq+j)*PJMEDIA_MAX_VID_PAYLOAD_SIZE, SEEK_SET);
                    log_error("lost package fseek result:%d seq:%d", result, nackNode->baseSeq+j);
                    if(0!=result)
                    {
                        log_error("fseek failed at seq:%d", nackNode->baseSeq);
                    }
                    int readsize = 0, sendres = 0;
                    pjmedia_file_hdr *head = (pjmedia_file_hdr*)ftask->buffer;
                    head->pt    = ftask->file_info.fileid;
                    head->seq   = nackNode->baseSeq+j;
                    readsize    = fread (ftask->buffer+FILE_HEAD_LENGTH, 1, PJMEDIA_MAX_VID_PAYLOAD_SIZE, ftask->fd);//return 0 is end of the file
                    head->m     = (i==nodeNum-1)&&(j==nackNode->lostNum-1);
                    head->p     = 1;
                    if(readsize>0) {
                        sendres = med_file_data_send(ftask->buffer, FILE_HEAD_LENGTH+readsize);
                        log_error("resend file seq:%d sendsize:%d mark:%d\n", head->seq, sendres, head->m);
                    }
                    usleep(20);
                }//for j
                nodePos+=FILE_NACK_NODE_LENGTH;
            }//for i
        }
        break;
        case FILE_NACK_REQ_ACK:
        {

        }
        break;

        case FILE_RECV_REQ_SYN://acker->send
        {
            //send recv require ack
            char*send_buff = media->send_buff;
            build_comm_file_head(send_buff, FILE_RECV_REQ_ACK);//payload type
            med_command_send(send_buff, sizeof(pjmedia_common_head));

            //send send file command to receiver
            pjmedia_files_info* finfo = (pjmedia_files_info*)(pkt+sizeof(pjmedia_common_head));
            med_file_sender(finfo->filepath, finfo->filename, finfo->filetype, finfo->fileid);
        }
        break;
        case FILE_RECV_REQ_ACK://acker->recv
        {
            
        }
        break;
        case FILE_RECV_DONE_SYN://acker->send, receiver done
        {
            pjmedia_files_info* finfo = &media->ftask.file_info;
            on_file_status fstatus_cb = media->ftask.fstatus_cb;
            if(fstatus_cb) {
                (*fstatus_cb)(finfo->filename, finfo->filetype, finfo->fileid, finfo->fileSize, FILESTATUS_SEND_DONE);
            }
            //close read file fd
            if(media->ftask.fd) {
                fclose(media->ftask.fd);
                media->ftask.fd = NULL;
                log_warn("close read file :%s", finfo->filename);
            }
        }
        break;
        case FILE_RECV_DONE_ACK:
        {

        }
        break;
    }//break

    log_error("on_rx_comm size:%d pt:%d\n", (int)bytes_read, commhead->pt);
}

//command thread
static void worker_file_recvfrom(void *arg)
{

}

RTC_API
int med_command_recv_start(const char*localAddr, unsigned short localPort, on_comm_recv comm_cb) {
    int status = 0;
    memset(&g_vid_command, 0, sizeof(pjmedia_vid_command));

    //set data callback to ui layer
    g_vid_command.comm_cb = comm_cb;
    
    //create comm socket and bind
    struct trans_channel* comm_chan = &g_vid_command.comm_chan;
    comm_chan->user_udp = &g_vid_command;   //user data
    comm_chan->is_rtcp  = PJ_TRUE;          //protocal type
    status = transport_channel_create(comm_chan, localAddr, localPort, PJ_TRUE, &on_rx_comm, worker_comm_recvfrom);

    //create files socket and bind
    struct trans_channel* file_chan = &(g_vid_command.ftask.file_chan);
    file_chan->user_udp = &g_vid_command;   //user data
    file_chan->is_rtcp  = PJ_TRUE;          //protocal type
    status = transport_channel_create(file_chan, localAddr, localPort+1, PJ_TRUE, &on_rx_file, worker_file_recvfrom);

    g_vid_command.maxFds = (comm_chan->sockfd > file_chan->sockfd)?comm_chan->sockfd+1:file_chan->sockfd+1;

    log_error("maxFds is:%d\n", g_vid_command.maxFds);
    if(status>=0)
    {
        //start recv thread
        status = transport_thread_start(comm_chan, "comm_thread");
    }

    return status;
}

RTC_API
int med_command_recv_stop() {
    int status = -1;
    status = transport_channel_destroy(&g_vid_command.comm_chan);
    return status;
}

RTC_API
int med_command_connect_set(const char* remoteAddr, unsigned short remotePort) {
    int status = -1;
    status = transport_remote_connect(&g_vid_command.comm_chan, remoteAddr, remotePort);
    return status;
}

//////////////////////////////////////////file trans//////////////////////////////////////////////////
//declared
static void on_rx_file(void *user, void *pkt, pj_ssize_t bytes_read)
{
    struct pjmedia_vid_command *media = (struct pjmedia_vid_command *) user;
    if(media->ftask.fd)
    {
        int result = 0;
        pjmedia_file_hdr* head    = (pjmedia_file_hdr*)pkt;
        pjmedia_files_task* task  = &media->ftask;
        pjmedia_files_info* finfo = &media->ftask.file_info;
        //disorder
        if((task->recvSeqCount==0 && finfo->lastSeq==0) || (head->seq - finfo->lastSeq) != 1)//(task->recvSeqCount==0 && finfo->lastSeq==0)是防止丢了第一个包
        {
            log_error("rxfpack sequence wrong:current seq:%d lastSeq:%d lostCount:%d", 
                        head->seq, finfo->lastSeq, head->seq-finfo->lastSeq-1);

            result = fseek(media->ftask.fd, head->seq*PJMEDIA_MAX_VID_PAYLOAD_SIZE, SEEK_SET);
            if(0!=result)
            {
                finfo->lastSeq = head->seq;
                log_error("fseek failed at seq:%d", head->seq);
                return;
            }
        }
            
        result = fwrite(pkt+FILE_HEAD_LENGTH, 1, bytes_read-FILE_HEAD_LENGTH, media->ftask.fd);
        log_warn("rxfpack size:%d pt:%d wres:%d seq:%d \n", (int)bytes_read, head->pt, result, head->seq);

        //receive file package recorder
        int segIndex = head->seq>>5, bitIndex = head->seq%32;
        setBitOn(&task->recvMark[segIndex], bitIndex);
        task->recvSeqCount++;
        int recvDone = 0;
        char*send_buff = media->send_buff;
        //file stream end first time
        if(head->m)
        {
            memset(send_buff, 0, PJMEDIA_MAX_MTU);
            if(task->recvSeqCount < finfo->maxSeq)
            {
                int buffLen = buildLostPackage(task->recvMark, finfo->maxSeq, send_buff);
                if(buffLen>0)
                {
                    med_command_send(send_buff, buffLen);   //send losted package require
                    log_error("lost package buffer length:%d recvCount:%d", buffLen, task->recvSeqCount);
                }
                else
                    recvDone = 1;
            }else
                recvDone = 1;
        }

        if(recvDone)
        {
            build_comm_file_head(send_buff, FILE_RECV_DONE_SYN);    //payload type
            med_command_send(send_buff, sizeof(pjmedia_common_head));

            on_file_status fstatus_cb = media->ftask.fstatus_cb;
            if(fstatus_cb)
                (*fstatus_cb)(finfo->filename, finfo->filetype, finfo->fileid, finfo->fileSize, FILESTATUS_RECV_DONE);

            //close write file fd
            fflush(media->ftask.fd);
            fclose(media->ftask.fd);
            media->ftask.fd = NULL;
            log_warn("close write filename :%s", finfo->filename);

            //printf bit mark
            printBitMark(task->recvMark, finfo->maxSeq);
        }

        finfo->lastSeq = head->seq;
    }//if
}

RTC_API
int med_file_connect_set(const char* remoteAddr, unsigned short remotePort, const char*save_filedir, on_file_status file_status_cb) {
    int status = -1;
    //set file status callback function
    g_vid_command.ftask.fstatus_cb = file_status_cb;

    //copy file save dir path
    memset(g_vid_command.ftask.savedir, FILENAME_MAX_LEN, 0);
    strcpy(g_vid_command.ftask.savedir, save_filedir);

    //set remote addr and port
    status = transport_remote_connect(&g_vid_command.ftask.file_chan, remoteAddr, remotePort);
    return status;
}

RTC_API
int med_command_send(char* buffer, pj_uint32_t size) {
    ssize_t status = -1;
    struct trans_channel* comm_chan = &g_vid_command.comm_chan;
    status = sendto(comm_chan->sockfd, buffer, size, 0,
                   (const struct sockaddr *)&comm_chan->remote_addr, sizeof(struct sockaddr_in));
    if(status < 0) {
        log_error("comm sendto failed sockid:%d errno:%d errstr:%s", comm_chan->sockfd, errno, strerror(errno));
    }
    return (int)status;
}

RTC_API
int med_file_sender(const char*filepath, const char* filename, int fileType, int taskId) {
    int status = -1;
    
    struct pjmedia_files_task*ftask = &g_vid_command.ftask;
    struct pjmedia_files_info*finfo = &ftask->file_info;
    memset(finfo, 0, sizeof(pjmedia_files_info));
    strcpy(finfo->filename, filename);      //copy filename
    struct stat fsta;
	stat(filepath, &fsta);
	finfo->fileSize = fsta.st_size;         //file size
    finfo->maxSeq   = finfo->fileSize/PJMEDIA_MAX_VID_PAYLOAD_SIZE + (fmod(finfo->fileSize, PJMEDIA_MAX_VID_PAYLOAD_SIZE)?1:0);
    finfo->filetype = fileType;             //file type
    finfo->fileid   = taskId;               //file trans taskid

    ftask->isrecv   = 0;                    //sender
    ftask->fd = fopen(filepath, "rb");      //fopen

    log_error("med_file_sender double size:%lf size:%lld maxSeq:%d", finfo->fileSize, fsta.st_size, finfo->maxSeq);//lld->368036 lf->368036.000000 
    
    //file send require command
    char*send_buff = g_vid_command.send_buff;
    memset(send_buff, 0, PJMEDIA_MAX_MTU);
    build_comm_file_head(send_buff, FILE_SEND_REQ_SYN);     //payload type
    memcpy(send_buff+sizeof(pjmedia_common_head), finfo, sizeof(pjmedia_files_info));
    med_command_send(send_buff, sizeof(pjmedia_common_head)+sizeof(pjmedia_files_info));

    return status;
}

RTC_API
int med_file_receiver(const char*filepath, const char* filename, int fileType, int taskId) {
    int status = -1;
    //struct pjmedia_files_task*ftask = &g_vid_command.ftask;
    struct pjmedia_files_info finfo = {0};
    //memset(&finfo, 0, sizeof(pjmedia_files_info));
    strcpy(finfo.filepath, filepath);      //copy filename
    strcpy(finfo.filename, filename);      //copy filename

    finfo.filetype = fileType;             //file type
    finfo.fileid   = taskId;               //file trans taskid

    //send recv require command
    char*send_buff = g_vid_command.send_buff;
    memset(send_buff, 0, PJMEDIA_MAX_MTU);
    build_comm_file_head(send_buff, FILE_RECV_REQ_SYN); //payload type
    memcpy(send_buff+sizeof(pjmedia_common_head), &finfo, sizeof(pjmedia_files_info));
    med_command_send(send_buff, sizeof(pjmedia_common_head)+sizeof(pjmedia_files_info));
    return status;
}

RTC_API
char* med_command_client_addr() {
    //ssize_t status = -1;
    struct trans_channel* comm_chan = &g_vid_command.comm_chan;

    return inet_ntoa(comm_chan->client_addr.sin_addr);
}

int med_file_data_send(char* buffer, pj_uint32_t size) {
    ssize_t status = -1;
    struct trans_channel* file_chan = &g_vid_command.ftask.file_chan;
    status = sendto(file_chan->sockfd, buffer, size, 0,
                   (const struct sockaddr *)&file_chan->remote_addr, sizeof(struct sockaddr_in));
    if(status < 0) {
        log_error("comm sendto failed sockid:%d errno:%d", file_chan->sockfd, errno);
    }
    return (int)status;
}

// RTC_API
// int med_command_start(const char* remoteAddr, unsigned short remotePort) {
//     int status = -1;

//     status = transport_channel_start(&g_vid_command.channel, remoteAddr, remotePort, "comm_thread");
    
//     return status;
// }

// RTC_API
// int med_command_stop() {
//     int status = -1;
//     status = transport_channel_stop(&g_vid_command.channel);
//     return status;
// }

