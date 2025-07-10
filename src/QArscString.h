//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor2\src\QArscString.h
//	desc:		
//
//	created:	wangdaye 22:6:2025   19:08
//********************************************************************
#ifndef QArscString_h__
#define QArscString_h__
#include <QString>
#include <QByteArray>

template<typename T>
T readValue(const char*& _buff)
{
	T t = *reinterpret_cast<const T*>(_buff);
	_buff += sizeof(T);
	return t;
}

uint32_t writeStringLen_16(char* _pBuff, const QString& _str);
uint32_t writeStringLen_8(char* _pBuff, const QString& _str, QByteArray& _utf8Array);
void writeString(QByteArray& _buff, const QString& _string, bool _isUTF8);
uint32_t readStringLen_16(const char*& _pBuff);
uint readStringLen_8(const char*& _pBuff);
QString readString(const char* _pBuff, bool _isUTF8);

#endif // QArscString_h__