//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor2\src\QManifestParser.h
//	desc:		
//
//	created:	wangdaye 6:1:2026   19:44
//********************************************************************
#ifndef QManifestParser_h__
#define QManifestParser_h__

#include "QAndroidParser.h"
#include "QPublicFinal.h"
#include "QResXmlTree.h"
class QXmlStreamWriter;

class QManifestParser : public QAndroidParser
{
	Q_OBJECT
public:
	QManifestParser(QObject* _parent);
	QManifestParser(const QManifestParser&) = delete;
	QManifestParser& operator=(const QManifestParser&) = delete;
	~QManifestParser();
	RES_TYPE getParserType(void) const;
	const QString& getBinFileSuffix(void) const;
	const char* getBinFileName(void) const;
	void traversalSubItems(void);
	void setTraversalAllXmlElementFunc(TTRAVERSAL_ALL_ELEMENT _callBack);
	void exportXml(QXmlStreamWriter& _xmlWriter) const;
private:
	void reset(void);
	bool readBuff(const QByteArray& _buff);
	void writeBuff(QByteArray& _buff);
private:
	TTRAVERSAL_ALL_ELEMENT m_traversalAllXmlElementFunc;
	QByteArray m_stringPoolBuff;
	QResXmlTree m_xmlTree;
};
#endif // QManifestParser_h__
