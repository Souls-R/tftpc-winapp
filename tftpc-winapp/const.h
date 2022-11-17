#pragma once
#pragma comment(lib, "ws2_32.lib")

using byte = unsigned char;
using uint16 = unsigned short;
using uint = unsigned;
using uint64 = unsigned long long;
constexpr uint16 opRRQ = 1;			//�����������
constexpr uint16 opWRQ = 2;			//д���������
constexpr uint16 opDATA = 3;		//���ݲ�����
constexpr uint16 opACK = 4;			//ȷ�ϲ�����
constexpr uint16 opERROR = 5;		//���������
constexpr int BufSize = 1024;	//Ĭ�ϻ�������С
constexpr char ModeAscii[] = "netascii";
constexpr char Modeoctet[] = "octet";

constexpr int DataMaxSize = 512;	//������ݴ�С
constexpr char ErrMsg[][34] = {
	"Undefined error code","File not found","Access violation",
	"Disk full or allocation exceeded","Illegal TFTP operation",
	"Unknown transfer ID","File already exists","No such user"
};//Ĭ�ϴ�����Ϣ
int SktAddrLen = sizeof(sockaddr);