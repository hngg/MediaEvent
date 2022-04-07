CXXFLAGS += -std=c++11
 
CC 	= gcc
CPP = g++
AR  = ar

COMPLIE_INCL = -I./ -I./include -I./packed -I./packed/pj -I./packed/net

COMPLIE_LIB  = -L./ -lpthread -lm
CFLAGS 		= -Wall -DHAVE_CONFIG_H -DSENDTEST -g -fPIC -DSAVE_RECV_FILE
CXXFLAGS += $(COMPLIE_INCL)
LDFLAGS += $(COMPLIE_LIB)

CFLAGS  += -I./ -I./packed
CFLAGS	+= $(COMPLIE_INCL)

TARGET:SEND RECV COMED_RECV COMED_SEND RTPTest 

OBJECTS = 	packed/net/buffer.o \
			packed/net/epoll.o \
			packed/net/epoll_sub.o \
			packed/net/event.o \
			packed/net/evbuffer.o \
			packed/net/signal.o \
			packed/net/log.o \
			packed/net/ikcp.o \
			packed/pj/os_core_unix.o \
		  	packed/pj/utils.o \
			packed/pj/glog.o \
			packed/fec/RS_FEC_log_table.o \
			packed/fec/RS_FEC.o \
			packed/packets_list.o \
			packed/jitter_buffer.o \
			packed/vid_unpack.o \
			packed/dealrtpunpack.o \
			packed/h264_packetizer.o \
			packed/list.o \
			packed/rtp.o \
			packed/rtcp.o \
			packed/files.o \
			vid_codec.o \
			vid_port.o \
			transport_udp.o \
			vid_stream.o \
			comm_media.o \
			comm_event.o


RTCLIB=RTCLIB
$(RTCLIB):$(OBJECTS)
	$(AR) rcs librtc.a $(OBJECTS)

##############################test##############################
TESTDIR  = test

SEND_OBJ = $(OBJECTS) \
			$(TESTDIR)/NALDecoder.o $(TESTDIR)/rtc_send.o

RECV_OBJ = $(OBJECTS) \
			$(TESTDIR)/NALDecoder.o $(TESTDIR)/rtc_recv.o

RTP_OBJ = $(OBJECTS) \
			$(TESTDIR)/RtpTest.o $(TESTDIR)/sps_pps.o #$(TESTDIR)/NALDecoder.o $(TESTDIR)/vid_stream_test.o 

COMED_RECV_OBJ = $(OBJECTS) \
			$(TESTDIR)/commedia_recv.o

COMED_SEND_OBJ = $(OBJECTS) \
			$(TESTDIR)/commedia_send.o

SEND=SEND
RECV=RECV
RTPTest=RTPTest
COMED_RECV=COMED_RECV
COMED_SEND=COMED_SEND

$(SEND):$(SEND_OBJ)
	$(CC) -o $(TESTDIR)/$(SEND) $(SEND_OBJ) $(LDFLAGS)

$(RECV):$(RECV_OBJ)
	$(CC) -o $(TESTDIR)/$(RECV) $(RECV_OBJ) $(LDFLAGS)

$(RTPTest):$(RTP_OBJ)
	$(CC) -o $(TESTDIR)/$(RTPTest) $(RTP_OBJ) $(LDFLAGS)

$(COMED_RECV):$(COMED_RECV_OBJ)
	$(CC) -o $(TESTDIR)/$(COMED_RECV) $(COMED_RECV_OBJ) $(LDFLAGS)

$(COMED_SEND):$(COMED_SEND_OBJ)
	$(CC) -o $(TESTDIR)/$(COMED_SEND) $(COMED_SEND_OBJ) $(LDFLAGS)

.PHONY:clean
clean:
	rm  *.o packed/*.o packed/pj/*.o $(TESTDIR)/*.o
	
