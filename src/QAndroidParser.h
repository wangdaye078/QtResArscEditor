//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor2\src\QAndroidParser.h
//	desc:		
//
//	created:	wangdaye 6:1:2026   19:24
//********************************************************************
#ifndef QAndroidParser_h__
#define QAndroidParser_h__
#include "QStringPool.h"
#include "ResArscStruct.h"
#include <QObject>

class QAndroidParser : public QObject
{
public:
	QAndroidParser(QObject* _parent);
	virtual ~QAndroidParser();
	virtual RES_TYPE getParserType(void) const = 0;
	virtual const QString& getBinFileSuffix(void) const = 0;
	virtual const char* getBinFileName(void) const = 0;
	virtual void traversalSubItems(void) = 0;
	QStringPool* getStringPool(void) const;
	bool readFile(const QString& _path);
	bool writeFile(const QString& _path);
protected:
	bool readApkFile(const QString& _path, const char* _fileName);
	bool readApkFile(const QByteArray& _buff, const char* _fileName);
	bool readBinFile(const QString& _path);
	bool updateApkFileBuff(const QByteArray& _buff, const char* _fileName);
	bool writeApkFile(const QString& _path, const char* _fileName);
	bool writeBinFile(const QString& _path);
private:
	virtual bool readBuff(const QByteArray& _buff) = 0;
	virtual void writeBuff(QByteArray& _buff) = 0;
protected:
	QStringPool* m_stringPool;
	QByteArray m_fileBuff;
};

#endif // QAndroidParser_h__
