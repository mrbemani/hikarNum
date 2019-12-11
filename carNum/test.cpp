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
LONG IUserID;	//摄像机设备
NET_DVR_DEVICEINFO_V30 struDeviceInfo;	//设备信息


char sDVRIP[20] = "192.168.62.64";	//抓拍摄像机设备IP地址
short wDVRPort = 8000;	//设备端口号
char sUserName[20] = "admin";	//登录的用户名
char sPassword[20] = "jtsjy123456";	//用户密码
string carNum;//车牌号							


				  //---------------------------------------------------------------------------------
				  //函数声明
void Init(void);//初始化
void Show_SDK_Version(); //获取sdk版本
void Connect();//设置连接事件与重连时间
void Htime();//获取海康威视设备时间
bool Login(char *sDVRIP, short wDVRPort, char *sUserName, char *sPassword);//注册摄像机设备
BOOL manualSnap(char outResult[255], BYTE vehicle_lane_number);
void OnExit(void);//退出
				  //---------------------------------------------------------------------------------------------------
				  //函数定义
				  //初始化
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


	//车牌颜色
	char strPlateColor[32] = { 0 };
	switch (struResult.struPlateInfo.byColor)
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
	switch (struResult.struPlateInfo.byPlateType)
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


//退出
void OnExit(void)
{
	std::cout << "Begin exit..." << std::endl;

	//释放相机
	NET_DVR_Logout(IUserID);//注销用户
	NET_DVR_Cleanup();//释放SDK资源	
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
	Init();//初始化sdk
	Connect();//设置连接事件与重连时间			  	
	Login(sDVRIP, wDVRPort, sUserName, sPassword);	//注册设备
	Htime(); //获取海康威视设备时间

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

	atexit(OnExit);//退出
	return 0;

}