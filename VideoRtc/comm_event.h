

#ifndef __EVENT_COMMAND_H__
#define __EVENT_COMMAND_H__

#include <sys/types.h>  //u_char define
#include <string.h>     //memset


#include "types.h"
#include "event.h"

#include "transport_udp.h"
#include "video_rtc_api.h"

typedef struct pjmedia_event_command
{
    on_comm_recv        comm_cb;
    char                send_buff[PJMEDIA_MAX_MTU];  /**< Outgoing packet.        */

    void                *user_data;    //struct point to self
    //pj_bool_t           attached;
    int                 maxFds;

    int                 is_running;
    pj_thread_t recv_tid;
    void *user_udp;
    func_worker_recvfrom    work_recv;

    struct event_base   *event_base_obj;
}pjmedia_event_command;

int event_create(pjmedia_event_command *event_comm);
int event_release(pjmedia_event_command *event_comm);

RTC_API
int event_command_recv_start(const char*localAddr, unsigned short localPort, on_comm_recv comm_cb);
int event_command_recv_stop();

#endif
