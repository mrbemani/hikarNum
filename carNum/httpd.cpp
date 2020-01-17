// very simple httpd

#include <Winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "httpd.h"
 
#pragma comment (lib,"ws2_32")
 
typedef struct _NODE_
{
 SOCKET s;
 sockaddr_in Addr;
 
}Node,*pNode;
 
 
//多线程处理多个客户端的连接
typedef struct _THREAD_
{
	DWORD ThreadID;
	HANDLE hThread;
}Thread,*pThread;
 
bool InitSocket();//线程函数
DWORD WINAPI AcceptThread(LPVOID lpParam);
DWORD WINAPI ClientThread(LPVOID lpParam);
bool IoComplete(char* szRequest);     //数据包的校验函数
bool ParseRequest(char* szRequest, char* szResponse, BOOL &bKeepAlive);

p_ts_http_callback_func _callback_func = NULL;
BYTE capture_lane_number = 1;
unsigned short uPort = 17000;

int start_http_server(unsigned short port, p_ts_http_callback_func http_req_callback_func)
{
	if (!InitSocket())
	{
		printf("InitSocket Error\n");
		return 1;
	}
 
	if (!http_req_callback_func)
	{
		printf("http callback cannot be null!");
		return 2;
	}

	uPort = port;
	_callback_func = http_req_callback_func;

	// start a thread
	//HANDLE hAcceptThread = CreateThread(NULL,0,AcceptThread,NULL,0,NULL);
 
	// use an event model to initiate server
	// create an event object
	//WaitForSingleObject(hAcceptThread,INFINITE);

	// sq update: call directly
	AcceptThread(NULL);

	return 0;
}
 
DWORD WINAPI AcceptThread(LPVOID lpParam)   // receive thread
{
	// create a listening socket
	SOCKET sListen = WSASocket(AF_INET,SOCK_STREAM,0,NULL,0,WSA_FLAG_OVERLAPPED); // 使用事件重叠的套接字
	if (sListen==INVALID_SOCKET)
	{
		printf("Create Listen Error\n");
		return -1; 
	}
	// initialize server address
	sockaddr_in LocalAddr;
	LocalAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	LocalAddr.sin_family = AF_INET;
	LocalAddr.sin_port = htons(uPort);
	int Ret = bind(sListen,(sockaddr*)&LocalAddr,sizeof(LocalAddr));
	if (Ret==SOCKET_ERROR)
	{
		printf("Bind Error\n");
		return -1;
	}
	printf("Start listening on: %s %d\n", INADDR_ANY, uPort);
	// start listening
	listen(sListen,5);
	// create an event
	WSAEVENT Event = WSACreateEvent();
	if (Event==WSA_INVALID_EVENT)
	{
		printf("Create WSAEVENT Error\n");
		closesocket(sListen);
		CloseHandle(Event);     // creating event failed, closing socket, closing event.
		return -1;
	}
	// 将监听套接字与事件进行关联属性为Accept
	WSAEventSelect(sListen,Event,FD_ACCEPT);
	WSANETWORKEVENTS NetWorkEvent;
	sockaddr_in ClientAddr;
	int nLen = sizeof(ClientAddr);
	DWORD dwIndex = 0;
	
	while (1)
	{
		dwIndex = WSAWaitForMultipleEvents(1,&Event,FALSE,WSA_INFINITE,FALSE);
		dwIndex = dwIndex - WAIT_OBJECT_0;
		if (dwIndex==WSA_WAIT_TIMEOUT||dwIndex==WSA_WAIT_FAILED)
		{
			continue;
		}
		//如果有真正的事件我们就进行判断
		WSAEnumNetworkEvents(sListen,Event,&NetWorkEvent);
		ResetEvent(&Event);   //
		if (NetWorkEvent.lNetworkEvents == FD_ACCEPT)
		{
			if (NetWorkEvent.iErrorCode[FD_ACCEPT_BIT]==0)
			{
				//我们要为新的连接进行接受并申请内存存入链表中
				SOCKET sClient = WSAAccept(sListen, (sockaddr*)&ClientAddr, &nLen, NULL, NULL);
				if (sClient==INVALID_SOCKET)
				{
					continue;
				}
				else
				{
					//如果接收成功我们要把用户的所有信息存放到链表中
					Node nTemp = { 0 };
					nTemp.Addr = ClientAddr;
					nTemp.s = sClient;
					ClientThread(&nTemp);
				}
			}
		}
	}
	return 0;
}
 
DWORD WINAPI ClientThread(LPVOID lpParam)
{
	//我们将每个用户的信息以参数的形式传入到该线程
	pNode pTemp = (pNode)lpParam;
	SOCKET sClient = pTemp->s; //这是通信套接字
	WSAEVENT Event = WSACreateEvent(); //该事件是与通信套接字关联以判断事件的种类
	WSANETWORKEVENTS NetWorkEvent;
	char szRequest[1024]={0}; //请求报文
	char szResponse[1024]={0}; //响应报文
	BOOL bKeepAlive = FALSE; //是否持续连接
	if ( Event == WSA_INVALID_EVENT )
	{
		return -1;
	}
	int Ret = WSAEventSelect(sClient, Event, FD_READ | FD_WRITE | FD_CLOSE); //关联事件和套接字
	DWORD dwIndex = 0;
	while (1)
	{
		dwIndex = WSAWaitForMultipleEvents(1,&Event,FALSE,WSA_INFINITE,FALSE);
		dwIndex = dwIndex - WAIT_OBJECT_0;
		if (dwIndex==WSA_WAIT_TIMEOUT||dwIndex==WSA_WAIT_FAILED)
		{
			continue;
		}
		// 分析什么网络事件产生
		Ret = WSAEnumNetworkEvents(sClient,Event,&NetWorkEvent);
		//其他情况
		if ( !NetWorkEvent.lNetworkEvents )
		{
			continue;
		}
		if ( NetWorkEvent.lNetworkEvents & FD_READ ) //这里很有意思的
		{
			DWORD NumberOfBytesRecvd;
			WSABUF Buffers;
			DWORD dwBufferCount = 1;
			char szBuffer[MAX_BUFFER];
			DWORD Flags = 0;
			Buffers.buf = szBuffer;
			Buffers.len = MAX_BUFFER;
			Ret = WSARecv(sClient,&Buffers,dwBufferCount,&NumberOfBytesRecvd,&Flags,NULL,NULL);
			//我们在这里要检测是否得到的完整请求
			memcpy(szRequest,szBuffer,NumberOfBytesRecvd);
			if (!IoComplete(szRequest)) //校验数据包
			{
				continue;
			}
			if (!ParseRequest(szRequest, szResponse, bKeepAlive)) //分析数据包
			{
				//我在这里就进行了简单的处理
				continue;
			}
			DWORD NumberOfBytesSent = 0;
			DWORD dwBytesSent = 0;
			//发送响应到客户端
			do
			{
				Buffers.len = (strlen(szResponse) - dwBytesSent) >= SENDBLOCK ? SENDBLOCK : strlen(szResponse) - dwBytesSent; 
				Buffers.buf = (char*)((DWORD)szResponse + dwBytesSent);  
				Ret = WSASend(sClient, &Buffers, 1, &NumberOfBytesSent, 0, 0, NULL);
				if (SOCKET_ERROR != Ret)
					dwBytesSent += NumberOfBytesSent;
			}
			while((dwBytesSent < strlen(szResponse)) && SOCKET_ERROR != Ret); 
		}
		if(NetWorkEvent.lNetworkEvents & FD_CLOSE)
		{
			//在这里我没有处理，我们要将内存进行释放否则内存泄露
		}
	}
	return 0;
}
 
bool InitSocket()
{
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2,2),&wsadata)==0)    //使用Socket前必须调用 参数 作用 返回值
	{
		return true;
	}
	return false;
}
 
//校验数据包
bool IoComplete(char* szRequest)
{
	char* pTemp = NULL;   //定义临时空指针
	int nLen = strlen(szRequest); //请求数据包长度
	pTemp = szRequest;   
	pTemp = pTemp+nLen-4; //定位指针
	if (strcmp(pTemp,"\r\n\r\n")==0)   //校验请求头部行末尾的回车控制符和换行符以及空行
	{
		return true;
	}
	return false;
}
 
//分析数据包
bool ParseRequest(char* szRequest, char* szResponse, BOOL &bKeepAlive)
{
	char* p = NULL;
	p = szRequest;
	int n = 0;
	char* pTemp = strstr(p, " "); //判断字符串str2是否是str1的子串。如果是，则该函数返回str2在str1中首次出现的地址；否则，返回NULL。
	n = pTemp - p;    //指针长度
	// pTemp = pTemp + n - 1; //将我们的指针下移
	//定义一个临时的缓冲区来存放我们
	char szMode[10] = { 0 };
	char szFileName[128] = { 0 };
	int pos_http_1_x = 0;
	memcpy(szMode, p, n);   //将请求方法拷贝到szMode数组中
	if (strcmp(szMode, "GET") == 0)  //一定要将Get写成大写
	{
		//获取文件名
		pTemp = strstr(pTemp, " ");
		pTemp = pTemp + 1;   //只有调试的时候才能发现这里的秘密
		char* s_http_1_x = strstr(pTemp, " HTTP/1.");
		pos_http_1_x = s_http_1_x - pTemp;
		memcpy(szFileName, pTemp, pos_http_1_x);
		printf("[%s] %s\n", szMode, szFileName);
		if (strstr(szFileName, "/capture") == szFileName)
		{
			capture_lane_number = 1;
			if (strcmp(szFileName, "/capture/2") == 0)
			{
				capture_lane_number = 2;
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
	// 分析链接类型
	pTemp = strstr(szRequest, "\nConnection: Keep-Alive");  //协议版本
	n = pTemp - p;
	if (p > 0)
	{
		bKeepAlive = TRUE;
	}
	else  //这里的设置是为了Proxy程序的运行
	{
		bKeepAlive = TRUE;
	}
	//定义一个回显头
	char pResponseHeader[512] = { 0 };
	char szStatusCode[20] = { 0 };
	char szContentType[20] = { 0 };
	strcpy(szStatusCode, "200 OK");
	strcpy(szContentType, "application/json"); // response mime type is JSON
	char szDT[128];
	struct tm *newtime;
	time_t ltime;
	time(&ltime);
	newtime = gmtime(&ltime);
	strftime(szDT, 128, "%a, %d %b %Y %H:%M:%S GMT", newtime);
	// response data
	int length = 0;
	char BufferTemp[255] = { 0 };
	if (!_callback_func(capture_lane_number, BufferTemp))
	{
		//sprintf(BufferTemp, "Hello, world!");
		memset(BufferTemp, 0, 255);	
		sprintf(BufferTemp, "null");
	}
	length = strlen(BufferTemp);
	// 返回响应
	sprintf(pResponseHeader, "HTTP/1.0 %s\r\nDate: %s\r\nServer: %s\r\nAccept-Ranges: bytes\r\nContent-Length: %d\r\nConnection: %s\r\nContent-Type: %s\r\n\r\n",
		szStatusCode, szDT, SERVERNAME, length, bKeepAlive ? "Keep-Alive" : "close", szContentType);   //响应报文
	strcpy(szResponse,pResponseHeader);
	strcat(szResponse,BufferTemp);
	return true;
}