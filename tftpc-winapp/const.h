#pragma once
#pragma comment(lib, "ws2_32.lib")

using byte = unsigned char;
using uint16 = unsigned short;
using uint = unsigned;
using uint64 = unsigned long long;
constexpr uint16 opRRQ = 1;			//读请求操作码
constexpr uint16 opWRQ = 2;			//写请求操作码
constexpr uint16 opDATA = 3;		//数据操作码
constexpr uint16 opACK = 4;			//确认操作码
constexpr uint16 opERROR = 5;		//错误操作码
constexpr int BufSize = 1024;	//默认缓冲区大小
constexpr char ModeAscii[] = "netascii";
constexpr char Modeoctet[] = "octet";

constexpr int DataMaxSize = 512;	//最大数据大小
constexpr char ErrMsg[][34] = {
	"Undefined error code","File not found","Access violation",
	"Disk full or allocation exceeded","Illegal TFTP operation",
	"Unknown transfer ID","File already exists","No such user"
};//默认错误信息
int SktAddrLen = sizeof(sockaddr);