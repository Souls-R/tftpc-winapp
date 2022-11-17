#pragma once
#include <string>
#include <cstring>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "const.h"
#undef ERROR
	//���ݰ�



class Packet {
	//���16λ�޷�����
	void addUint16(uint16 v) {
		v = htons(v);	//ת��Ϊ�����ֽ���
		memcpy(buf + len, &v, 2);
		len += 2;
	}

	//����ַ���
	void addChars(const char* str) {
		strcpy(buf + len, str);
		len += strlen(str) + 1;		//����Ҫ��������'\0'
	}
	

public:
	char buf[BufSize];	//���ݰ�������
	int len;	//���ݰ���Ч����

	Packet(){
		len = 0;
		
	}
	//��װRRQ�����
	static Packet RRQ(const char* filename, const char* mode);
	//��װRQ�����
	static Packet WRQ(const char* filename, const char* mode);
	//��װDATA��
	static Packet DATA(uint16 BlockId, const char* data, int l);
	//��װACK��
	static Packet ACK(uint16 BlockId);
	//��װERROR��
	static Packet ERROR(uint16 errCode, const char* msg = nullptr);
	
	//��ȡDATA�������ݳ���
	int getDataLen() const { return len - 4; }
	//��ȡ���ݰ��Ĳ�����
	uint16 getop() const {
		uint16 v;
		memcpy(&v, buf + 0, 2);
		return ntohs(v);
	}
	//��ȡ���ݰ������ݿ���
	uint16 getBlockId()const {
		uint16 v;
		memcpy(&v, buf + 2, 2);
		return ntohs(v); }
	//��ȡERROR���Ĵ�����
	uint16 getErrCode() const {
		uint16 v;
		memcpy(&v, buf + 2, 2);
		return ntohs(v);
	}
	//��ȡERROR���Ĵ�����Ϣ
	const char* getErrMsg() const { return buf + 4; }
	//��ȡDATA��������(����ַ)
	const char* getData() const { return buf + 4; }
};


Packet Packet::RRQ(const char* filename, const char* mode) {
	auto pkt = Packet();
	pkt.addUint16(uint16(opRRQ));	//���ò�����
	pkt.addChars(filename);	//����ļ���
	pkt.addChars(mode);	//��Ӵ���ģʽ�ַ���
	return pkt;
}

Packet Packet::WRQ(const char* filename, const char* mode) {
	auto pkt = Packet();
	pkt.addUint16(uint16(opWRQ));	//���ò�����
	pkt.addChars(filename);	//����ļ���
	pkt.addChars(mode);	//��Ӵ���ģʽ�ַ���
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
	//Ĭ�ϴ�����Ϣ
	if (errCode > 0 && errCode < 8) pkt.addChars(ErrMsg[errCode]);
	//�Զ��������Ϣ
	else if (msg) pkt.addChars(msg);
	else if (errCode == 0 && !msg) pkt.addChars(ErrMsg[0]);
	return pkt;
}


#include "pch.h"