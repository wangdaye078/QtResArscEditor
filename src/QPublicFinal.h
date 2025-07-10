//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor\src\QPublicFinal.h
//	desc:		
//
//	created:	wangdaye 19:3:2025   21:57
//********************************************************************
#ifndef QPublicFinal_h__
#define QPublicFinal_h__
#include <QObject>
#include <QMap>
#include "QStringPool.h"

class QPublicFinal : public QObject
{
	Q_OBJECT
public:
	QPublicFinal(QObject* parent);
	~QPublicFinal();
	PArscRichString getDataName(uint32_t _data);
private:
	void initFinalData(void);
private:
	QMap<uint32_t, PArscRichString> m_finalMap;
};

extern QPublicFinal* g_publicFinal;

#endif // QPublicFinal_h__
