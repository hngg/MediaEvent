// NALDecoder.cpp : Defines the entry point for the console application.
//


#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "NALDecoder.h"

FILE *bits = NULL;                //!< the bit stream file
static int info2=0, info3=0;
static int FindStartCode2 (unsigned char *Buf);//查找开始字符0x000001
static int FindStartCode3 (unsigned char *Buf);//查找开始字符0x00000001

static int FindStartCode2 (unsigned char *Buf) {
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=1)
		return 0; //判断是否为0x000001,如果是返回1
	else
		return 1;
}

static int FindStartCode3 (unsigned char *Buf) {
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=0 || Buf[3] !=1)
		return 0;//判断是否为0x00000001,如果是返回1
	else
		return 1;
}

//为NALU_t结构体分配内存空间
NALU_t *AllocNALU(int buffersize)
{
	NALU_t *n;

	if ((n = (NALU_t*)calloc (1, sizeof (NALU_t))) == NULL)
	{
		printf("AllocNALU: n");
		exit(0);
	}

	n->max_size=buffersize;

	if ((n->buf = (unsigned char*)calloc (buffersize, sizeof (char))) == NULL)
	{
		free (n);
		printf ("AllocNALU: n->buf");
		exit(0);
	}

	return n;
}

//释放
void FreeNALU(NALU_t *n)
{
	if (n)
	{
	if (n->buf)
	{
		free(n->buf);
		n->buf=NULL;
	}
	free (n);
	}
}

FILE* OpenBitstreamFile (const char *filename) {
	FILE* file = NULL;
	if (NULL == (file=fopen(filename, "rb")))
	{
		printf("open file error\n");
		exit(0);
	}
	return file;
}

void  CloseBitstreamFile(FILE*file) {
	if(file!=NULL)
		fclose(file);
}

//这个函数输入为一个NAL结构体，主要功能为得到一个完整的NALU并保存在NALU_t的buf中，获取他的长度，填充F,IDC,TYPE位。
//并且返回两个开始字符之间间隔的字节数，即包含有前缀的NALU的长度
int GetAnnexbNALU (FILE *file, NALU_t *nalu)
{
	int pos = 0;
	int StartCodeFound, rewind;
	unsigned char *Buf;

	if ((Buf = (unsigned char*)calloc (nalu->max_size , sizeof(char))) == NULL)
		printf ("GetAnnexbNALU: Could not allocate Buf memory\n");

	nalu->startcodeprefix_len=3;//初始化码流序列的开始字符为3个字节

	if (3 != fread (Buf, 1, 3, file))//从码流中读3个字节
	{
		free(Buf);
		return 0;
	}
	info2 = FindStartCode2 (Buf);//判断是否为0x000001
	if(info2 != 1)
	{
		//如果不是，再读一个字节
		if(1 != fread(Buf+3, 1, 1, file))//读一个字节
		{
			free(Buf);
			return 0;
		}
		info3 = FindStartCode3 (Buf);//判断是否为0x00000001
		if (info3 != 1)//如果不是，返回-1
		{
			free(Buf);
			return -1;
		}
		else
		{
			//如果是0x00000001,得到开始前缀为4个字节
			pos = 4;
			nalu->startcodeprefix_len = 4;
		}
	}
	else
		{
			//如果是0x000001,得到开始前缀为3个字节
			nalu->startcodeprefix_len = 3;
			pos = 3;
		}
	//查找下一个开始字符的标志位
	StartCodeFound = 0;
	info2 = 0;
	info3 = 0;

	int headLen = nalu->startcodeprefix_len;

	while (!StartCodeFound)
	{
		if (feof (file))//判断是否到了文件尾
		{
				nalu->len = (pos-1)-nalu->startcodeprefix_len;
				memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len-headLen], nalu->len+headLen);
				nalu->forbidden_bit = nalu->buf[0+headLen] & 0x80; 		// 1 bit
				nalu->nal_reference_idc = nalu->buf[0+headLen] & 0x60; 	// 2 bit
				nalu->nal_unit_type = (nalu->buf[0+headLen]) & 0x1f;	// 5 bit
				free(Buf);
				return pos-1;
		}
		Buf[pos++] = fgetc (file);//读一个字节到BUF中
		info3 = FindStartCode3(&Buf[pos-4]);//判断是否为0x00000001
		if(info3 != 1)
			info2 = FindStartCode2(&Buf[pos-3]);//判断是否为0x000001
		StartCodeFound = (info2 == 1 || info3 == 1);
	}

	// Here, we have found another start code (and read length of startcode bytes more than we should
	// have.  Hence, go back in the file
	rewind = (info3 == 1)? -4 : -3;

	if (0 != fseek (file, rewind, SEEK_CUR))//把文件指针指向前一个NALU的末尾
	{
			free(Buf);
			printf("GetAnnexbNALU: Cannot fseek in the bit stream file");
	}

	// Here the Start code, the complete NALU, and the next start code is in the Buf.
	// The size of Buf is pos, pos+rewind are the number of bytes excluding the next
	// start code, and (pos+rewind)-startcodeprefix_len is the size of the NALU excluding the start code

	nalu->len = (pos+rewind)-nalu->startcodeprefix_len;
	memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len-headLen], nalu->len+headLen);//拷贝一个完整NALU，不拷贝起始前缀0x000001或0x00000001
	nalu->forbidden_bit = nalu->buf[0+headLen] & 0x80; 			// 1 bit
	nalu->nal_reference_idc = nalu->buf[0+headLen] & 0x60; 		// 2 bit
	nalu->nal_unit_type = (nalu->buf[0+headLen]) & 0x1f;		// 5 bit
	free(Buf);

	return (pos+rewind);//返回两个开始字符之间间隔的字节数，即包含有前缀的NALU的长度
}

void file_exten(const char*fileName, char*outExten) {
    int i=0,length= strlen(fileName);
    while(fileName[i]) {
        if(fileName[i]=='.')
            break;
            i++;
    }
    if(i<length)
        strcpy(outExten, fileName+i+1);
    else
        strcpy(outExten, "\0");
}

static int frameRunning = 0;

int start_file_frame_out(char*filename, int codecType, on_media_frame frame_cb)
{
    FILE			*pFile;
	NALU_t 			*mNALU;
    char idrBuf[2*1024*1024] = {0};

	if(frameRunning)
		return 0;
	else
		frameRunning = 1;
	
	pFile = OpenBitstreamFile( filename ); //camera_640x480.h264 //camera_1280x720.h264
	if(pFile==NULL) {
		printf("open file:%s failed.\n", filename);
		return 0;
	}

    //vid_stream_create();

	mNALU  = AllocNALU(8000000);

	int count = 1, cpyPos = 0;
	do {
		
		int size=GetAnnexbNALU(pFile, mNALU);//每执行一次，文件的指针指向本次找到的NALU的末尾，下一个位置即为下个NALU的起始码0x000001
		printf("\n------GetAnnexbNALU type:0x%02X  nal size:%d count:%d------\n", mNALU->buf[4], size, count);
		if(size<4) {
			printf("get nul error!\n");
			continue;
		}

        int type = (codecType==H264_HARD_CODEC)?(mNALU->buf[4] & 0x1F):(((mNALU->buf[4])>>1) & 0x3F) ;//GET_H265_NAL_TYPE(mNALU->buf[4]);
        if(type==7 || type==8 || type==32 || type==33 || type==34) {
            memcpy(idrBuf+cpyPos, mNALU->buf, size);
            cpyPos += size;
            continue;
        }

		count++;

        memcpy(idrBuf+cpyPos, mNALU->buf, size);
        cpyPos += size;
		printf("GetAnnexbNALU frame size:%d\n", cpyPos);

		if(frame_cb)
			frame_cb(idrBuf, cpyPos, codecType);

        switch(codecType) {
            case H264_HARD_CODEC:
                //h264_test(mNALU->buf, size);
                //h264_package_test(idrBuf, cpyPos);
                //packet_and_send_test((pj_uint8_t *)idrBuf, cpyPos);
				//rtp_packet_recv_test(idrBuf, cpyPos);
                break;
            case H265_HARD_CODEC:
                //h265_test(mNALU->buf, size);
				//packet_and_send_test((pj_uint8_t *)idrBuf, cpyPos);
                break;
        }
        cpyPos = 0;
	}while(!feof(pFile) && frameRunning);

	if(pFile != NULL)
		fclose(pFile);

	FreeNALU(mNALU);
	frameRunning = 0;

    //vid_stream_destroy();
	return 0;
}

int stop_file_frame_out() {
	frameRunning = 0;
	return 0;
}


//输出NALU长度和TYPE
void dump(NALU_t *n)
{
	if (!n)return;
	//printf("a new nal:");
	printf(" len: %d  ", n->len);
	printf("nal_unit_type: %x\n", n->nal_unit_type);
	//printf("%#X %#X %#X %#X \n",n->buf[0],n->buf[1],n->buf[2],n->buf[3]);
}

