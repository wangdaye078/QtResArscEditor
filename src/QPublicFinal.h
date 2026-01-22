//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor\src\QPublicFinal.h
//	desc:		
//
//	created:	wangdaye 19:3:2025   21:57
//********************************************************************
#ifndef QPublicFinal_h__
#define QPublicFinal_h__
#include "QStringPool.h"
#include <QMap>
#include <QObject>

inline bool operator<(const PArscRichString& _l, const PArscRichString& _r)
{
	return *_l < *_r;
}

class QPublicFinal : public QObject
{
	Q_OBJECT
public:
	QPublicFinal(QObject* parent);
	~QPublicFinal();
	PArscRichString getDataName(uint32_t _data);
	uint32_t getDataId(const QString& _dataName);
private:
	void initFinalData(void);
private:
	QMap<uint32_t, PArscRichString> m_finalIdMap;
	QMap<PArscRichString, uint32_t> m_finalStrMap;

};

extern QPublicFinal* g_publicFinal;

#endif // QPublicFinal_h__
