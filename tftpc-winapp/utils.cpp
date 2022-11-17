#include <string>
#include <sstream>
#include <iomanip>

std::wstring TUtf8ToUnicode(const char* pszUtf8Str, unsigned len = -1)
{
	std::wstring ret;
	do
	{
		if (!pszUtf8Str) break;
		// get UTF8 string length
		if (-1 == len)
		{
			len = strlen(pszUtf8Str);
		}
		if (len <= 0) break;

		// get UTF16 string length
		int wLen = MultiByteToWideChar(CP_UTF8, 0, pszUtf8Str, len, 0, 0);
		if (0 == wLen || 0xFFFD == wLen) break;

		// convert string  
		wchar_t* pwszStr = new(std::nothrow) wchar_t[wLen + 1];
		if (!pwszStr) break;
		pwszStr[wLen] = 0;
		MultiByteToWideChar(CP_UTF8, 0, pszUtf8Str, len, pwszStr, wLen + 1);
		ret = pwszStr;
		delete[] pwszStr;
	} while (0);
	return ret;
}

// std::string => Platform::String
Platform::String^ Ts2ps(std::string str)
{
	return ref new Platform::String(TUtf8ToUnicode(str.c_str()).c_str());
}

// 宽字符转为UTF8编码的多字节
std::string TUnicodeToUtf8(const wchar_t* pwszStr)
{
	std::string ret;
	do
	{
		if (!pwszStr) break;
		size_t len = wcslen(pwszStr);
		if (len <= 0) break;

		size_t convertedChars = 0;
		char* pszUtf8Str = new(std::nothrow) char[len * 3 + 1];
		if (!pszUtf8Str) break;
		WideCharToMultiByte(CP_UTF8, 0, pwszStr, len + 1, pszUtf8Str, len * 3 + 1, 0, 0);
		ret = pszUtf8Str;
		delete[] pszUtf8Str;
	} while (0);

	return ret;
}

// Platform::String => std::string
std::string Tps2s(Platform::String^ pstr)
{
	if (pstr == nullptr)
		return "";
	return TUnicodeToUtf8(pstr->Data());
}





static __int64 GetUnixTime()
{
	std::string nowTimeUnix;
	std::string cs_uninxtime;
	std::string cs_milliseconds;
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	time_t unixTime;
	time(&unixTime);
	char buf[30], buf1[30];
	sprintf_s(buf, sizeof(buf), "%I64d", (INT64)unixTime);
	sprintf_s(buf1, sizeof(buf1), "%03I64d", (INT64)sysTime.wMilliseconds);
	nowTimeUnix = std::string(buf) + std::string(buf1);
	return _atoi64(nowTimeUnix.c_str());
}

//小数点后两位精度的文本
auto itos_in2(double d) {
	std::stringstream stream;
	stream << std::fixed << std::setprecision(2) << d;
	std::string s = stream.str();
	return Ts2ps(s);
};
#include "pch.h"