//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor2\src\QResArscParser.h
//	desc:		
//
//	created:	wangdaye 22:6:2025   11:48
//********************************************************************
#ifndef QResArscParser_h__
#define QResArscParser_h__

#include "GuidFactory.h"
#include "QPublicFinal.h"
#include "QStringPool.h"
#include "QTablePackage.h"
#include <QMap>
#include <QObject>

typedef	std::function< void(const QString&, const PTablePackage&) > TTRAVERSAL_ALL_TABLEPACKAGE;
struct zip_source;

class QResArscParser : public QObject
{
	Q_OBJECT

public:
	QResArscParser(QObject* _parent);
	QResArscParser(const QResArscParser&) = delete;
	QResArscParser& operator=(const QResArscParser&) = delete;
	~QResArscParser();
	void reset(void);
	bool readFile(const QString& _path);
	bool readBuff(const QByteArray& _buff);
	bool writeFile(const QString& _path);
	void writeBuff(QByteArray& _buff);
	void traversalAllTablePackage(TTRAVERSAL_ALL_TABLEPACKAGE _callBack);
private:
	bool readApkFile(const QString& _path);
	bool readApkFile(const QByteArray& _buff);
	bool readArscFile(const QString& _path);
	bool writeApkFile(const QString& _path);
	bool updateApkFileBuff(void);
	bool writeArscFile(const QString& _path);
private:
	QPublicFinal* m_publicFinal;
	QStringPool* m_stringPool;
	QMap<QString, PTablePackage> m_tablePackages;
	QByteArray m_fileBuff;
};

#endif // QResArscParser_h__
