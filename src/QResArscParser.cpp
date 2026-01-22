#include "common/basicDefine.h"
#include "common/zip.h"
#include "QResArscParser.h"
#include <QDebug>
#include <QFile>
#include <QFileInfo>

QResArscParser::QResArscParser(QObject* parent)
	: QAndroidParser(parent)
{
}
QResArscParser::~QResArscParser()
{
}
RES_TYPE QResArscParser::getParserType(void) const
{
	return RES_TYPE::RES_TABLE_TYPE;
}
const QString& QResArscParser::getBinFileSuffix(void) const
{
	static QString t_suffix = "arsc";
	return t_suffix;
}
const char* QResArscParser::getBinFileName(void) const
{
	static char t_fileName[] = "resources.arsc";
	return t_fileName;
}
void QResArscParser::reset(void)
{
	m_tablePackages.clear();
}
bool QResArscParser::readBuff(const QByteArray& _buff)
{
	reset();
	if (_buff.size() < sizeof(ResTable_header))
	{
		qDebug() << "read buffer too small, invalid data";
		return false;
	}
	const char* t_pBuff = _buff.constData();
	ResTable_header t_tableHeader = *reinterpret_cast<const ResTable_header*>(t_pBuff);
	if (t_tableHeader.header.type != RES_TYPE::RES_TABLE_TYPE)
	{
		qDebug() << "read buffer type error, invalid data";
		return false;
	}
	if (t_tableHeader.header.headerSize != sizeof(ResTable_header))
	{
		qDebug() << "read buffer header size error, invalid data";
		return false;
	}
	if (t_tableHeader.header.size != _buff.size())
	{
		qDebug() << "read buffer size error, invalid data";
		return false;
	}
	t_pBuff += sizeof(t_tableHeader);
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
			m_stringPool->readBuff(t_pBuff, QVector<uint32_t>());
			break;
		case RES_TYPE::RES_TABLE_PACKAGE_TYPE:
			{
				QString t_packageName = WCHARToQString(reinterpret_cast<const ResTable_package*>(t_pBuff)->name);
				PTablePackage t_pTablePackage(new QTablePackage(this));
				t_pTablePackage->readBuff(t_pBuff);
				m_tablePackages[t_packageName] = t_pTablePackage;
			}
			break;
		default:
			t_readOk = false;
			break;
		}
		t_pBuff += t_pHeader->size;
	}
	return t_readOk;
}
void QResArscParser::traversalSubItems(void)
{
	if (m_traversalAllTablePackageFunc)
		traversalAllTablePackage(m_traversalAllTablePackageFunc);
}
void QResArscParser::setTraversalAllTablePackageFunc(TTRAVERSAL_ALL_TABLEPACKAGE _callBack)
{
	m_traversalAllTablePackageFunc = _callBack;
}
void QResArscParser::writeBuff(QByteArray& _buff)
{
	ResTable_header t_tableHeader;
	t_tableHeader.header.type = RES_TYPE::RES_TABLE_TYPE;
	t_tableHeader.header.headerSize = sizeof(ResTable_header);
	t_tableHeader.packageCount = m_tablePackages.size();
	_buff.append(reinterpret_cast<const char*>(&t_tableHeader), sizeof(t_tableHeader));

	m_stringPool->writeBuff(_buff);
	for (QMap<QString, PTablePackage>::iterator i = m_tablePackages.begin(); i != m_tablePackages.end(); ++i)
	{
		i.value()->writeBuff(_buff);
	}
	ResChunk_header* t_head = reinterpret_cast<ResChunk_header*>(_buff.data());
	t_head->size = static_cast<uint32_t>(_buff.size());
}
void QResArscParser::traversalAllTablePackage(TTRAVERSAL_ALL_TABLEPACKAGE _callBack)
{
	for (QMap<QString, PTablePackage>::iterator i = m_tablePackages.begin(); i != m_tablePackages.end(); ++i)
		_callBack(i.key(), i.value());
}

