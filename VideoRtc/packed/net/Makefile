
#--------------------------------------------------------------------

CC = gcc
AR = ar cru
CFLAGS = -Wall -D_REENTRANT -DHAVE_CONFIG_H -g -fPIC
SOFLAGS = -shared
LDFLAGS = -shared

LINKER = $(CC)
LINT = lint -c
RM = /bin/rm -f

LIBEVENT_INCL = -I./
#LIBEVENT_INCL = -I$(HOME)/libevent/include -I../common
#LIBEVENT_LIB  = -L$(HOME)/libevent/lib -levent

CFLAGS  += $(LIBEVENT_INCL)
LDFLAGS += $(LIBEVENT_LIB)

#--------------------------------------------------------------------

LIBOBJS = 

TARGET = libnet.so

#--------------------------------------------------------------------

all: $(TARGET)


libnet.so: buffer.o epoll.o epoll_sub.o event.o evbuffer.o signal.o log.o
	$(LINKER) $(LDFLAGS) $^ -o $@

clean:
	@( $(RM) *.o vgcore.* core core.* $(TARGET) )

#--------------------------------------------------------------------

# make rule
%.o : %.c
	$(CC) $(CFLAGS) -c $^ -o $@	

%.o : %.cpp
	$(CC) $(CFLAGS) -c $^ -o $@	

