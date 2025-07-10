//********************************************************************
//	filename: 	F:\mygit\QTrayToolbar\src\basicDefine.h
//	desc:		
//
//	created:	wangdaye 20:2:2025   12:54
//********************************************************************
#ifndef basicDefine_h__
#define basicDefine_h__

#ifdef _UNICODE
#define QStringToTCHAR(x)     (wchar_t*)((x).utf16())
#define PQStringToTCHAR(x)    (wchar_t*)((x)->utf16())
#define TCHARToQString(x)     QString::fromUtf16((x))
#define TCHARToQStringN(x,y)  QString::fromUtf16((x),(y))
#else
#define QStringToTCHAR(x)     x.toLocal8Bit().constData()
#define PQStringToTCHAR(x)    x->toLocal8Bit().constData()
#define TCHARToQString(x)     QString::fromLocal8Bit((x))
#define TCHARToQStringN(x,y)  QString::fromLocal8Bit((x),(y))
#endif

#define QStringToWCHAR(x)     (wchar_t*)(x).utf16()
#define QStringToCHAR(x)     (x).toLocal8Bit().constData()
#define QStringToUTF8(x)     (x).toUtf8().constData()
#define WCHARToQString(x)    QString::fromUtf16((ushort*)(x))
#define WCHARToQStringN(x,y)    QString::fromUtf16((ushort*)(x),(y))
#define CHARToQString(x)     QString::fromLocal8Bit((x))
#define CHARToQStringN(x,y)     QString::fromLocal8Bit((x),(y))
#define UTF8ToQString(x)     QString::fromUtf8((x))
#define UTF8ToQStringN(x,y)     QString::fromUtf8((x),(y))


#endif // basicDefine_h__
