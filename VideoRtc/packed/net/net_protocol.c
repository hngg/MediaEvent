/*==========================================================================
 *FILENAME:      net_protocol.c
 *
 *DESCRIPTION:   net_protocol Module
 *
 *AUTHOR:        ming jiawan              
 *
 *DATE:          2011-01-01
 *
 *COMPANY:      	 
===========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "protocol.h"
#include "net_protocol.h"
#include "basedef.h"


int int2hex2str(char *pValue,int lValue,int lCharLen)
{
    char tmp[10]={0};
    memset(tmp,0,sizeof(tmp));
    sprintf(tmp,"%%0%dx",lCharLen);
    return sprintf(pValue,tmp,lValue);
}
int int2str(char *pValue,int lValue,int lCharLen)
{
    char tmp[10];
    memset(tmp,0,sizeof(tmp));
    sprintf(tmp,"%%0%dd",lCharLen);
    return sprintf(pValue,tmp,lValue);
}

int  PROTO_MakeDataHead(PROTO_ST_MSG *stMsg,int cmd)
{
    //�Զ��������ݴ�С
    unsigned char *p=NULL;
    char acLen[7]={0};
    char acCmd[5]={0};
    int2str(acCmd, cmd,PROTOCOL_CMD_LENGTH);

    if(4!=strlen(acCmd))
    {
        GLOGE( "acCmd length not right \n ");
        return 0;
    }
    p = stMsg->msg;
    if(p)
    {
        memset(p,0,stMsg->total);
        memcpy(p,PROTOCOL_VERSION,PROTOCOL_VERSION_LENGTH);  //�汾��4���ֽ�
        p += PROTOCOL_VERSION_LENGTH;
        memcpy(p,acCmd,PROTOCOL_CMD_LENGTH);            //�����4���ֽ�
        p += PROTOCOL_CMD_LENGTH;
        int2hex2str(acLen,0,PROTOCOL_LOAD_LENGTH);
        //sprintf(acLen,"%06x",0);
        memcpy(p,acLen,PROTOCOL_LOAD_LENGTH);         //���ݳ���6���ֽ�
        stMsg->len = PROTOCOL_HEAD_LENGTH;
    }
    else
    {
        stMsg->len = 0;
        GLOGE( "malloc fail\n");
    }
    return stMsg->len;
    
}
int PROTO_GetPackDataLen(char *pDataHead)
{
	NET_CMD *stCmd = (NET_CMD*)pDataHead;
	unsigned int len;

#ifdef MY_LITTLE_ENDIAN /*2018.10.17*/
    len = (stCmd->dwLength);
#else
	len = ntohl(stCmd->dwLength);
#endif
	//LOGE("%s:%d, len=%d, stCmd->dwLength=%d", __FUNCTION__, __LINE__, len, stCmd->dwLength);

    return len;
}

int PROTO_SetDataPackLen(char *pDataHead,int len)
{
    char acLen[PROTOCOL_LOAD_LENGTH+1]={0};
    memset(acLen,0,sizeof(acLen));
    int2hex2str(acLen,len,PROTOCOL_LOAD_LENGTH);
    memcpy(pDataHead+PROTOCOL_LOAD_OFFSET,acLen,PROTOCOL_LOAD_LENGTH);
    return len;
    
}

int PROTO_AddOneItem(PROTO_ST_MSG *stMsg,char *pName,char *pValue,int lValueLen)
{
    int lNewLen;
    int lNameLen;
    char acLen[PROTOCOL_LOAD_LENGTH+1]={0};
    unsigned char *pTmptr=NULL;

    if(stMsg->len < PROTOCOL_LOAD_LENGTH)
    {
        GLOGE( "not have head\n");
        return -1;
    }
    if(pName == NULL)
    {
        GLOGE( "name is not right!\n");
        return -1;
    }
    lNameLen = strlen(pName);
     //�¿ռ�Ĵ�С
    lNewLen = 2+6+lNameLen +lValueLen;
    pTmptr = stMsg->msg+stMsg->len;
    //��ֵ
    if(stMsg->msg)
    {
        if(lNameLen > 0xFF)
        {
            return -1;
        }
        if(lValueLen > 0xFFFFFF)
        {
            return -1;
        }
        //����������
        memset(acLen,0,sizeof(acLen));
        int2hex2str(acLen,lNameLen,2);
        memcpy(pTmptr,acLen,2);
        pTmptr+=2;
        //����ֵ����
        memset(acLen,0,sizeof(acLen));
        int2hex2str(acLen,lValueLen,6);
        memcpy(pTmptr,acLen,6);
        pTmptr+=6;
        //������
        memcpy(pTmptr,pName,lNameLen);
        pTmptr+=lNameLen;
        //����ֵ
        if(pValue != NULL)
        	memcpy(pTmptr,pValue,lValueLen);
        //���һ����������ͷ�����ݴ�Сֵ
        stMsg->len+=lNewLen;

        PROTO_SetDataPackLen((char*)stMsg->msg, stMsg->len-PROTOCOL_HEAD_LENGTH);
    }
    else
    {
        GLOGE( "malloc fail\n");
        return -1;
    }
    return stMsg->len;
}

int PROTO_AddOneItemInt(PROTO_ST_MSG *stMsg,char *pName, int nValue)
{
    char acValue[16] = {0};
    int lValueLen = 0;
    lValueLen = snprintf(acValue, 16, "%d", nValue);
    return PROTO_AddOneItem(stMsg, pName, acValue, lValueLen);
}

int PROTO_GetCommand(char *pData)
{
	NET_CMD *stCmd = (NET_CMD*)pData;
	unsigned int cmd;


    GLOGE("stCmd->dwFlag=0x%x, stCmd->dwCmd=0x%x, stCmd->dwIndex=0x%x, stCmd->dwLength=0x%x \n", stCmd->dwFlag, stCmd->dwCmd, stCmd->dwIndex, stCmd->dwLength);

#ifdef MY_LITTLE_ENDIAN /*2018.10.17*/
    cmd = (stCmd->dwCmd);
#else
	cmd = ntohl(stCmd->dwCmd);
#endif
	
	return cmd;
}

/*
XML data
*/
int PROTO_GetValueByName(char *pData,char *pName,char *pValueOut,int *pValueLen)
{
    char *ptr;
	char *p;
	int pNameLen;
	NET_CMD *stCmd = (NET_CMD*)pData;

	pNameLen = strlen(pName);
	//GLOGE("pNameLen:%d, lpData:%s \n", pNameLen, stCmd->lpData);

    ptr = stCmd->lpData;
	p = strstr(ptr, pName);

	if(NULL != p)
	{
		//GLOGE("find string\n");
		if(*(p + pNameLen) == '=')
		{
			//LOGD("find = symbol. \n");
			int i = 0;
			p = p + pNameLen + 2;
			while(*(p + i) != '\"')
			{
				i++;
			}
			//*pValue = p;
			*pValueLen = i-1;
			memcpy(pValueOut, p, i);
			//GLOGE("pValue:%s , pValueLen:%d \n", *pValue, *pValueLen);
			//printf("p:%s\n", p);
			return 1;
		}
		*pValueLen = 0;
		return 0;
	}
	*pValueLen = 0;
	
    return 0;
    
}

int  PROTO_MakeRegistMsg(PROTO_ST_MSG *stMsg,char *pcDeviceSerial,char *pcDeviceName)
{
    int ret;
    
    ret = PROTO_MakeDataHead(stMsg,PROTOCOL_REGIST);
    ret = PROTO_AddOneItem(stMsg,"DeviceSerial",pcDeviceSerial,strlen(pcDeviceSerial));
    ret = PROTO_AddOneItem(stMsg,"DeviceName",pcDeviceName,strlen(pcDeviceName));
    return ret;
}

int PROTO_MakeSdpMsg(PROTO_ST_MSG *stMsg,char*sdp,int sdp_len)
{
    int ret;
    
    ret = PROTO_MakeDataHead(stMsg,PROTOCOL_REPLY_ASKSDP);
    ret = PROTO_AddOneItem(stMsg,"Ret","1",1);
    ret = PROTO_AddOneItem(stMsg,"Data",sdp,sdp_len);
    GLOGD( "SDP = %s\n",stMsg->msg);
    return ret;
    
}

int PROTO_MakeClientMsg(PROTO_ST_MSG *stMsg,char *pData, char *pName)
{
    unsigned char *pcValue;
    int lValueLen;
    int ret;
    unsigned char ClientID[32]={0};
    
    PROTO_GetValueByName(pData, pName, (char*)pcValue,&lValueLen);	//origin (char**)&pcValue changed datetime:20190131
	if(pcValue)
	{
		memcpy(ClientID, pcValue, lValueLen);
	}				
	ret = PROTO_MakeDataHead(stMsg, PROTOCOL_REPLY_TRANSFER);
	if(ret != PROTOCOL_HEAD_LENGTH)
	{
		return -1;
	}
	
	if(pcValue)
	{
		ret = PROTO_AddOneItem(stMsg, pName, (char*)ClientID, lValueLen);
	}    

    return 0;
}

int PROTO_MakeAVDataMsg(PROTO_ST_MSG *stMsg,char*data,char *type,int data_len)
{
    int ret;
    ret = PROTO_MakeDataHead(stMsg,PROTOCOL_REPLY_ASKDATA);
    ret = PROTO_AddOneItem(stMsg,"Ret","1",1);
    if(ret<0)
    {
        GLOGE( "error\n");
        return -1;
    }
    ret = PROTO_AddOneItem(stMsg,"Type",type,strlen(type));
    if(ret<0)
    {
        GLOGE( "error\n");
        return -2;
    }
    ret = PROTO_AddOneItem(stMsg,"Data",data,data_len);
    if(ret<0)
    {
        GLOGE( "error\n");
        return -3;
    }
    
    return 0;
}
int PROTO_MakeHeartBeatMsg(PROTO_ST_MSG *stMsg)
{
    int ret;
    
    ret = PROTO_MakeDataHead(stMsg,PROTOCOL_HEARTBEAT);
    return ret;
       
}
int PROTO_MakeReplyUpTime(PROTO_ST_MSG *stMsg,int result)
{
    int ret;
    
    ret = PROTO_MakeDataHead(stMsg,PROTOCOL_REPLY_TIME);
    ret = PROTO_AddOneItem(stMsg,"Ret","1",1);
    return ret;
}
int PROTO_MakeUpTime(PROTO_ST_MSG *stMsg)
{
    int ret;
    
    ret = PROTO_MakeDataHead(stMsg,PROTOCOL_UPDATE_TIME);
    return ret;
}

int PROTO_DestoryMsg(PROTO_ST_MSG *msg)
{
    free(msg->msg);
    msg->len = 0;
    msg->total = 0;
    return 0;
}


//int GetResult(int dwMsg,char *pData, int nLength,void* lpResult,int* nResult)
//{
//	int ret;
//	char *p = NULL;
//	char acValue[256] = {0};
//	int lValueLen;
//	if(dwMsg == MODULE_MSG_LOGIN)
//	{
//		ret = PROTO_GetValueByName(pData, "play path", &p, &lValueLen);
//		if (!ret)
//		{
//			*nResult = 0;
//			return ERR_NONEEDXML;
//		}
//		memcpy(acValue, p, lValueLen);
//        GLOGD("lValueLen:%d, acValue:%s \n",lValueLen, (char *)acValue);
//
//
//		memset(lpResult,0,*nResult);
//		int hRecorder = MP4File_OpenFile(acValue,lpResult,nResult);
//		if(!hRecorder)
//		{
//			*nResult = 0;
//			return ERR_NOOBJECT;
//		}
//		MP4File_CloseFile(hRecorder);
//
//        return ERR_NOERROR;
//	}
//
//	return ERR_NOSURPORT;
//}
