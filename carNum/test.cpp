/************************************************************************
* Copyright(c) 2019
* Hikarnum
* Using Hikvision SDK to manual capture license plate.
************************************************************************/
//-------------------------------------------------------------------------
//ͷ�ļ�

#include <stdio.h>
#include <iostream>
#include <fstream> 
#include <string>
#include <streambuf>
#include "HCNetSDK.h"
#include <time.h>
#include "httpd.h"

#pragma comment(lib,"HCNetSDK.lib")
#pragma comment(lib,"PlayCtrl.lib")
#pragma comment(lib,"GdiPlus.lib")
#pragma comment(lib,"HCCore.lib")

using namespace std;


//��������
LONG IUserID;	//������豸
NET_DVR_DEVICEINFO_V30 struDeviceInfo;	//�豸��Ϣ


char sDVRIP[20] = "192.168.62.64";	//ץ��������豸IP��ַ
short wDVRPort = 8000;	//�豸�˿ں�
char sUserName[20] = "admin";	//��¼���û���
char sPassword[20] = "jtsjy123456";	//�û�����
string carNum;//���ƺ�							


				  //---------------------------------------------------------------------------------
				  //��������
void Init(void);//��ʼ��
void Show_SDK_Version(); //��ȡsdk�汾
void Connect();//���������¼�������ʱ��
void Htime();//��ȡ���������豸ʱ��
bool Login(char *sDVRIP, short wDVRPort, char *sUserName, char *sPassword);//ע��������豸
BOOL manualSnap(char outResult[255], BYTE vehicle_lane_number);
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




// manual snap
BOOL manualSnap(char outResult[255], BYTE vehicle_lane_number)
{
	NET_DVR_MANUALSNAP struManualSnap = { 0 };
	NET_DVR_PLATE_RESULT struResult = { 0 };
	
	struManualSnap.byLaneNo = min(6, max(vehicle_lane_number, 1));
	struManualSnap.byOSDEnable = 1;

	memset(outResult, 0, sizeof(outResult));

	memset(&struManualSnap, 0, sizeof(struManualSnap));
	memset(&struResult, 0, sizeof(struResult));

	if (!NET_DVR_ManualSnap(IUserID, &struManualSnap, &struResult))
	{
		DWORD err = NET_DVR_GetLastError();
		printf("err: %ld", err);
		return FALSE;
	}


	//������ɫ
	char strPlateColor[32] = { 0 };
	switch (struResult.struPlateInfo.byColor)
	{
	case VCA_BLUE_PLATE:
		sprintf(strPlateColor, "��ɫ");
		break;
	case VCA_YELLOW_PLATE:
		sprintf(strPlateColor, "��ɫ");
		break;
	case VCA_WHITE_PLATE:
		sprintf(strPlateColor, "��ɫ");
		break;
	case VCA_BLACK_PLATE:
		sprintf(strPlateColor, "��ɫ");
		break;
	default:
		sprintf(strPlateColor, "δ֪");
		break;
	}

	char strPlateType[32] = { 0 };
	switch (struResult.struPlateInfo.byPlateType)
	{
	case VCA_STANDARD92_PLATE:
		sprintf(strPlateType, "��׼���ó������");
		break;
	case VCA_STANDARD02_PLATE:
		sprintf(strPlateType, "02ʽ���ó���");
		break;
	case VCA_WJPOLICE_PLATE:
		sprintf(strPlateType, "�侯��");
		break;
	case VCA_JINGCHE_PLATE:
		sprintf(strPlateType, "����");
		break;
	case STANDARD92_BACK_PLATE:
		sprintf(strPlateType, "���ó�˫��β��");
		break;
	case VCA_SHIGUAN_PLATE:
		sprintf(strPlateType, "ʹ�ݳ���");
		break;
	case VCA_NONGYONG_PLATE:
		sprintf(strPlateType, "ũ�ó���");
		break;
	case VCA_MOTO_PLATE:
		sprintf(strPlateType, "Ħ�г�����");
		break;
	case NEW_ENERGY_PLATE:
		sprintf(strPlateType, "����Դ������");
		break;
	default:
		sprintf(strPlateType, "δ֪");
		break;
	}

	if (struResult.struPlateInfo.byLicenseLen < 5) 
	{
		return FALSE;
	}

	if (struResult.struPlateInfo.byBright < 5) 
	{
		printf("[FAILED] license brightness is: %d\n", struResult.struPlateInfo.byBright);
		return FALSE;
	}

	sprintf(outResult,
		"{\"plateConfidence\": %d,"
		" \"direction\": %d,"
		" \"plateType\": \"%s\","
		" \"plateColor\": \"%s\","
		" \"brightness\": %d,"
		" \"license\": \"%s\"}",
		struResult.struPlateInfo.byEntireBelieve,
		struResult.byCarDirectionType,
		strPlateType,
		strPlateColor,
		struResult.struPlateInfo.byBright,
		struResult.struPlateInfo.sLicense
	);

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

int req_carnum(BYTE iLane, char result[255])
{
	//sprintf(result, "Hello, %d", iLane);
	if (!manualSnap(result, iLane))
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

int _main()
{
	start_http_server(req_carnum);
	return 0;
}

int main()
{
	Init();//��ʼ��sdk
	Connect();//���������¼�������ʱ��			  	
	Login(sDVRIP, wDVRPort, sUserName, sPassword);	//ע���豸
	Htime(); //��ȡ���������豸ʱ��

	start_http_server(req_carnum);

	/*
	BYTE nLaneNumber = 1;
	char snapResultJSON[255] = { 0 };

	while (!manualSnap(snapResultJSON, nLaneNumber)) 
	{
		Sleep(500);
	}
	
	printf("%s\n", snapResultJSON);
	//*/

	atexit(OnExit);//�˳�
	return 0;

}