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
#include "ResArscStruct.h"

class QPublicFinal : public QObject
{
	Q_OBJECT
public:
	QPublicFinal(QObject* parent);
	~QPublicFinal();
	QString getDataName(uint32_t _data);
private:
	void initFinalData(void);
private:
	QMap<uint32_t, QString> m_finalMap;
};
#endif // QPublicFinal_h__
