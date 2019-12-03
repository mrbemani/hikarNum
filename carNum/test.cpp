/************************************************************************
* Copyright(c) 2018
*
* File:		��������_0.3.c
* Brief:	���ú�������sdk��ȡ���ƺ�
* Version: 	0.3
* Author: 	һ������
* Email:	zhunmeng@live.com
* Date:    	2018.02.26
* History:
2018.2.26 	���ú�������sdk��ȡ���ƺ�
2018.3.7 	��ȡ�豸ϵͳʱ��
2018.3.12   ���泵�ƺŵ�csv�ļ�
2018.3.14	��ʶ�𵽵ĳ��ƺŽ��а������ȶ�

************************************************************************/
//-------------------------------------------------------------------------
//ͷ�ļ�

#include <iostream>
#include <fstream> 
#include <string>
#include <streambuf>
#include "HCNetSDK.h"
#include <time.h>

#pragma comment(lib,"HCNetSDK.lib")
#pragma comment(lib,"PlayCtrl.lib")
#pragma comment(lib,"GdiPlus.lib")
#pragma comment(lib,"HCCore.lib")

using namespace std;


//��������
LONG IUserID;	//������豸
NET_DVR_DEVICEINFO_V30 struDeviceInfo;	//�豸��Ϣ


char sDVRIP[20];	//ץ��������豸IP��ַ
short wDVRPort = 8000;	//�豸�˿ں�
char sUserName[20] = { 0 };	//��¼���û���
char sPassword[20] = { 0 };	//�û�����
string carNum;//���ƺ�							


				  //---------------------------------------------------------------------------------
				  //��������
void Init(void);//��ʼ��
void Show_SDK_Version(); //��ȡsdk�汾
void Connect();//���������¼�������ʱ��
void Htime();//��ȡ���������豸ʱ��
bool Login(char *sDVRIP, short wDVRPort, char *sUserName, char *sPassword);//ע��������豸
void manualSnap();
void OnExit(void);//�˳�
				  //---------------------------------------------------------------------------------------------------
				  //��������
				  //��ʼ��
void Init(void)
{
	//��ȡϵͳʱ��
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	cout << sys.wYear << "-" << sys.wMonth << "-" << sys.wDay << " " << sys.wHour << ":" << sys.wMinute << ":" << sys.wSecond << endl;

		
	NET_DVR_Init();//��ʼ��
	Show_SDK_Version();//��ȡ SDK  �İ汾�ź� build  ��Ϣ	
}

//���������¼�������ʱ��
void Connect()
{
	NET_DVR_SetConnectTime(2000, 1);
	NET_DVR_SetReconnect(10000, true);
}
//��ȡ���������豸ʱ��
void Htime() {
	bool iRet;
	DWORD dwReturnLen;
	NET_DVR_TIME struParams = { 0 };

	iRet = NET_DVR_GetDVRConfig(IUserID, NET_DVR_GET_TIMECFG, 1, \
		&struParams, sizeof(NET_DVR_TIME), &dwReturnLen);
	if (!iRet)
	{
		printf("NET_DVR_GetDVRConfig NET_DVR_GET_TIMECFG  error.\n");
		NET_DVR_Logout(IUserID);
		NET_DVR_Cleanup();
	}



	printf("%d��%d��%d��%d:%d:%d\n", struParams.dwYear, struParams.dwMonth, struParams.dwDay, struParams.dwHour, struParams.dwMinute, struParams.dwSecond);
}

//ע��������豸
bool Login(char *sDVRIP, short wDVRPort, char *sUserName, char *sPassword)
{
	IUserID = NET_DVR_Login_V30(sDVRIP, wDVRPort, sUserName, sPassword, &struDeviceInfo);

	if (IUserID < 0)
	{
		std::cout << "Login Failed! Error number��" << NET_DVR_GetLastError() << std::endl;
		NET_DVR_Cleanup();
		return false;
	}
	else
	{
		std::cout << "Login Successfully!" << std::endl;
		return true;
	}

}

//Show_SDK_Version()��������sdk�汾��ȡ����
void Show_SDK_Version()
{
	unsigned int uiVersion = NET_DVR_GetSDKBuildVersion();

	char strTemp[1024] = { 0 };
	sprintf_s(strTemp, "HCNetSDK V%d.%d.%d.%d\n", \
		(0xff000000 & uiVersion) >> 24, \
		(0x00ff0000 & uiVersion) >> 16, \
		(0x0000ff00 & uiVersion) >> 8, \
		(0x000000ff & uiVersion));
	printf(strTemp);
}


/*
//��ͨץ�Ľ��(�±�����Ϣ)
	case COMM_ITS_PLATE_RESULT: {
		NET_ITS_PLATE_RESULT struITSPlateResult = { 0 };
		memcpy(&struITSPlateResult, pAlarmInfo, sizeof(struITSPlateResult));
		for (i = 0; i<struITSPlateResult.dwPicNum; i++)
		{
			printf("���ƺ�: %s\n", struITSPlateResult.struPlateInfo.sLicense); //���ƺ�
			carNum = struITSPlateResult.struPlateInfo.sLicense;
			/**
			oFile << carNum << "," << sys.wYear << "-" << sys.wMonth << "-" << sys.wDay << " " << sys.wHour << ":" << sys.wMinute << ":" << sys.wSecond << endl; //���泵�ƺŵ�csv�ļ�	
			if ((struITSPlateResult.struPicInfo[i].dwDataLen != 0) && (struITSPlateResult.struPicInfo[i].byType == 1) || (struITSPlateResult.struPicInfo[i].byType == 2))
			{
				sprintf(filename, "./pic/%s_%d.jpg", struITSPlateResult.struPlateInfo.sLicense, i);
				fSnapPic = fopen(filename, "wb");
				fwrite(struITSPlateResult.struPicInfo[i].pBuffer, struITSPlateResult.struPicInfo[i].dwDataLen, 1, fSnapPic);
				iNum++;
				fclose(fSnapPic);
			}
			//����СͼƬ
			if ((struITSPlateResult.struPicInfo[i].dwDataLen != 0) && (struITSPlateResult.struPicInfo[i].byType == 0))
			{
				sprintf(filename, "./pic/1/%s_%d.jpg", struITSPlateResult.struPlateInfo.sLicense, i);
				fSnapPicPlate = fopen(filename, "wb");
				fwrite(struITSPlateResult.struPicInfo[i].pBuffer, struITSPlateResult.struPicInfo[i].dwDataLen, 1, \
					fSnapPicPlate);
				iNum++;
				fclose(fSnapPicPlate);
			}
			*/

// manual snap
BOOL manualSnap(BYTE vehicle_lane_number = 1, char outResult[255])
{
	NET_DVR_MANUALSNAP struManualSnap = { 0 };
	NET_DVR_PLATE_RESULT struResult = { 0 };
	
	struManualSnap.byLaneNo = min(6, max(vehicle_lane_number, 1));
	struManualSnap.byOSDEnable = 1;

	memset(&struManualSnap, 0, sizeof(struManualSnap));
	memset(&struResult, 0, sizeof(struResult));

	if (!NET_DVR_manualSnap(IUserID, &struManualSnap, &struResult))
	{
		return FALSE;
	}
	
	memset(outResult, 0, sizeof(outResult));
	sprintf(outResult, "{%d|%s}", struResult.struPlateInfo.byPlateType, struResult.struPlateInfo.sLicense);

	

	return TRUE;
}


//�˳�
void OnExit(void)
{
	std::cout << "Begin exit..." << std::endl;

	//�ͷ����
	NET_DVR_Logout(IUserID);//ע���û�
	NET_DVR_Cleanup();//�ͷ�SDK��Դ	
}


int main()
{
	Init();//��ʼ��sdk
	Connect();//���������¼�������ʱ��			  	
	Login(sDVRIP, wDVRPort, sUserName, sPassword);	//ע���豸
	Htime(); //��ȡ���������豸ʱ��

	

	for (;;) {


	}

	Sleep(-1);
	atexit(OnExit);//�˳�
	return 0;

}