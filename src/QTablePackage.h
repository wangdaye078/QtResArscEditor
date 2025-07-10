//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor2\src\QTablePackage.h
//	desc:		
//
//	created:	wangdaye 22:6:2025   11:49
//********************************************************************
#ifndef QTablePackage_h__
#define QTablePackage_h__

#include <QObject>
#include <QSharedPointer>
#include "QStringPool.h"
#include "QTableType.h"
#include <QVariant>


class QTablePackage : public QObject
{
public:
	QTablePackage(QObject* _parent = NULL);
	QTablePackage(const QTablePackage&) = delete;
	QTablePackage& operator=(const QTablePackage&) = delete;
	~QTablePackage();
	void reset(void);
	void readBuff(const char* _buff);
	void writeBuff(QByteArray& _buff);
	//遍历所有的资源类及资源分组，但并不遍历具体的数据
	void traversalData(TRAVERSE_PACKAGE_DATA_CALLBACK _callBack) const;
	QString getKeyString(const QString& _prefix, bool _addtype, uint32_t _index) const;
	QString getTypeString(uint32_t _index) const;
	QString getReference(const ResTable_config& _config, qint32 _data) const;
	uint32_t keyGuidToIndex(uint32_t _guid) const;
	const ResTable_package& packageInfo(void) const;
private:
	ResTable_package m_packageInfo;
	QStringPool m_typeStringPool;		//anim,array,string这些类型字符串，key-1才是索引值
	QStringPool m_keyStringPool;		//每个值的名字，key就是索引
	QMap<uint32_t, PTableType> m_tableType;
};


#endif // QTablePackage_h__
