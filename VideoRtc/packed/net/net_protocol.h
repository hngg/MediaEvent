/*==========================================================================
 *FILENAME:      net_protocol.h
 *
 *DESCRIPTION:   net_protocol Module
 *
 *AUTHOR:        mingjiawan               
 *
 *DATE:          2011-01-01
 *
 *COMPANY:      	 
===========================================================================*/

#ifndef __NET_PROTOCOL_H__
#define __NET_PROTOCOL_H__
#ifdef __cplusplus
extern "C"{
#endif

#define COMMAND_D_S_REGISTER_REQ		1
#define COMMAND_S_D_REGISTER_ACK		2
#define COMMAND_D_S_DATETIME_REQ		3
#define COMMAND_S_D_DATETIME_ACK		4
#define COMMAND_D_S_NULL			5               //����

#define COMMAND_S_D_DATAINFO_REQ		6
#define COMMAND_D_S_DATAINFO_ACK		7
#define COMMAND_S_D_DATA_REQ		8
#define COMMAND_D_S_DATA_ACK		9
#define COMMAND_S_D_TRANSMIT_REQ		10
#define COMMAND_D_S_TRANSMIT_ACK		11

#define PROTOCOL_VERSION "0100" //����ĸ��ֽ�
#define PROTOCOL_HEAD_LENGTH (14+2)
#define PROTOCOL_CMD_LENGTH (4)
#define PROTOCOL_VERSION_LENGTH (4)
#define PROTOCOL_LOAD_OFFSET (12)
#define PROTOCOL_LOAD_LENGTH (4)


//�����
#define PROTOCOL_REGIST (COMMAND_D_S_REGISTER_REQ)
#define PROTOCOL_REPLY_REGIST (COMMAND_S_D_REGISTER_ACK)
#define PROTOCOL_UPDATE_TIME (COMMAND_D_S_DATETIME_REQ)
#define PROTOCOL_REPLY_TIME (COMMAND_S_D_DATETIME_ACK)
#define PROTOCOL_HEARTBEAT (COMMAND_D_S_NULL)
#define PROTOCOL_ASKSDP (COMMAND_S_D_DATAINFO_REQ)
#define PROTOCOL_REPLY_ASKSDP (COMMAND_D_S_DATAINFO_ACK)
#define PROTOCOL_ASKDATA (COMMAND_S_D_DATA_REQ)
#define PROTOCOL_REPLY_ASKDATA (COMMAND_D_S_DATA_ACK)
#define PROTOCOL_TRANSFER (COMMAND_S_D_TRANSMIT_REQ)
#define PROTOCOL_REPLY_TRANSFER (COMMAND_D_S_TRANSMIT_ACK)

#define PROTOCOL_LONGIN (2001)
#define PROTOCOL_REPLY_LONGIN (2002)

typedef struct PROTO_ST_MSG
{
	unsigned char	*msg;
	int				len;
	int				total;
}PROTO_ST_MSG;

//typedef struct frame_SendProcParma
//{
//    int sem;
//    int fd;
//    ThreadLib_tid send_tid;
//    PROTO_ST_MSG st_send_recv_buf;
//    int buf_index;
//    int send_audio_flag;
//    int send_video_flag;
//    int lRunState; //�����߳��Ƿ�������
//
//}frame_SendProcParma;
//typedef struct _CMD_ARGS
//{
//    char clid[32];
//    int clen;
//    time_t time;
//
//}ST_CMD_ARGS;

int  PROTO_MakeDataHead(PROTO_ST_MSG *stMsg,int cmd);
int PROTO_GetPackDataLen(char *pDataHead);
int PROTO_GetCommand(char *pData);
int PROTO_AddOneItem(PROTO_ST_MSG *stMsg,char *pName,char *pValueOut,int lValueLen);
int PROTO_AddOneItemInt(PROTO_ST_MSG *stMsg,char *pName, int nValue);
int PROTO_GetValueByName(char *pData, char *pName, char * pValue, int *pValueLen);
int  PROTO_MakeRegistMsg(PROTO_ST_MSG *stMsg,char *pcDeviceSerial,char *pcDeviceName);
int PROTO_MakeSdpMsg(PROTO_ST_MSG *stMsg,char*sdp,int sdp_len);
int PROTO_MakeClientMsg(PROTO_ST_MSG *stMsg,char *pData, char *pName);
int PROTO_DestoryMsg(PROTO_ST_MSG *msg);
int PROTO_MakeAVDataMsg(PROTO_ST_MSG *stMsg,char*data,char *type,int data_len);
int PROTO_MakeUpTime(PROTO_ST_MSG *stMsg);
int PROTO_MakeHeartBeatMsg(PROTO_ST_MSG *stMsg);

#ifdef __cplusplus
}
#endif
#endif //__NET_PROTOCOL_H__


