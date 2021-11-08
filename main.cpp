 /*
* Copyright(C) 2011,Hikvision Digital Technology Co., Ltd 
* 
* File   name��main.cpp
* Discription��demo for muti thread get stream
* Version    ��1.0
* Author     ��luoyuhua
* Create Date��2011-12-10
* Modification History��
*/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "HCNetSDK.h"
#include "iniFile.h"
#include "PlayM4.h"

// ȡ�������Ϣ�������̴߳���
typedef struct tagREAL_PLAY_INFO
{
	char szIP[16];
	int iUserID;
	int iChannel;
}REAL_PLAY_INFO, *LPREAL_PLAY_INFO;

int iPicNum=0;//Set channel NO.
LONG nPort=-1;
FILE *Videofile=NULL;
FILE *Audiofile=NULL;
char filename[100];
HWND hPlayWnd=NULL;
// CString ErrorNum;

// �ص�ʵʱ�������ڵ���NET_DVR_SaveRealData���˴�������
void g_RealDataCallBack_V30(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer,DWORD dwBufSize,void* dwUser)
{
	//LPREAL_PLAY_INFO pPlayInfo = (LPREAL_PLAY_INFO)dwUser;
    //printf("[g_RealDataCallBack_V30]Get data, ip=%s, channel=%d, handle=%d, data size is %d, thread=%d\n", 
		//pPlayInfo->szIP, pPlayInfo->iChannel, lRealHandle, dwBufSize, pthread_self());

	//printf("[g_RealDataCallBack_V30]Get data, handle=%d, data size is %d, thread=%d\n", 
		//lRealHandle, dwBufSize, pthread_self());
	//NET_DVR_SaveRealData(lRealHandle, cFilename);
}

FILE *g_pFile = NULL;

void PsDataCallBack(LONG lRealHandle, DWORD dwDataType,BYTE *pPacketBuffer,DWORD nPacketSize, void* pUser)
{

	if (dwDataType  == NET_DVR_SYSHEAD)
	{	
		//д��ͷ����
		g_pFile = fopen("./record/ps.dat", "wb");
		
		if (g_pFile == NULL)
		{
			printf("CreateFileHead fail\n");
			return;
		}

		//д��ͷ����
		fwrite(pPacketBuffer, sizeof(unsigned char), nPacketSize, g_pFile);
		printf("write head len=%d\n", nPacketSize);
	}
	else
	{
		if(g_pFile != NULL)
		{
			fwrite(pPacketBuffer, sizeof(unsigned char), nPacketSize, g_pFile);
			printf("write data len=%d\n", nPacketSize);
		}
	}	

}

//////////////////////////////////////////////////////////////////////////
////解码回调 视频为YUV数据(YV12)，音频为PCM数据
void DecCBFun(int nPort,char * pBuf,int nSize,FRAME_INFO * pFrameInfo, void* nReserved1,int nReserved2)
{
 	long lFrameType = pFrameInfo->nType;	
	if (lFrameType ==T_AUDIO16)
	{
		// TRACE("Audio nStamp:%d\n",pFrameInfo->nStamp);
		// OutputDebugString("test_DecCb_Write Audio16 \n");
		if (Audiofile==NULL)
		{
			sprintf(filename,"AudionPCM.pcm",iPicNum);
			Audiofile = fopen(filename,"wb");
		}
		fwrite(pBuf,nSize,1,Audiofile);
	}

	else if(lFrameType ==T_YV12)
	{		
	    // TRACE("Video nStamp:%d\n",pFrameInfo->nStamp);
		// OutputDebugString("test_DecCb_Write YUV \n");
		if (Videofile==NULL)
		{
			sprintf(filename,"VideoYV12.yuv",iPicNum);
			Videofile = fopen(filename,"wb");
		}
		fwrite(pBuf,nSize,1,Videofile);
	}
	else
	{

	}
}


//////////////////////////////////////////////////////////////////////////
///实时流回调
void fRealDataCallBack(LONG lRealHandle,DWORD dwDataType,BYTE *pBuffer,DWORD dwBufSize,void *pUser)
{
	DWORD dRet = 0;
	BOOL inData = FALSE;

	switch (dwDataType)
	{
	case NET_DVR_SYSHEAD:

		// std::cout << "Hello! NET_DVR_SYSHEAD" << std::endl;
		if (!PlayM4_GetPort(&nPort))
		{
			break;
		}
		if (!PlayM4_OpenStream(nPort,pBuffer,dwBufSize,1024*1024))
		{
			dRet=PlayM4_GetLastError(nPort);
			break;
		}

		//设置解码回调函数 只解码不显示
// 		if (!PlayM4_SetDecCallBack(nPort,DecCBFun))
// 		{
// 			dRet=PlayM4_GetLastError(nPort);
// 			break;
// 		}
		
		//设置解码回调函数 解码且显示
		if (!PlayM4_SetDecCallBackEx(nPort,DecCBFun,NULL,NULL))
		{
			dRet=PlayM4_GetLastError(nPort);
			break;
		}

		//打开视频解码
		if (!PlayM4_Play(nPort,hPlayWnd))
		{
			dRet=PlayM4_GetLastError(nPort);
			break;
		}

		//打开音频解码, 需要码流是复合流
		if (!PlayM4_PlaySound(nPort))
		{
			dRet=PlayM4_GetLastError(nPort);
			break;
		}
		break;
		
	// case NET_DVR_STREAMDATA:
    //     inData=PlayM4_InputData(nPort,pBuffer,dwBufSize);
	// 	while (!inData)
	// 	{
	// 		Sleep(10);
	// 		inData=PlayM4_InputData(nPort,pBuffer,dwBufSize);
	// 		OutputDebugString("PlayM4_InputData failed \n");	
	// 	}
	// 	break;
	// default:
	// 	inData=PlayM4_InputData(nPort,pBuffer,dwBufSize);
	// 	while (!inData)
	// 	{
	// 		Sleep(10);
	// 		inData=PlayM4_InputData(nPort,pBuffer,dwBufSize);
	// 		OutputDebugString("PlayM4_InputData failed \n");	
	// 	}
	// 	break;
	}
}








void GetStream()
{
	// �������ļ���ȡ�豸��Ϣ 
	IniFile ini("Device.ini");
	unsigned int dwSize = 0;
	char sSection[16] = "DEVICE";

	
	char *sIP = ini.readstring(sSection, "ip", "error", dwSize);
	int iPort = ini.readinteger(sSection, "port", 0);
	char *sUserName = ini.readstring(sSection, "username", "error", dwSize); 
	char *sPassword = ini.readstring(sSection, "password", "error", dwSize);
	int iChannel = ini.readinteger(sSection, "channel", 0);
		
	NET_DVR_DEVICEINFO_V30 struDeviceInfo;
	int iUserID = NET_DVR_Login_V30(sIP, iPort, sUserName, sPassword, &struDeviceInfo);
	if(iUserID >= 0)
	{

    		//NET_DVR_CLIENTINFO ClientInfo = {0};
    		//ClientInfo.lChannel     = iChannel;  //channel NO.
    		//ClientInfo.lLinkMode    = 0;
    		//ClientInfo.sMultiCastIP = NULL;
    		//int iRealPlayHandle = NET_DVR_RealPlay_V30(iUserID, &ClientInfo, PsDataCallBack, NULL, 0);
		NET_DVR_PREVIEWINFO struPreviewInfo = {0};
		struPreviewInfo.lChannel =iChannel;
		struPreviewInfo.dwStreamType = 0;
		struPreviewInfo.dwLinkMode = 0;
		struPreviewInfo.bBlocked = 1;
		struPreviewInfo.bPassbackRecord  = 1;
		int iRealPlayHandle = NET_DVR_RealPlay_V40(iUserID, &struPreviewInfo, fRealDataCallBack, NULL);
		if(iRealPlayHandle >= 0)
		{
			printf("[GetStream]---RealPlay %s:%d success, \n", sIP, iChannel, NET_DVR_GetLastError());
			//int iRet = NET_DVR_SaveRealData(iRealPlayHandle, "./record/realplay.dat");
			//NET_DVR_SetStandardDataCallBack(iRealPlayHandle, StandardDataCallBack, 0);

		}
		else
		{
			printf("[GetStream]---RealPlay %s:%d failed, error = %d\n", sIP, iChannel, NET_DVR_GetLastError());
		}
	}
	else
	{
		printf("[GetStream]---Login %s failed, error = %d\n", sIP, NET_DVR_GetLastError());
	}
}


int main()
{
    NET_DVR_Init();
	NET_DVR_SetLogToFile(3, "./record/");
	
	GetStream();
	

	char c = 0;
	while('q' != c)
	{
		printf("input 'q' to quit\n");
		printf("input: ");
		scanf("%c", &c);
	}


    NET_DVR_Cleanup();
    return 0;
}


