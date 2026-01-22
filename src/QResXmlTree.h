//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor2\src\QResXmlTree.h
//	desc:		
//
//	created:	wangdaye 7:1:2026   19:39
//********************************************************************
#ifndef QResXmlTree_h__
#define QResXmlTree_h__
#include "common/shared_ptr.h"
#include "QTableType.h"
#include "ResArscStruct.h"
#include <QObject>
#include <QSharedPointer>
#include <QVector>
class QResXmlElement;
using PResXmlElement = QSharedPointer<QResXmlElement>;
Q_DECLARE_METATYPE(PResXmlElement)

class QTreeWidgetItem;
using TTRAVERSAL_ALL_ELEMENT = std::function< QTreeWidgetItem* (QTreeWidgetItem* _parent, const QVariant& _v) >;

class QXmlStreamWriter;

struct TXmlAttrEntry : public IResValue
{
	ResXMLTree_attribute attribute;
	PArscRichString snameSpace;
	PArscRichString sname;
	PArscRichString svalue;
	//
	TXmlAttrEntry(QStringPool* _stringPool) : IResValue(_stringPool) {}
	~TXmlAttrEntry() override {};
	uint32_t readBuff(const char* _buff) override;
	void writeBuff(const QStringPool* _keyPool, QByteArray& _buff) override;
	EValueItemType getValueType(void) override { return  eValueItemType_value; }
	PArscRichString getValue(Res_value** _value) override
	{
		if (_value != NULL)
			*_value = &attribute.typedValue;
		return svalue;
	}
	void setValue(const Res_value& _value) override;
	uint32_t getKeyIndex(void) override
	{
		return attribute.name.index;
	}
	IResValue* clone() const override
	{
		TXmlAttrEntry* t_clone = new TXmlAttrEntry(m_stringPool);
		t_clone->attribute = attribute;
		t_clone->svalue = svalue;
		return t_clone;
	}
};

class QResXmlElement
{
	friend class QResXmlTree;
public:
	QResXmlElement(QResXmlElement* _parent);
	QResXmlElement(const QResXmlElement&) = delete;
	QResXmlElement& operator=(const QResXmlElement&) = delete;
	~QResXmlElement();
	void startElement(const char* _pBegin);
	void endElement(const char* _pBegin);
	const QResXmlTree* getXmlTree(void) const;
	PResXmlElement appendSubElement(PArscRichString _name);
	QResXmlElement* parentElement(void);
	void writeBuff(QByteArray& _buff);
	void traversalAllSubElement(TTRAVERSAL_ALL_ELEMENT _callBack, QTreeWidgetItem* _parent) const;
	const ResXMLTree_attrExt& getAttrExt(void) const;
	const QVector<PResValue>& getAttributes(void);
	void exportXml(QXmlStreamWriter& _xmlWriter) const;
	void removeAttribute(int _index);
	void addAttribute(PResValue _attr);
	void removeSubElement(int _index);
	void moveSubElement(int _from, int _to);
private:
	QResXmlTree* m_xmlTree;
	ResXMLTree_node m_node;
	ResXMLTree_attrExt m_attrExt;
	PArscRichString m_nodeName;
	QVector<PResValue> m_attributes;
	QVector<PResXmlElement> m_subElements;
	QResXmlElement* m_parentElement;
};

struct TXmlNameSpaceExt
{
	PArscRichString prefix;
	PArscRichString uri;
};
class QResXmlTree : public QObject
{
public:
	QResXmlTree(QAndroidParser* _parent);
	QResXmlTree(const QResXmlTree&) = delete;
	QResXmlTree& operator=(const QResXmlTree&) = delete;
	~QResXmlTree();
	void reset(void);
	void startNameSpace(const char* _pBegin);
	void endNameSpace(const char* _pBegin);
	void startElement(const char* _pBegin);
	void endElement(const char* _pBegin);
	void writeBuff(QByteArray& _buff);
	PResXmlElement getRootElement(void) const;
	void exportXml(QXmlStreamWriter& _xmlWriter) const;
	PArscRichString getNameSpacePrefix(const QString& _uri, bool* _ok) const;
	PArscRichString getNameSpaceUrl(const QString& _Prefix, bool* _ok) const;
	QAndroidParser* getParentParser(void) const;
private:
	QAndroidParser* m_parentParser;
	QVector<TXmlNameSpaceExt> m_nameSpaces;
	QMap<QString, PArscRichString> m_nameSpaceUrlMap;	//URL->Prefix
	QMap<QString, PArscRichString> m_nameSpacePrefixMap;	//Prefix->URL
	uint32_t m_nameSpaceLineNumber;
	PResXmlElement m_rootElement;
	QResXmlElement* m_currentElement;
};


#endif // QResXmlTree_h__
