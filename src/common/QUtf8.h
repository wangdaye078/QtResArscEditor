//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor\src\QUtf8.h
//	desc:		
//
//	created:	wangdaye 25:3:2025   8:56
//********************************************************************
#ifndef QUtf8_h__
#define QUtf8_h__
#include <QString>

extern QString utf8_to_QString(const char* _pBuff, quint32 _len);
extern QByteArray QString_to_utf8(const QString& _str);

#endif // QUtf8_h__