#pragma once
#include <string>
#include <cstring>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "const.h"
#undef ERROR
	//数据包



class Packet {
	//添加16位无符号数
	void addUint16(uint16 v) {
		v = htons(v);	//转换为网络字节序
		memcpy(buf + len, &v, 2);
		len += 2;
	}

	//添加字符串
	void addChars(const char* str) {
		strcpy(buf + len, str);
		len += strlen(str) + 1;		//长度要加上最后的'\0'
	}
	

public:
	char buf[BufSize];	//数据包缓冲区
	int len;	//数据包有效长度

	Packet(){
		len = 0;
		
	}
	//封装RRQ请求包
	static Packet RRQ(const char* filename, const char* mode);
	//封装RQ请求包
	static Packet WRQ(const char* filename, const char* mode);
	//封装DATA包
	static Packet DATA(uint16 BlockId, const char* data, int l);
	//封装ACK包
	static Packet ACK(uint16 BlockId);
	//封装ERROR包
	static Packet ERROR(uint16 errCode, const char* msg = nullptr);
	
	//获取DATA包中数据长度
	int getDataLen() const { return len - 4; }
	//获取数据包的操作码
	uint16 getop() const {
		uint16 v;
		memcpy(&v, buf + 0, 2);
		return ntohs(v);
	}
	//获取数据包的数据块编号
	uint16 getBlockId()const {
		uint16 v;
		memcpy(&v, buf + 2, 2);
		return ntohs(v); }
	//获取ERROR包的错误码
	uint16 getErrCode() const {
		uint16 v;
		memcpy(&v, buf + 2, 2);
		return ntohs(v);
	}
	//获取ERROR包的错误信息
	const char* getErrMsg() const { return buf + 4; }
	//获取DATA包的数据(基地址)
	const char* getData() const { return buf + 4; }
};


Packet Packet::RRQ(const char* filename, const char* mode) {
	auto pkt = Packet();
	pkt.addUint16(uint16(opRRQ));	//设置操作码
	pkt.addChars(filename);	//添加文件名
	pkt.addChars(mode);	//添加传输模式字符串
	return pkt;
}

Packet Packet::WRQ(const char* filename, const char* mode) {
	auto pkt = Packet();
	pkt.addUint16(uint16(opWRQ));	//设置操作码
	pkt.addChars(filename);	//添加文件名
	pkt.addChars(mode);	//添加传输模式字符串
	return pkt;
}


Packet Packet::DATA(uint16 BlockId, const char* data, int l) {
	auto pkt = Packet();
	if (l > DataMaxSize) {
		//Log
	}
	pkt.addUint16(uint16(opDATA));
	pkt.addUint16(BlockId);
	memcpy(pkt.buf + pkt.len, data, l);
	pkt.len += l;
	return pkt;
}

Packet Packet::ACK(uint16 BlockId) {
	auto pkt = Packet();
	pkt.addUint16(uint16(opACK));
	pkt.addUint16(BlockId);
	return pkt;
}

Packet Packet::ERROR(uint16 errCode, const char* msg) {
	auto pkt = Packet();
	pkt.addUint16(uint16(opERROR));
	pkt.addUint16(errCode);
	//默认错误信息
	if (errCode > 0 && errCode < 8) pkt.addChars(ErrMsg[errCode]);
	//自定义错误信息
	else if (msg) pkt.addChars(msg);
	else if (errCode == 0 && !msg) pkt.addChars(ErrMsg[0]);
	return pkt;
}


#include "pch.h"