/************************************************************************
* Copyright(c) 2019
* Hikarnum
* Using Hikvision SDK to manual capture license plate.
************************************************************************/
//-------------------------------------------------------------------------
//头文件

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


//参数声明
LONG IUserID = NULL;	//摄像机设备
LONG lAlarmHandle = NULL;	//布防句柄
NET_DVR_DEVICEINFO_V30 struDeviceInfo;	//设备信息
char plateResultBuff[255] = { 0 };	// 缓存BUFF
BYTE laneNumber = 0; // 车道号
time_t plateSnapTime = 0; // 抓取时间

u_short HTTP_PORT = 17000;
char sDVRIP[20] = "192.168.1.65";	//抓拍摄像机设备IP地址
short wDVRPort = 8000;	//设备端口号
char sUserName[20] = "admin";	//登录的用户名
char sPassword[20] = "jtsjy123456";	//用户密码
string carNum;//车牌号							


//---------------------------------------------------------------------------------
//函数声明
DWORD getTimeStamp(void);
BOOL CALLBACK MSesGCallback(LONG lCommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void* pUser);
void error_to_str(DWORD err, char err_str[255]);
void Init(void);//初始化
void Show_SDK_Version(); //获取sdk版本
void Connect();//设置连接事件与重连时间
void Htime();//获取海康威视设备时间
bool Login(char *sDVRIP, short wDVRPort, char *sUserName, char *sPassword);//注册摄像机设备
void OnExit(void);//退出
				  //---------------------------------------------------------------------------------------------------
				  //函数定义
				  //初始化


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
	//获取系统时间
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	cout << sys.wYear << "-" << sys.wMonth << "-" << sys.wDay << " " << sys.wHour << ":" << sys.wMinute << ":" << sys.wSecond << endl;

		
	NET_DVR_Init();//初始化
	Show_SDK_Version();//获取 SDK  的版本号和 build  信息	
}

//设置连接事件与重连时间
void Connect()
{
	NET_DVR_SetConnectTime(2000, 1);
	NET_DVR_SetReconnect(10000, true);
}
//获取海康威视设备时间
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



	printf("%d年%d月%d日%d:%d:%d\n", struParams.dwYear, struParams.dwMonth, struParams.dwDay, struParams.dwHour, struParams.dwMinute, struParams.dwSecond);
}

//注册摄像机设备
bool Login(char *sDVRIP, short wDVRPort, char *sUserName, char *sPassword)
{
	IUserID = NET_DVR_Login_V30(sDVRIP, wDVRPort, sUserName, sPassword, &struDeviceInfo);

	printf("Login to: %s\n", sDVRIP);

	if (IUserID < 0)
	{
		std::cout << "Login Failed! Error number：" << NET_DVR_GetLastError() << std::endl;
		NET_DVR_Cleanup();
		return false;
	}
	else
	{
		std::cout << "Login Successfully!" << std::endl;
		return true;
	}

}

//Show_SDK_Version()海康威视sdk版本获取函数
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
		sprintf(err_str, "DVR操作失败");
		break;
	case NET_DVR_DVRNORESOURCE:
		sprintf(err_str, "DVR资源不足");
		break;
	case NET_DVR_BUSY:
		sprintf(err_str, "设备忙");
		break;
	case NET_DVR_COMMANDTIMEOUT:
		sprintf(err_str, "设备命令执行超时");
		break;
	case NET_DVR_NETWORK_RECV_TIMEOUT:
		sprintf(err_str, "从设备接收数据超时");
		break;
	case NET_DVR_NETWORK_RECV_ERROR:
		sprintf(err_str, "从设备接收数据失败");
		break;
	case NET_DVR_SOCKETCLOSE_ERROR:
		sprintf(err_str, "socket连接中断，此错误通常是由于连接中断或目的地不可达");
		break;
	default:
		sprintf(err_str, "未知错误[%ld]", err);
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

		//车牌颜色
		char strPlateColor[32] = { 0 };
		switch (struITSPlateResult.struPlateInfo.byColor)
		{
		case VCA_BLUE_PLATE:
			sprintf(strPlateColor, "蓝色");
			break;
		case VCA_YELLOW_PLATE:
			sprintf(strPlateColor, "黄色");
			break;
		case VCA_WHITE_PLATE:
			sprintf(strPlateColor, "白色");
			break;
		case VCA_BLACK_PLATE:
			sprintf(strPlateColor, "黑色");
			break;
		default:
			sprintf(strPlateColor, "未知");
			break;
		}

		char strPlateType[32] = { 0 };
		switch (struITSPlateResult.struPlateInfo.byPlateType)
		{
		case VCA_STANDARD92_PLATE:
			sprintf(strPlateType, "标准民用车与军车");
			break;
		case VCA_STANDARD02_PLATE:
			sprintf(strPlateType, "02式民用车牌");
			break;
		case VCA_WJPOLICE_PLATE:
			sprintf(strPlateType, "武警车");
			break;
		case VCA_JINGCHE_PLATE:
			sprintf(strPlateType, "警车");
			break;
		case STANDARD92_BACK_PLATE:
			sprintf(strPlateType, "民用车双行尾牌");
			break;
		case VCA_SHIGUAN_PLATE:
			sprintf(strPlateType, "使馆车牌");
			break;
		case VCA_NONGYONG_PLATE:
			sprintf(strPlateType, "农用车牌");
			break;
		case VCA_MOTO_PLATE:
			sprintf(strPlateType, "摩托车车牌");
			break;
		case NEW_ENERGY_PLATE:
			sprintf(strPlateType, "新能源车车牌");
			break;
		default:
			sprintf(strPlateType, "未知");
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


//退出
void OnExit(void)
{
	std::cout << "Begin exit..." << std::endl;

	//Disarming uploading channel
	if (lAlarmHandle != NULL && !NET_DVR_CloseAlarmChan_V30(lAlarmHandle))
	{
		printf("NET_DVR_CloseAlarmChan_V30 error, %d\n", NET_DVR_GetLastError());
	}

	//释放相机
	NET_DVR_Logout(IUserID);//注销用户
	NET_DVR_Cleanup();//释放SDK资源	
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

	Init();//初始化sdk
	Connect();//设置连接事件与重连时间			  	
	Login(sDVRIP, wDVRPort, sUserName, sPassword);	//注册设备
	Htime(); //获取海康威视设备时间

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

	atexit(OnExit);//退出
	return 0;

}