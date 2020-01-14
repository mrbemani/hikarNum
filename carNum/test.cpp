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
LONG IUserID = NULL;	//������豸
LONG lAlarmHandle = NULL;	//�������
NET_DVR_DEVICEINFO_V30 struDeviceInfo;	//�豸��Ϣ
char plateResultBuff[255] = { 0 };	// ����BUFF
BYTE laneNumber = 0; // ������
time_t plateSnapTime = 0; // ץȡʱ��

u_short HTTP_PORT = 17000;
char sDVRIP[20] = "192.168.1.65";	//ץ��������豸IP��ַ
short wDVRPort = 8000;	//�豸�˿ں�
char sUserName[20] = "admin";	//��¼���û���
char sPassword[20] = "jtsjy123456";	//�û�����
string carNum;//���ƺ�							


//---------------------------------------------------------------------------------
//��������
DWORD getTimeStamp(void);
BOOL CALLBACK MSesGCallback(LONG lCommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void* pUser);
void error_to_str(DWORD err, char err_str[255]);
void Init(void);//��ʼ��
void Show_SDK_Version(); //��ȡsdk�汾
void Connect();//���������¼�������ʱ��
void Htime();//��ȡ���������豸ʱ��
bool Login(char *sDVRIP, short wDVRPort, char *sUserName, char *sPassword);//ע��������豸
void OnExit(void);//�˳�
				  //---------------------------------------------------------------------------------------------------
				  //��������
				  //��ʼ��


time_t getLocalTimeStamp()
{
	time_t ltime;
	time(&ltime);
	return ltime;
}

time_t getUTCTimeStamp()
{
	time_t ltime = getLocalTimeStamp();
	struct tm* timeinfo = gmtime(&ltime); /* Convert to UTC */
	ltime = mktime(timeinfo); /* Store as unix timestamp */
}


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

	printf("Login to: %s\n", sDVRIP);

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


// error number to text string
void error_to_str(DWORD err, char err_str[255])
{
	switch (err) 
	{
	case NET_DVR_DVROPRATEFAILED:
		sprintf(err_str, "DVR����ʧ��");
		break;
	case NET_DVR_DVRNORESOURCE:
		sprintf(err_str, "DVR��Դ����");
		break;
	case NET_DVR_BUSY:
		sprintf(err_str, "�豸æ");
		break;
	case NET_DVR_COMMANDTIMEOUT:
		sprintf(err_str, "�豸����ִ�г�ʱ");
		break;
	case NET_DVR_NETWORK_RECV_TIMEOUT:
		sprintf(err_str, "���豸�������ݳ�ʱ");
		break;
	case NET_DVR_NETWORK_RECV_ERROR:
		sprintf(err_str, "���豸��������ʧ��");
		break;
	case NET_DVR_SOCKETCLOSE_ERROR:
		sprintf(err_str, "socket�����жϣ��˴���ͨ�������������жϻ�Ŀ�ĵز��ɴ�");
		break;
	default:
		sprintf(err_str, "δ֪����[%ld]", err);
	}
}


// Alarm Callback function
BOOL CALLBACK MSesGCallback(LONG lCommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void* pUser)
{
	int i = 0;
	char filename[100];
	FILE *fSnapPic = NULL;
	FILE *fSnapPicPlate = NULL;

	//The following codes are for reference only. In the actual application, handle data and save file in the callback function is not suggested.
	//For example, you can handle data in the message response function by using message format (PostMessage).

	switch (lCommand)
	{
	case COMM_ITS_PLATE_RESULT:
	{
		NET_ITS_PLATE_RESULT struITSPlateResult = { 0 };
		memcpy(&struITSPlateResult, pAlarmInfo, sizeof(struITSPlateResult));

		//������ɫ
		char strPlateColor[32] = { 0 };
		switch (struITSPlateResult.struPlateInfo.byColor)
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
		switch (struITSPlateResult.struPlateInfo.byPlateType)
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

		laneNumber = 0;
		plateSnapTime = 0;
		memset(plateResultBuff, 0, sizeof(plateResultBuff));

		if (struITSPlateResult.struPlateInfo.byLicenseLen < 5)
		{
			plateSnapTime = 0;
			return TRUE;
		}

		if (struITSPlateResult.struPlateInfo.byBright < 5)
		{
			plateSnapTime = 0;
			return TRUE;
		}

		laneNumber = 3 - struITSPlateResult.byDriveChan;

		plateSnapTime = getLocalTimeStamp();
		sprintf(plateResultBuff,
			"{\"plateConfidence\": %d,"
			" \"laneNumber\": %d,"
			" \"plateType\": \"%s\","
			" \"plateColor\": \"%s\","
			" \"brightness\": %d,"
			" \"license\": \"%s\"}",
			struITSPlateResult.struPlateInfo.byEntireBelieve,
			laneNumber,
			strPlateType,
			strPlateColor,
			struITSPlateResult.struPlateInfo.byBright,
			struITSPlateResult.struPlateInfo.sLicense
		);
		printf("plate captured: %s", plateResultBuff);
		break;
	}
	default:
		break;
	}

	return TRUE;
}


//�˳�
void OnExit(void)
{
	std::cout << "Begin exit..." << std::endl;

	//Disarming uploading channel
	if (lAlarmHandle != NULL && !NET_DVR_CloseAlarmChan_V30(lAlarmHandle))
	{
		printf("NET_DVR_CloseAlarmChan_V30 error, %d\n", NET_DVR_GetLastError());
	}

	//�ͷ����
	NET_DVR_Logout(IUserID);//ע���û�
	NET_DVR_Cleanup();//�ͷ�SDK��Դ	
}

int req_carnum(BYTE iLane, char result[255])
{
	//sprintf(result, "Hello, %d", iLane);
	if (iLane != laneNumber) return FALSE;
	time_t now_t = getLocalTimeStamp();
	if (now_t - plateSnapTime < 2)
	{
		memcpy(result, plateResultBuff, sizeof(plateResultBuff));
		return TRUE;
	}
	else
	{
		memset(result, 0, sizeof(result));
		return FALSE;
	}
}

int main(int argc, char *argv[])
{

	if (argc > 1) 
	{
		sprintf(sDVRIP, "%s", argv[1]);
	}



	if (argc > 2)
	{
		HTTP_PORT = atoi(argv[2]);
	}

	Init();//��ʼ��sdk
	Connect();//���������¼�������ʱ��			  	
	Login(sDVRIP, wDVRPort, sUserName, sPassword);	//ע���豸
	Htime(); //��ȡ���������豸ʱ��

	//Set alarm callback function
	NET_DVR_SetDVRMessageCallBack_V31(MSesGCallback, NULL);
	
	//Enable arm
	NET_DVR_SETUPALARM_PARAM struSetupParam = { 0 };
	struSetupParam.dwSize = sizeof(NET_DVR_SETUPALARM_PARAM);
	struSetupParam.byLevel = 1; //Arming level: 0- level 1 (high), 1- level 2 (medium)
	struSetupParam.byAlarmInfoType = 1; //Uploaded alarm types: 0- History alarm (NET_DVR_PLATE_RESULT), 1- Real-time alarm (NET_ITS_PLATE_RESULT)

	lAlarmHandle = NET_DVR_SetupAlarmChan_V41(IUserID, &struSetupParam);
	if (lAlarmHandle < 0)
	{
		printf("NET_DVR_SetupAlarmChan_V41 failed, error code: %d\n", NET_DVR_GetLastError());
		OnExit();
		return 1;
	}
	printf("Armed.\n");

	// start http server
	start_http_server(HTTP_PORT, req_carnum);

	atexit(OnExit);//�˳�
	return 0;

}