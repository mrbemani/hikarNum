#pragma once

// server constants
#define MAX_BUFFER   100000
#define SENDBLOCK   200000
#define SERVERNAME   "ButterflyIllusion_HikarNum/0.2a"


// callback function
typedef int(*p_ts_http_callback_func)(BYTE, char[255]);

// start httpd server
int start_http_server(unsigned short, p_ts_http_callback_func, unsigned short);
