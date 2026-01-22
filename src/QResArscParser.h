//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor2\src\QResArscParser.h
//	desc:		
//
//	created:	wangdaye 22:6:2025   11:48
//********************************************************************
#ifndef QResArscParser_h__
#define QResArscParser_h__

#include "GuidFactory.h"
#include "QAndroidParser.h"
#include "QTablePackage.h"
#include <QMap>

using TTRAVERSAL_ALL_TABLEPACKAGE = std::function< void(const QString&, const PTablePackage&) >;
struct zip_source;

class QResArscParser : public QAndroidParser
{
	Q_OBJECT
public:
	QResArscParser(QObject* _parent);
	QResArscParser(const QResArscParser&) = delete;
	QResArscParser& operator=(const QResArscParser&) = delete;
	~QResArscParser();
	RES_TYPE getParserType(void) const;
	const QString& getBinFileSuffix(void) const;
	const char* getBinFileName(void) const;
	void traversalSubItems(void);
	void setTraversalAllTablePackageFunc(TTRAVERSAL_ALL_TABLEPACKAGE _callBack);
private:
	void reset(void);
	bool readBuff(const QByteArray& _buff);
	void writeBuff(QByteArray& _buff);
	void traversalAllTablePackage(TTRAVERSAL_ALL_TABLEPACKAGE _callBack);
private:
	TTRAVERSAL_ALL_TABLEPACKAGE m_traversalAllTablePackageFunc;
	QMap<QString, PTablePackage> m_tablePackages;
};

#endif // QResArscParser_h__
