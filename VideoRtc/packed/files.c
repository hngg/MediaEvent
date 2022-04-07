

#include "files.h"
#include "glog.h"
#include "comm_media.h"


inline int setBitOn(int *value, int pos)
{
	*value = 1<<(31-pos) | *value;
	return *value;
}

int build_comm_file_head(char*buffer, int pt)
{
    pjmedia_common_head *head = (pjmedia_common_head*)buffer;
    head->pt = pt;
    head->version   = COMM_VERS;
    head->ssrc      = COMM_MAKR;
    return sizeof(pjmedia_common_head);
}

int add_comm_file_nack_node(char*buffer, pj_uint16_t baseSeq, pj_uint16_t lostNum)
{
    pjmedia_file_nack_node *node = (pjmedia_file_nack_node*)buffer;
    node->baseSeq = baseSeq;
    node->lostNum = lostNum;
    return sizeof(pjmedia_file_nack_node);
}

int buildLostPackage(int *markRow, int maxSeq, char*buffer)
{
    pjmedia_common_head *head = (pjmedia_common_head*)buffer;
    head->pt        = FILE_NACK_REQ_SYN;
    head->version   = COMM_VERS;
    head->ssrc      = COMM_MAKR;
    pjmedia_file_nack_info *nackInfo = (pjmedia_file_nack_info*)(buffer+sizeof(pjmedia_common_head));
    pjmedia_file_nack_node*node = NULL;

    int bufLen   = 0, pos = 0, baseSeq = 0, lostNum = 0;
    int segIndex = maxSeq>>5, tailMod = maxSeq%32, i=0, j=0;
    bufLen = sizeof(pjmedia_common_head)+sizeof(pjmedia_file_nack_info);
    nackInfo->node_count = 0;

    for(; i<segIndex; i++)
    {
        if(markRow[i]==0xffffffff) 
        {
            pos+=32;
            if(lostNum>0)
            {
                node = (pjmedia_file_nack_node*)(buffer+bufLen);
                node->baseSeq = baseSeq;
                node->lostNum = lostNum;
                nackInfo->node_count++;
                bufLen += FILE_NACK_NODE_LENGTH;
                log_info("baseseq:%d number:%d", baseSeq, lostNum);
            }
            lostNum = 0;
            baseSeq = 0;
        }
        else 
        {
            for(j=0; j<32; j++) 
            {
                int res = markRow[i]>>(31-j) & 0x1;
                if(res==0)
                {
                    //lostNum=0 防止第一个为0和第二个也为0时baseSeq变为1, 比如001101110001
                    if(baseSeq==0 && lostNum==0)
                        baseSeq = pos;
                    lostNum++;
                }else
                {
                    if(lostNum>0)
                    {
                        node = (pjmedia_file_nack_node*)(buffer+bufLen);
                        node->baseSeq = baseSeq;
                        node->lostNum = lostNum;
                        nackInfo->node_count++;
                        bufLen += FILE_NACK_NODE_LENGTH;
                        log_info("baseseq:%d number:%d", baseSeq, lostNum);
                    }
                    lostNum = 0;
                    baseSeq = 0;
                }
                pos++;
            }
        }//if
    }//for

    if(tailMod)
    {
        for(j=0; j<tailMod; j++) {
		    int res = markRow[i]>>(31-j) & 0x1;
            if(res==0)
            {
                if(baseSeq==0)
                    baseSeq = pos;
                lostNum++;
                //最后一个刚好为0
                if(j==(tailMod-1))
                {
                    node = (pjmedia_file_nack_node*)(buffer+bufLen);
                    node->baseSeq = baseSeq;
                    node->lostNum = lostNum;
                    nackInfo->node_count++;
                    bufLen += FILE_NACK_NODE_LENGTH;
                    log_info("baseseq:%d number:%d", baseSeq, lostNum);
                }
            }else
            {
                if(lostNum>0)
                {
                    node = (pjmedia_file_nack_node*)(buffer+bufLen);
                    node->baseSeq = baseSeq;
                    node->lostNum = lostNum;
                    nackInfo->node_count++;
                    bufLen += FILE_NACK_NODE_LENGTH;
                    log_info("baseseq:%d number:%d", baseSeq, lostNum);
                }
                lostNum = 0;
                baseSeq = 0;
            }
            pos++;
	    }//for
    }
    //printf("node_count:%d\n", nackInfo->node_count);
    return (nackInfo->node_count>0)?bufLen:0;//没有添加nack节点时就返回0
}

/////////////////////////////////////////////////test//////////////////////////////////////////////

int showIntBit(int value, int length)
{
	int i=0;
	for(; i<length; i++) {
		int res = value>>(31-i) & 0x1;
        #ifdef __ANDROID__
		    log_info("%d", res);
        #else
            printf("%d", res);
        #endif
	}
    #ifdef __ANDROID__
        log_info("\n");
    #else
        printf("\n");
    #endif

	return i;
}

void printBitMark(int *markRow, int maxSeq)
{
    log_info("\n");
    int segIndex = maxSeq>>5, tailMod = maxSeq%32, i=0;
    for(; i<segIndex; i++)
    {
        showIntBit(markRow[i], 32);
    }

    if(tailMod)
    {
        showIntBit(markRow[segIndex], tailMod);
    }
    log_info("\n");
}



