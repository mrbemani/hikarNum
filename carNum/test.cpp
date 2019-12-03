/************************************************************************
* Copyright(c) 2018
*
* File:		报警布防_0.3.c
* Brief:	调用海康威视sdk获取车牌号
* Version: 	0.3
* Author: 	一念无明
* Email:	zhunmeng@live.com
* Date:    	2018.02.26
* History:
2018.2.26 	调用海康威视sdk获取车牌号
2018.3.7 	获取设备系统时间
2018.3.12   保存车牌号到csv文件
2018.3.14	对识别到的车牌号进行白名单比对

************************************************************************/
//-------------------------------------------------------------------------
//头文件

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


//参数声明
LONG IUserID;	//摄像机设备
NET_DVR_DEVICEINFO_V30 struDeviceInfo;	//设备信息


char sDVRIP[20];	//抓拍摄像机设备IP地址
short wDVRPort = 8000;	//设备端口号
char sUserName[20] = { 0 };	//登录的用户名
char sPassword[20] = { 0 };	//用户密码
string carNum;//车牌号							


				  //---------------------------------------------------------------------------------
				  //函数声明
void Init(void);//初始化
void Show_SDK_Version(); //获取sdk版本
void Connect();//设置连接事件与重连时间
void Htime();//获取海康威视设备时间
bool Login(char *sDVRIP, short wDVRPort, char *sUserName, char *sPassword);//注册摄像机设备
void manualSnap();
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


/*
//交通抓拍结果(新报警消息)
	case COMM_ITS_PLATE_RESULT: {
		NET_ITS_PLATE_RESULT struITSPlateResult = { 0 };
		memcpy(&struITSPlateResult, pAlarmInfo, sizeof(struITSPlateResult));
		for (i = 0; i<struITSPlateResult.dwPicNum; i++)
		{
			printf("车牌号: %s\n", struITSPlateResult.struPlateInfo.sLicense); //车牌号
			carNum = struITSPlateResult.struPlateInfo.sLicense;
			/**
			oFile << carNum << "," << sys.wYear << "-" << sys.wMonth << "-" << sys.wDay << " " << sys.wHour << ":" << sys.wMinute << ":" << sys.wSecond << endl; //保存车牌号到csv文件	
			if ((struITSPlateResult.struPicInfo[i].dwDataLen != 0) && (struITSPlateResult.struPicInfo[i].byType == 1) || (struITSPlateResult.struPicInfo[i].byType == 2))
			{
				sprintf(filename, "./pic/%s_%d.jpg", struITSPlateResult.struPlateInfo.sLicense, i);
				fSnapPic = fopen(filename, "wb");
				fwrite(struITSPlateResult.struPicInfo[i].pBuffer, struITSPlateResult.struPicInfo[i].dwDataLen, 1, fSnapPic);
				iNum++;
				fclose(fSnapPic);
			}
			//车牌小图片
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


//退出
void OnExit(void)
{
	std::cout << "Begin exit..." << std::endl;

	//释放相机
	NET_DVR_Logout(IUserID);//注销用户
	NET_DVR_Cleanup();//释放SDK资源	
}


int main()
{
	Init();//初始化sdk
	Connect();//设置连接事件与重连时间			  	
	Login(sDVRIP, wDVRPort, sUserName, sPassword);	//注册设备
	Htime(); //获取海康威视设备时间

	

	for (;;) {


	}

	Sleep(-1);
	atexit(OnExit);//退出
	return 0;

}