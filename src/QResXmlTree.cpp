#include "GuidFactory.h"
#include "QAndroidParser.h"
#include "QResXmlTree.h"
#include <QDebug>
#include <QList>
#include <QXmlStreamWriter>
uint32_t TXmlAttrEntry::readBuff(const char* _buff)
{
	attribute = *reinterpret_cast<const ResXMLTree_attribute*>(_buff);
	if (attribute.ns.index != ResStringPool_ref::END)
		snameSpace = m_stringPool->getGuidRef(attribute.ns.index);
	sname = m_stringPool->getGuidRef(attribute.name.index);
	if (attribute.typedValue.dataType == Res_value::_DataType::TYPE_STRING)
	{
		svalue = m_stringPool->getGuidRef(attribute.typedValue.data);
		Q_ASSERT(attribute.typedValue.data == attribute.rawValue.index);
	}
	else
		Q_ASSERT(ResStringPool_ref::END == attribute.rawValue.index);
	return sizeof(ResXMLTree_attribute);
}
void TXmlAttrEntry::writeBuff(const QStringPool* _keyPool, QByteArray& _buff)
{
	Q_ASSERT(_keyPool == m_stringPool);
	ResXMLTree_attribute t_newAttr = attribute;
	if (attribute.ns.index != ResStringPool_ref::END)
		attribute.ns.index = m_stringPool->getRefIndex(snameSpace);
	if ((t_newAttr.name.index >> 16) == 0)
	{
		//如果m_keyStringPool内容或者排列有修改，那就需要重新计算索引，而且不能在原有entry上面修改，需要复制一个新的entry，
		uint32_t t_newIdx = _keyPool->getRefIndex(sname);
		if (t_newIdx != t_newAttr.name.index)
			t_newAttr.name.index = t_newIdx;
	}
	if (t_newAttr.typedValue.dataType == Res_value::_DataType::TYPE_STRING)
	{
		t_newAttr.typedValue.data = m_stringPool->getRefIndex(svalue);
		Q_ASSERT(t_newAttr.typedValue.data == svalue->index);
		t_newAttr.rawValue.index = t_newAttr.typedValue.data;
	}
	else
	{
		t_newAttr.rawValue.index = ResStringPool_ref::END;
	}
	_buff.append(reinterpret_cast<const char*>(&t_newAttr), sizeof(ResXMLTree_attribute));
}
void TXmlAttrEntry::setValue(const Res_value& _value)
{
	attribute.typedValue = _value;
	if (attribute.typedValue.dataType == Res_value::_DataType::TYPE_STRING)
	{
		svalue = m_stringPool->getGuidRef(attribute.typedValue.data);
		attribute.rawValue.index = attribute.typedValue.data;
	}
	else
	{
		svalue.reset();
		attribute.rawValue.index = ResStringPool_ref::END;
	}
}
//---------------------------------------
QResXmlElement::QResXmlElement(QResXmlElement* _parent) :
	m_parentElement(_parent)
{
	m_node.header.type = RES_TYPE::RES_XML_START_ELEMENT_TYPE;
	m_node.header.headerSize = sizeof(ResXMLTree_node);
	m_node.header.size = 0;	//后面写入的时候再计算
	m_node.lineNumber = 0;
	m_node.comment.index = ResStringPool_ref::END;
	m_attrExt.ns.index = ResStringPool_ref::END;
	m_attrExt.name.index = ResStringPool_ref::END;
	m_attrExt.attributeStart = sizeof(ResXMLTree_attrExt);
	m_attrExt.attributeSize = sizeof(ResXMLTree_attribute);
	m_attrExt.attributeCount = 0;	//后面写入的时候再确定
	m_attrExt.idIndex = 0;
	m_attrExt.classIndex = 0;
	m_attrExt.styleIndex = 0;
}
QResXmlElement::~QResXmlElement()
{

}
void QResXmlElement::startElement(const char* _pBegin)
{
	const char* t_pBuff = _pBegin;
	m_node = *reinterpret_cast<const ResXMLTree_node*>(t_pBuff);
	Q_ASSERT(m_node.header.type == RES_TYPE::RES_XML_START_ELEMENT_TYPE);
	Q_ASSERT(m_node.header.headerSize == sizeof(ResXMLTree_node));
	Q_ASSERT(m_node.comment.index == ResStringPool_ref::END);
	t_pBuff += sizeof(ResXMLTree_node);
	m_attrExt = *reinterpret_cast<const ResXMLTree_attrExt*>(t_pBuff);
	Q_ASSERT(m_attrExt.ns.index == ResStringPool_ref::END);
	Q_ASSERT(m_attrExt.attributeStart == sizeof(ResXMLTree_attrExt));
	Q_ASSERT(m_attrExt.attributeSize == sizeof(ResXMLTree_attribute));
	m_attrExt.idIndex = 0;	//回写的时候重填
	m_attrExt.classIndex = 0;
	m_attrExt.styleIndex = 0;
	m_nodeName = m_xmlTree->getParentParser()->getStringPool()->getGuidRef(m_attrExt.name.index);
	t_pBuff += sizeof(ResXMLTree_attrExt);

	for (int i = 0; i < m_attrExt.attributeCount; ++i)
	{
		TXmlAttrEntry* t_attrEntry = new TXmlAttrEntry(m_xmlTree->getParentParser()->getStringPool());
		t_pBuff += t_attrEntry->readBuff(t_pBuff);
		m_attributes.push_back(PResValue(t_attrEntry));
	}
}
void QResXmlElement::endElement(const char* _pBegin)
{
	const char* t_pBuff = _pBegin;
	ResXMLTree_node t_nodeEnd = *reinterpret_cast<const ResXMLTree_node*>(t_pBuff);
	Q_ASSERT(t_nodeEnd.header.type == RES_TYPE::RES_XML_END_ELEMENT_TYPE);
	Q_ASSERT(t_nodeEnd.header.headerSize == sizeof(ResXMLTree_node));
	Q_ASSERT(t_nodeEnd.comment.index == ResStringPool_ref::END);
	t_pBuff += sizeof(ResXMLTree_node);
	ResXMLTree_endElementExt t_endExt = *reinterpret_cast<const ResXMLTree_endElementExt*>(t_pBuff);
	Q_ASSERT(t_endExt.ns.index == m_attrExt.ns.index);
	Q_ASSERT(t_endExt.name.index == m_attrExt.name.index);
}
const QResXmlTree* QResXmlElement::getXmlTree(void) const
{
	return m_xmlTree;
}
PResXmlElement QResXmlElement::appendSubElement(PArscRichString _name)
{
	PResXmlElement t_newElement(new QResXmlElement(this));
	t_newElement->m_xmlTree = m_xmlTree;
	if (_name)
	{
		t_newElement->m_attrExt.name.index = _name->guid;
		t_newElement->m_nodeName = _name;
	}
	m_subElements.append(t_newElement);
	return t_newElement;
}
QResXmlElement* QResXmlElement::parentElement(void)
{
	return m_parentElement;
}
void QResXmlElement::writeBuff(QByteArray& _buff)
{
	int t_startElement_pos = _buff.size();
	_buff.append(reinterpret_cast<const char*>(&m_node), sizeof(ResXMLTree_node));

	int t_attrExt_pos = _buff.size();
	ResXMLTree_attrExt t_attrExt = m_attrExt;
	if ((t_attrExt.name.index >> 16) == 0)
	{
		uint32_t t_newIdx = m_xmlTree->getParentParser()->getStringPool()->getRefIndex(m_nodeName);
		if (t_newIdx != t_attrExt.name.index)
			t_attrExt.name.index = t_newIdx;
	}
	t_attrExt.attributeCount = static_cast<uint16_t>(m_attributes.size());
	_buff.append(reinterpret_cast<const char*>(&t_attrExt), sizeof(ResXMLTree_attrExt));
	ResXMLTree_attrExt* t_pAttrExt = reinterpret_cast<ResXMLTree_attrExt*>(_buff.data() + t_attrExt_pos);
	for (int i = 0; i < m_attributes.size(); ++i)
	{
		TXmlAttrEntry* t_attr = reinterpret_cast<TXmlAttrEntry*>(m_attributes[i].get());
		t_attr->writeBuff(m_xmlTree->getParentParser()->getStringPool(), _buff);
		if (t_attr->attribute.ns.index != ResStringPool_ref::END)
		{
			QString t_prefix = m_xmlTree->getNameSpacePrefix(t_attr->snameSpace->string, NULL)->string;
			if (t_prefix == "android")
			{
				if (t_pAttrExt->idIndex == 0 && t_attr->sname->string == "id")
					t_pAttrExt->idIndex = static_cast<uint16_t>(i + 1);
			}
		}
		else
		{
			if (t_pAttrExt->styleIndex == 0 && t_attr->sname->string == "style")
				t_pAttrExt->styleIndex = static_cast<uint16_t>(i + 1);
			else if (t_pAttrExt->classIndex == 0 && t_attr->sname->string == "class")
				t_pAttrExt->classIndex = static_cast<uint16_t>(i + 1);
		}
	}
	reinterpret_cast<ResChunk_header*>(_buff.data() + t_startElement_pos)->size = _buff.size() - t_startElement_pos;

	for (int i = 0; i < m_subElements.size(); ++i)
		m_subElements[i]->writeBuff(_buff);

	ResXMLTree_node t_node = m_node;
	t_node.header.type = RES_TYPE::RES_XML_END_ELEMENT_TYPE;
	t_node.header.size = sizeof(ResXMLTree_node) + sizeof(ResXMLTree_endElementExt);
	_buff.append(reinterpret_cast<const char*>(&t_node), sizeof(ResXMLTree_node));

	ResXMLTree_endElementExt t_endExt;
	t_endExt.ns = t_attrExt.ns;
	t_endExt.name = t_attrExt.name;
	_buff.append(reinterpret_cast<const char*>(&t_endExt), sizeof(ResXMLTree_endElementExt));
}
void QResXmlElement::traversalAllSubElement(TTRAVERSAL_ALL_ELEMENT _callBack, QTreeWidgetItem* _parent) const
{
	for (int i = 0; i < m_subElements.size(); ++i)
	{
		QTreeWidgetItem* t_subItem = _callBack(_parent, QVariant::fromValue(m_subElements[i]));
		m_subElements[i]->traversalAllSubElement(_callBack, t_subItem);
	}
}
const ResXMLTree_attrExt& QResXmlElement::getAttrExt(void) const
{
	return m_attrExt;
}
const QVector<PResValue>& QResXmlElement::getAttributes(void)
{
	return m_attributes;
}
void QResXmlElement::exportXml(QXmlStreamWriter& _xmlWriter) const
{
	for (int i = 0; i < m_attributes.size(); ++i)
	{
		const TXmlAttrEntry* t_attr = reinterpret_cast<const TXmlAttrEntry*>(m_attributes[i].get());
		PArscRichString t_sValue;
		if (t_attr->attribute.typedValue.dataType == Res_value::_DataType::TYPE_STRING)
			t_sValue = m_xmlTree->getParentParser()->getStringPool()
			->getGuidRef(t_attr->attribute.typedValue.data);
		_xmlWriter.writeAttribute(t_attr->sname->string, resValue2String(t_attr->sname->string, t_attr->attribute.typedValue, t_sValue));
	}
	for (int i = 0; i < m_subElements.size(); ++i)
	{
		const PResXmlElement& t_subElement = m_subElements[i];
		_xmlWriter.writeStartElement(t_subElement->m_nodeName->string);
		t_subElement->exportXml(_xmlWriter);
		_xmlWriter.writeEndElement();
	}
}
void QResXmlElement::removeAttribute(int _index)
{
	Q_ASSERT(_index >= 0 && _index < m_attributes.size());
	m_attributes.removeAt(_index);
}
void QResXmlElement::addAttribute(PResValue _attr)
{
	m_attributes.push_back(_attr);
}
void QResXmlElement::removeSubElement(int _index)
{
	Q_ASSERT(_index >= 0 && _index < m_subElements.size());
	m_subElements.removeAt(_index);
}
void QResXmlElement::moveSubElement(int _from, int _to)
{
	Q_ASSERT(_from >= 0 && _from < m_subElements.size());
	Q_ASSERT(_to >= 0 && _to < m_subElements.size());
	m_subElements.move(_from, _to);
}
//-------------------
QResXmlTree::QResXmlTree(QAndroidParser* _parent) :
	QObject(_parent), m_parentParser(_parent)
{

}
QResXmlTree::~QResXmlTree()
{
	reset();
}
void QResXmlTree::reset(void)
{
	m_rootElement.reset();
}
void QResXmlTree::startNameSpace(const char* _pBegin)
{
	reset();
	const char* t_pBuff = _pBegin;
	ResXMLTree_node t_nameSpaceNode = *reinterpret_cast<const ResXMLTree_node*>(t_pBuff);
	Q_ASSERT(t_nameSpaceNode.header.type == RES_TYPE::RES_XML_START_NAMESPACE_TYPE);
	Q_ASSERT(t_nameSpaceNode.header.headerSize == sizeof(ResXMLTree_node));
	Q_ASSERT(t_nameSpaceNode.comment.index == ResStringPool_ref::END);
	m_nameSpaceLineNumber = t_nameSpaceNode.lineNumber;		//不管有几个namespace，lineNumber都一样的
	t_pBuff += sizeof(ResXMLTree_node);
	ResXMLTree_namespaceExt t_nameSpaceExt = *reinterpret_cast<const ResXMLTree_namespaceExt*>(t_pBuff);
	TXmlNameSpaceExt t_nameSpace;
	t_nameSpace.prefix = m_parentParser->getStringPool()->getGuidRef(t_nameSpaceExt.prefix.index);
	t_nameSpace.uri = m_parentParser->getStringPool()->getGuidRef(t_nameSpaceExt.uri.index);
	m_nameSpaces.push_back(t_nameSpace);
	m_nameSpaceUrlMap.insert(t_nameSpace.uri->string, t_nameSpace.prefix);
	m_nameSpacePrefixMap.insert(t_nameSpace.prefix->string, t_nameSpace.uri);
}
void QResXmlTree::endNameSpace(const char* _pBegin)
{
	//目前不做任何处理，endNameSpace和startNameSpace成对出现即可
}
void QResXmlTree::startElement(const char* _pBegin)
{
	if (!m_rootElement)
	{
		m_rootElement = PResXmlElement(new QResXmlElement(NULL));
		m_rootElement->m_xmlTree = this;
		m_currentElement = m_rootElement.data();
	}
	else
		m_currentElement = m_currentElement->appendSubElement(PArscRichString(NULL)).data();
	m_currentElement->startElement(_pBegin);
}
void QResXmlTree::endElement(const char* _pBegin)
{
	Q_ASSERT(m_rootElement);
	m_currentElement->endElement(_pBegin);
	m_currentElement = m_currentElement->parentElement();
}
void QResXmlTree::writeBuff(QByteArray& _buff)
{
	struct TResXMLTree_NS
	{
		ResXMLTree_node node;
		ResXMLTree_namespaceExt ext;
	};
	QByteArray t_nameSpaceBuff;
	for (int i = 0; i < m_nameSpaces.size(); ++i)
	{
		const TXmlNameSpaceExt& t_nameSpaceExt = m_nameSpaces[i];
		TResXMLTree_NS t_nameSpace;
		t_nameSpace.node.header.type = RES_TYPE::RES_XML_START_NAMESPACE_TYPE;
		t_nameSpace.node.header.headerSize = sizeof(ResXMLTree_node);
		t_nameSpace.node.header.size = sizeof(ResXMLTree_node) + sizeof(ResXMLTree_namespaceExt);
		t_nameSpace.node.lineNumber = m_nameSpaceLineNumber;
		t_nameSpace.node.comment.index = ResStringPool_ref::END;
		t_nameSpace.ext.prefix.index = m_parentParser->getStringPool()->getRefIndex(t_nameSpaceExt.prefix);
		t_nameSpace.ext.uri.index = m_parentParser->getStringPool()->getRefIndex(t_nameSpaceExt.uri);
		_buff.append(reinterpret_cast<const char*>(&t_nameSpace), sizeof(TResXMLTree_NS));

		//保证后写入的RES_XML_START_NAMESPACE_TYPE和先写入的RES_XML_END_NAMESPACE_TYPE配对
		t_nameSpace.node.header.type = RES_TYPE::RES_XML_END_NAMESPACE_TYPE;
		t_nameSpaceBuff.insert(0, reinterpret_cast<const char*>(&t_nameSpace), sizeof(TResXMLTree_NS));
	}
	m_rootElement->writeBuff(_buff);

	_buff.append(t_nameSpaceBuff);
}
PResXmlElement QResXmlTree::getRootElement(void) const
{
	return m_rootElement;
}
void QResXmlTree::exportXml(QXmlStreamWriter& _xmlWriter) const
{
	_xmlWriter.writeStartElement(m_rootElement->m_nodeName->string);
	for (int i = 0; i < m_nameSpaces.size(); ++i)
	{
		const TXmlNameSpaceExt& t_nameSpace = m_nameSpaces[i];
		_xmlWriter.writeNamespace(t_nameSpace.uri->string, t_nameSpace.prefix->string);
	}
	m_rootElement->exportXml(_xmlWriter);
	_xmlWriter.writeEndElement();
}
PArscRichString QResXmlTree::getNameSpacePrefix(const QString& _uri, bool* _ok) const
{
	if (_ok)
		*_ok = m_nameSpaceUrlMap.contains(_uri);
	return m_nameSpaceUrlMap.value(_uri);
}
PArscRichString QResXmlTree::getNameSpaceUrl(const QString& _Prefix, bool* _ok) const
{
	if (_ok)
		*_ok = m_nameSpacePrefixMap.contains(_Prefix);
	return m_nameSpacePrefixMap.value(_Prefix);
}
QAndroidParser* QResXmlTree::getParentParser(void) const
{
	return m_parentParser;
}
