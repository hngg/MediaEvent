

#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>

#include <jni.h>
#include <android/native_window_jni.h>


#include "utils.h"
#include "glog.h"
#include "rtcp.h"
#include "video_rtc_api.h"


#define JNI_API_NAME(A) 	Java_com_hnggpad_modtrunk_medialib_NativeVideoRtc_##A
#define JNI_VERSION 		JNI_VERSION_1_6

#define MAX_PROFILE_SIZE	256

typedef struct PROFILE_FRAME {
	char data[MAX_PROFILE_SIZE];
	int  size;
}PROFILE_FRAME;
PROFILE_FRAME gProfile;


JavaVM		*mJavaVm 			= NULL;//
jobject		 mCallbackObject 	= NULL;


jmethodID    mCallbackString;	//cmd string
jmethodID    mCallbackStatus;	//network status : rtt
jmethodID    mCallbackFiles;	//file status callback
jmethodID    mCallbackByteArray;//current not used,20211008


jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
	// Keep a reference on the Java VM.
	mJavaVm = vm;

	void *ptr = NULL;
	log_info("Compile: %s %s\n", __DATE__, __TIME__);
	log_info("JNI interface loaded double size:%d sys size:%d", sizeof(double), sizeof(ptr));
	
	return JNI_VERSION;
}

void JNI_OnUnload(JavaVM* vm, void* reserved) 
{
    log_info("JNI interface unloaded.");
}

//command callback function
int command_call(void*user, char* buffer, int size) 
{
	int status = -1;
	if(size>0 && buffer) {
		pjmedia_rtcp_common *rtcp = (pjmedia_rtcp_common*)buffer;
		if(rtcp->ssrc!=SPACAIL_SSRC)
			status = stringCallback(buffer, size);//update to java layer //ByteArrayCallback stringCallback
	}
	return status;
}

int commCreateCallback(JNIEnv *pEnv, jclass obj)
{
    int status = 0;
	mCallbackObject = (*pEnv)->NewGlobalRef(pEnv, obj);
	
	//find java callback function
	jclass pCallbackClass = (*pEnv)->GetObjectClass(pEnv, obj);
	if (NULL == pCallbackClass) 
	{   
		log_error("NULL == pCallbackClass");
		return -1;  
	}

	//command of string
    mCallbackString  = (*pEnv)->GetMethodID(pEnv, pCallbackClass, "onCommString", "(Ljava/lang/String;)V");//括号内的为参数，括号右边的是返回值
	if (NULL == mCallbackString)
	{   
		(*pEnv)->DeleteLocalRef(pEnv, pCallbackClass); //
		log_error("mCallbackString == NULL");
		return -2;  
	}
	//network status
	mCallbackStatus = (*pEnv)->GetMethodID(pEnv, pCallbackClass, "onNetworkStatus", "(IIIJI)V");//type rtt frame_rate lost_rate byte_count
	if (NULL == mCallbackStatus)
	{   
		(*pEnv)->DeleteLocalRef(pEnv, pCallbackClass); //
		log_error("mCallbackStatus == NULL");
		return -3;
	}
	//file send or recv status callback
	mCallbackFiles = (*pEnv)->GetMethodID(pEnv, pCallbackClass, "onFilesStatus", "(Ljava/lang/String;IIDI)V");
	if (NULL == mCallbackFiles)
	{   
		(*pEnv)->DeleteLocalRef(pEnv, pCallbackClass); //
		log_error("mCallbackFiles == NULL");
		return -4;
	}

	// mCallbackByteArray = (*pEnv)->GetMethodID(pEnv, pCallbackClass, "onCommRtc", "([BI)V");
	// if (NULL == mCallbackByteArray)
	// {  
	// 	(*pEnv)->DeleteLocalRef(pEnv, pCallbackClass); //
	// 	log_error("mCallbackByteArray == NULL");
	// 	return -3;  
	// }
    return status;
}

void commDestroyCallback(JNIEnv *pEnv)
{
	if(mCallbackObject) {
		(*pEnv)->DeleteGlobalRef(pEnv, mCallbackObject);
		mCallbackObject = NULL;
	}
}

//回调字节数组数据
int ByteArrayCallback(char*databuf, int datalen )
{
	jint result = -1;
	JNIEnv*		lenv;
	jobject		mobj;
	jclass		tmpClass;

	if(NULL == mCallbackObject)
	{
		log_error("mCallbackObject is null.");
		return result;
	}

	if(mJavaVm)
	{
		result = (*mJavaVm)->AttachCurrentThread( mJavaVm, &lenv, NULL);  
		if(NULL == lenv)
		{
			log_error("function: %s, line: %d, GetEnv failed!", __FUNCTION__, __LINE__);
			return (*mJavaVm)->DetachCurrentThread(mJavaVm);
		}
	}
	else
	{
		log_error("function: %s, line: %d, JavaVM is null!", __FUNCTION__, __LINE__);
		return result;
	}

	jbyteArray tranArray = (*lenv)->NewByteArray(lenv, datalen);  
	jbyte *bytes = (*lenv)->GetByteArrayElements(lenv, tranArray, 0);  
	//(*lenv)->SetByteArrayRegion(lenv, tranArray, 0, datalen, bytes ); 
	memcpy(bytes, databuf, datalen);
	
	(*lenv)->CallVoidMethod(lenv, mCallbackObject, mCallbackByteArray, tranArray, datalen);
	(*lenv)->ReleaseByteArrayElements(lenv, tranArray, bytes, 0);
	(*lenv)->DeleteLocalRef(lenv, tranArray);		//remember

	return (*mJavaVm)->DetachCurrentThread(mJavaVm);//must do that,else Native thread exited without calling DetachCurrentThread
}

//回调string数据
int stringCallback(const char*databuf, int datalen )
{
	jint result = -1;
	JNIEnv*		lenv;
	jobject		mobj;
	jclass		tmpClass;

	if(NULL == mCallbackObject)
	{
		log_error("mCallbackObject is null.");
		return result;
	}

	if(mJavaVm)
	{
		result = (*mJavaVm)->AttachCurrentThread( mJavaVm, &lenv, NULL);  
		if(NULL == lenv)
		{
			log_error("function: %s, line: %d, GetEnv failed!", __FUNCTION__, __LINE__);
			return (*mJavaVm)->DetachCurrentThread(mJavaVm);
		}
	}
	else
	{
		log_error("function: %s, line: %d, JavaVM is null!", __FUNCTION__, __LINE__);
		return result;
	}

	jstring str = (*lenv)->NewStringUTF(lenv, databuf);
	
	(*lenv)->CallVoidMethod(lenv, mCallbackObject, mCallbackString, str);

	return (*mJavaVm)->DetachCurrentThread(mJavaVm);	//must do that,else Native thread exited without calling DetachCurrentThread
}

int networkCallback( int type, int rtt, int frame_rate, long lost_rate, int byte_count )
{
	jint result = -1;
	JNIEnv*		lenv;
	jobject		mobj;
	jclass		tmpClass;

	if(NULL == mCallbackObject)
	{
		log_error("mCallbackObject is null.");
		return result;
	}

	if(mJavaVm)
	{
		result = (*mJavaVm)->AttachCurrentThread( mJavaVm, &lenv, NULL);  
		if(NULL == lenv)
		{
			log_error("function: %s, line: %d, GetEnv failed!", __FUNCTION__, __LINE__);
			return (*mJavaVm)->DetachCurrentThread(mJavaVm);
		}
	}
	else
	{
		log_error("function: %s, line: %d, JavaVM is null!", __FUNCTION__, __LINE__);
		return result;
	}
	
	(*lenv)->CallVoidMethod(lenv, mCallbackObject, mCallbackStatus, type, rtt, frame_rate, lost_rate, byte_count);

	return (*mJavaVm)->DetachCurrentThread(mJavaVm);	//must do that,else Native thread exited without calling DetachCurrentThread
}

int files_status_call(char*filename, int filetype, int fileid, double totalsize, int status)
{
	jint result = -1;
	JNIEnv*		lenv;
	jobject		mobj;
	jclass		tmpClass;

	if(NULL == mCallbackObject)
	{
		log_error("mCallbackObject is null.");
		return result;
	}

	if(mJavaVm)
	{
		result = (*mJavaVm)->AttachCurrentThread( mJavaVm, &lenv, NULL);  
		if(NULL == lenv)
		{
			log_error("function: %s, line: %d, GetEnv failed!", __FUNCTION__, __LINE__);
			return (*mJavaVm)->DetachCurrentThread(mJavaVm);
		}
	}
	else
	{
		log_error("function: %s, line: %d, JavaVM is null!", __FUNCTION__, __LINE__);
		return result;
	}
	
	jstring strName = (*lenv)->NewStringUTF(lenv, filename);

	(*lenv)->CallVoidMethod(lenv, mCallbackObject, mCallbackFiles, strName, filetype, fileid, totalsize, status);

	return (*mJavaVm)->DetachCurrentThread(mJavaVm);
}

//-----------------------------------------------------commands--------------------------------------------------->
JNIEXPORT jint JNICALL JNI_API_NAME(commRecvStart)(JNIEnv *env, jclass obj, jstring strLocalAddr, int localPort)
{
	int status = -1;
    jboolean isOk = JNI_FALSE;

    commCreateCallback(env, obj);

	const char* localAddr = (*env)->GetStringUTFChars(env, strLocalAddr, &isOk);
	status = med_command_recv_start(localAddr, localPort, &command_call);	//local
	(*env)->ReleaseStringUTFChars(env, strLocalAddr, localAddr);

	log_info("commRecvStart done, localAddr:%s port:%d status:%d", localAddr, localPort, status);

	return status;
}

JNIEXPORT jint JNICALL JNI_API_NAME(commRecvStop)(JNIEnv *env, jclass obj) 
{
	int status = 0;
	status = med_command_recv_stop();

    commDestroyCallback(env);

	log_info("commRecvStop done, status:%d", status);

	return status;
}

JNIEXPORT jint JNICALL JNI_API_NAME(commConnectSet)(JNIEnv *env, jclass obj, jstring saveFileDir, jstring strRemoteAddr, int remotePort) 
{
	int status = 0;

	jboolean isOk = JNI_FALSE;
	const char* save_dir   = (*env)->GetStringUTFChars(env, saveFileDir, &isOk);
    const char* remoteAddr = (*env)->GetStringUTFChars(env, strRemoteAddr, &isOk);

	status = med_command_connect_set(remoteAddr, remotePort);
	status = med_file_connect_set(remoteAddr, remotePort+1, save_dir, &files_status_call);//set file status callback

	(*env)->ReleaseStringUTFChars(env, saveFileDir, save_dir);
	(*env)->ReleaseStringUTFChars(env, strRemoteAddr, remoteAddr);

	log_info("commConnectSet done, save_dir:%s remoteAddr:%s rport:%d status:%d", 
			save_dir, remoteAddr, remotePort, status);

	return status;
}

JNIEXPORT jint JNICALL JNI_API_NAME(commFilesSender)(JNIEnv *env, jclass obj, jstring strFilePath, 
													jstring strFileName, int fileType, int taskId) 
{
	int status = 0;

	jboolean isOk = JNI_FALSE;
	const char* ch_path = (*env)->GetStringUTFChars(env, strFilePath, &isOk);
    const char* ch_name = (*env)->GetStringUTFChars(env, strFileName, &isOk);

	status = med_file_sender(ch_path, ch_name, fileType, taskId);
	log_info("commFilesSender path:%s name:%s type:%d taskid:%d", ch_path, ch_name, fileType, taskId);

	(*env)->ReleaseStringUTFChars(env, strFilePath, ch_path);
	(*env)->ReleaseStringUTFChars(env, strFileName, ch_name);

	return status;
}

JNIEXPORT jint JNICALL JNI_API_NAME(commFilesReceiver)(JNIEnv *env, jclass obj, jstring strFilePath, 
														jstring strFileName, int fileType, int taskId) 
{
	int status = 0;
	jboolean isOk = JNI_FALSE;
	const char* ch_path = (*env)->GetStringUTFChars(env, strFilePath, &isOk);
    const char* ch_name = (*env)->GetStringUTFChars(env, strFileName, &isOk);

	status = med_file_receiver(ch_path, ch_name, fileType, taskId);
	log_info("med_file_receiver path:%s name:%s type:%d taskid:%d", ch_path, ch_name, fileType, taskId);
	(*env)->ReleaseStringUTFChars(env, strFilePath, ch_path);
	(*env)->ReleaseStringUTFChars(env, strFileName, ch_name);
	return status;
}

JNIEXPORT jint JNICALL JNI_API_NAME(commDataSend)(JNIEnv *env, jclass obj, jbyteArray byteBuf, jint size) 
{
	int status = 0;

	char *data = (char*)(*env)->GetByteArrayElements(env, byteBuf, NULL);	//(char*)env->GetDirectBufferAddress(byteBuf);
	status = med_command_send(data, size);
	(*env)->ReleaseByteArrayElements(env, byteBuf, data,0);

	log_info("commDataSend size:%d status:%d", size, status);

	return status;
}


//发送udp数据
JNIEXPORT jint JNICALL JNI_API_NAME(commSendString)(JNIEnv *env, jclass obj, jstring data, jint size)
{
	char* pdata = NULL;
	int32_t result = -1;
	jboolean isCopy;
	pdata = (char*)(*env)->GetStringUTFChars(env, data, &isCopy );
	
	if (pdata)
		result = med_command_send(pdata, size);

	(*env)->ReleaseStringUTFChars( env, data, pdata );
	
	return result;
}

JNIEXPORT jstring JNICALL JNI_API_NAME(commClientAddr)(JNIEnv *env, jclass obj)
{
	char*clientAddr = med_command_client_addr();
	return (*env)->NewStringUTF(env, clientAddr);	//return value, not need release
}


///////////////////////////////////////////////////////rtp//////////////////////////////////////////////////////////////////////////

JNIEXPORT jint JNICALL JNI_API_NAME(rtpBindCreate)(JNIEnv *env, jclass obj, jstring strLocalAddr, int localPort, int codecType)
{
	int status = 0;
	jboolean isOk = JNI_FALSE;
	const char* localAddr = (*env)->GetStringUTFChars(env, strLocalAddr, &isOk);
	log_info("rtpBindCreate localAddr:%s localPort:%d", localAddr, localPort);
	status = vid_stream_create(localAddr, localPort, codecType);	//local address
    
	(*env)->ReleaseStringUTFChars(env, strLocalAddr, localAddr);
    
	log_info("rtpBindCreate done.");

	return status;
}

JNIEXPORT jint JNICALL JNI_API_NAME(rtpBindDestroy)(JNIEnv *env, jclass obj)
{
	int status = 0;
	status = vid_stream_destroy();

	log_info("rtpBindDestroy done.");

	return status;
}

JNIEXPORT jint JNICALL JNI_API_NAME(rtpRecvStart)(JNIEnv *env, jclass obj, jobject surface)
{
	int status = 0;
	ANativeWindow *pAnw = NULL;
	if(surface)
		pAnw = ANativeWindow_fromSurface(env, surface);
	status = vid_stream_recv_start_android(pAnw);	//local address
    
	log_info("rtpRecvStart done.");

	return status;
}

JNIEXPORT jint JNICALL JNI_API_NAME(rtpRecvStop)(JNIEnv *env, jclass obj)
{
	int status = 0;
	status = vid_stream_recv_stop();    //local address
    
	log_info("rtpRecvStop done.");

	return status;
}

JNIEXPORT jint JNICALL JNI_API_NAME(rtpRemoteConnect)(JNIEnv *env, jclass obj, jstring strRemoteAddr, int remotePort)
{
	int status = 0;
	jboolean isOk = JNI_FALSE;
    const char* remoteAddr = (*env)->GetStringUTFChars(env, strRemoteAddr, &isOk);
	status = vid_stream_remote_connect(remoteAddr, remotePort);
    (*env)->ReleaseStringUTFChars(env, strRemoteAddr, remoteAddr);
    
	vid_stream_network_callback(&networkCallback);

	log_info("rtpRemoteConnect done.");
	
	return status;
}

JNIEXPORT jint JNICALL JNI_API_NAME(sendSampleData)(JNIEnv *env, jclass obj, jobject byteBuf, jint offset, jint size, jlong ptus,  jint flags) 
{
	//jlong dstSize;
	char *data = (char*)(*env)->GetDirectBufferAddress(env, byteBuf);

	//log_info("sendSampleData dst type:%d dstSize:%d", data[4]&0x1F, size);

	switch(flags)
	{
		case 1:	//key frame
			if(gProfile.size>0 )
				packet_and_send(gProfile.data, gProfile.size);
		break;

		case 2://sps and pps frame, save to profile buffer
			memcpy(gProfile.data, data, size);
			gProfile.size = size;
		return;
	}
	
	return packet_and_send(data, size);
}


