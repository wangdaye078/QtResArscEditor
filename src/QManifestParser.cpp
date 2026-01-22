#include "common/basicDefine.h"
#include "QManifestParser.h"
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QTreeWidgetItem>
QManifestParser::QManifestParser(QObject* parent)
	: QAndroidParser(parent), m_xmlTree(this)
{
}

QManifestParser::~QManifestParser()
{
}
RES_TYPE QManifestParser::getParserType(void) const
{
	return RES_TYPE::RES_XML_TYPE;
}
const QString& QManifestParser::getBinFileSuffix(void) const
{
	static QString t_suffix = "xml";
	return t_suffix;
}
const char* QManifestParser::getBinFileName(void) const
{
	static char t_fileName[] = "AndroidManifest.xml";
	return t_fileName;
}
void QManifestParser::traversalSubItems(void)
{
	if (m_traversalAllXmlElementFunc)
	{
		PResXmlElement t_rootElement = m_xmlTree.getRootElement();
		QTreeWidgetItem* t_rootItem = m_traversalAllXmlElementFunc(NULL, QVariant::fromValue(t_rootElement));
		t_rootElement->traversalAllSubElement(m_traversalAllXmlElementFunc, t_rootItem);
		t_rootItem->setExpanded(true);
	}
}
void QManifestParser::setTraversalAllXmlElementFunc(TTRAVERSAL_ALL_ELEMENT _callBack)
{
	m_traversalAllXmlElementFunc = _callBack;
}
void QManifestParser::reset(void)
{
	m_xmlTree.reset();
}
bool QManifestParser::readBuff(const QByteArray& _buff)
{
	reset();
	if (_buff.size() < sizeof(ResXMLTree_header))
	{
		qDebug() << "read buffer too small, invalid data";
		return false;
	}
	const char* t_pBuff = _buff.constData();
	ResXMLTree_header t_xmltreeHeader = *reinterpret_cast<const ResXMLTree_header*>(t_pBuff);
	if (t_xmltreeHeader.header.type != RES_TYPE::RES_XML_TYPE)
	{
		qDebug() << "read buffer type error, invalid data";
		return false;
	}
	if (t_xmltreeHeader.header.headerSize != sizeof(ResChunk_header))
	{
		qDebug() << "read buffer header size error, invalid data";
		return false;
	}
	if (t_xmltreeHeader.header.size != _buff.size())
	{
		qDebug() << "read buffer size error, invalid data";
		return false;
	}
	t_pBuff += sizeof(t_xmltreeHeader);
	bool t_readOk = true;
	while (t_pBuff < _buff.constData() + _buff.size())
	{
		if (t_pBuff + sizeof(ResChunk_header) > _buff.constData() + _buff.size())
		{
			qDebug() << "read buffer overflow, invalid data";
			t_readOk = false;
			break;
		}
		const ResChunk_header* t_pHeader = reinterpret_cast<const ResChunk_header*>(t_pBuff);
		if (t_pBuff + t_pHeader->size > _buff.constData() + _buff.size())
		{
			qDebug() << "read buffer overflow, invalid chunk size";
			t_readOk = false;
			break;
		}
		switch (t_pHeader->type)
		{
		case RES_TYPE::RES_STRING_POOL_TYPE:
			//字符串池前面一部分是android属性，可能和后面的一些字符串有重复，所以需要加上“android:”前缀来区分，
			//但是这部分属性的数量在RES_XML_RESOURCE_MAP_TYPE里面，所以这里先缓存，而不处理
			m_stringPoolBuff = QByteArray(t_pBuff, t_pHeader->size);
			break;
		case RES_TYPE::RES_XML_RESOURCE_MAP_TYPE:
			//ResChunk_header + uint32_t[] resourceIds
			{
				int t_count = (t_pHeader->size - t_pHeader->headerSize) / sizeof(uint32_t);
				QVector<uint32_t> t_resourceIds(t_count);
				memcpy(t_resourceIds.data(), t_pBuff + t_pHeader->headerSize, t_count * sizeof(uint32_t));
				//读取字符串池
				m_stringPool->readBuff(m_stringPoolBuff.data(), t_resourceIds);
			}
			break;
		case RES_TYPE::RES_XML_START_NAMESPACE_TYPE:
			//ResXMLTree_node + ResXMLTree_namespaceExt
			m_xmlTree.reset();
			m_xmlTree.startNameSpace(t_pBuff);
			break;
		case RES_TYPE::RES_XML_END_NAMESPACE_TYPE:
			//ResXMLTree_node + ResXMLTree_namespaceExt
			m_xmlTree.endNameSpace(t_pBuff);
			break;
		case RES_TYPE::RES_XML_START_ELEMENT_TYPE:
			//ResXMLTree_node + ResXMLTree_attrExt + ResXMLTree_attribute[]
			m_xmlTree.startElement(t_pBuff);
			break;
		case RES_TYPE::RES_XML_END_ELEMENT_TYPE:
			//ResXMLTree_node + ResXMLTree_endElementExt
			m_xmlTree.endElement(t_pBuff);
			break;
		default:
			t_readOk = false;
			break;
		}
		t_pBuff += t_pHeader->size;
	}
	return t_readOk;
}
void QManifestParser::writeBuff(QByteArray& _buff)
{
	ResXMLTree_header t_tableHeader;
	t_tableHeader.header.type = RES_TYPE::RES_XML_TYPE;
	t_tableHeader.header.headerSize = sizeof(ResXMLTree_header);
	_buff.append(reinterpret_cast<const char*>(&t_tableHeader), sizeof(t_tableHeader));

	m_stringPool->writeBuff(_buff);

	const QVector<uint32_t>& t_resourceIds = m_stringPool->getResIDs();
	ResChunk_header t_resMapHeader;
	t_resMapHeader.type = RES_TYPE::RES_XML_RESOURCE_MAP_TYPE;
	t_resMapHeader.headerSize = sizeof(ResChunk_header);
	t_resMapHeader.size = sizeof(ResChunk_header) + static_cast<uint32_t>(t_resourceIds.size() * sizeof(uint32_t));
	_buff.append(reinterpret_cast<const char*>(&t_resMapHeader), sizeof(t_resMapHeader));
	_buff.append(reinterpret_cast<const char*>(t_resourceIds.data()), t_resourceIds.size() * sizeof(uint32_t));

	m_xmlTree.writeBuff(_buff);
	ResChunk_header* t_head = reinterpret_cast<ResChunk_header*>(_buff.data());
	t_head->size = static_cast<uint32_t>(_buff.size());
}
void QManifestParser::exportXml(QXmlStreamWriter& _xmlWriter) const
{
	m_xmlTree.exportXml(_xmlWriter);
}
