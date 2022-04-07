
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netinet/in.h>

#include <signal.h>

#include "glog.h"
#include "comm_event.h"


pjmedia_event_command g_event_command;


void sigHandler( int sockid, short ev, void * arg ) {
    event_release((pjmedia_event_command*)arg);
    log_info("sigHandler process.");
    // station->shutdown();
}

//inistance of typedef void (*func_worker_recvfrom)(void *arg);
static void proc_event_loop(void *arg)
{
    event_create((pjmedia_event_command*)arg);
}

int event_create(pjmedia_event_command *event_comm) {
    /* Don't die with SIGPIPE on remote read shutdown. That's dumb. */
    signal( SIGPIPE, SIG_IGN );

    int ret = 0;

    if( 0 == ret ) {

        // Clean close on SIGINT or SIGTERM.
        struct event evSigInt, evSigTerm;
        signal_set( &evSigInt, SIGINT,  sigHandler, event_comm );
        event_base_set( event_comm->event_base_obj, &evSigInt );
        signal_add( &evSigInt, NULL);

        signal_set( &evSigTerm, SIGTERM, sigHandler, event_comm );
        event_base_set( event_comm->event_base_obj, &evSigTerm );
        signal_add( &evSigTerm, NULL);

        /* Start the event loop. */
        while(event_comm->is_running) {
            event_base_loop( event_comm->event_base_obj, EVLOOP_ONCE );
        }

        //mEventArg.Destroy();
        //GLOGV("ActorStation is shutdown.\n");

        signal_del( &evSigTerm );
        signal_del( &evSigInt );
    }
    return ret;
}

int event_release(pjmedia_event_command *event_comm)
{
    if(event_comm->is_running) {
			event_comm->is_running = 0;
			struct timeval tv;
			tv.tv_sec=0;
			tv.tv_usec=10;
			event_loopexit(&tv);

			log_info("event_release function.\n");
	}
    event_destroy();
    return 0;
}

pj_status_t event_thread_start( struct pjmedia_event_command* eve_comm, char*threadName)
{
    pj_status_t status = 0;
    
    if(eve_comm->is_running)
        event_destroy(eve_comm);

    //init
    eve_comm->event_base_obj    = (struct event_base*)event_init();
    eve_comm->is_running        = 1;
    eve_comm->user_udp          = &g_event_command;

    memset(&(eve_comm->recv_tid), 0, sizeof(pj_thread_t));
    pj_thread_create(threadName, (thread_proc *)eve_comm->work_recv, eve_comm->user_udp, 0, 0, &eve_comm->recv_tid);
    if (status != 0) {
        log_error("pj_thread_create create failed name:%s", threadName);
        return status;
    }
    
    eve_comm->is_running = PJ_TRUE;
    
    return status;
}

RTC_API
int event_command_recv_start(const char*localAddr, unsigned short localPort, on_comm_recv comm_cb) {
    int status = 0;
    memset(&g_event_command, 0, sizeof(pjmedia_event_command));

    //set data callback to ui layer
    g_event_command.comm_cb     = comm_cb;
    g_event_command.work_recv   = proc_event_loop;

    event_thread_start(&g_event_command, "udp_event");

    return status;
}

RTC_API
int event_command_recv_stop() {
    int status = -1;
    event_release(&g_event_command);

    return status;
}
