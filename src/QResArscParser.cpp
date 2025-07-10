#include "QResArscParser.h"
#include <QFile>
#include "common/basicDefine.h"
#include "ResArscStruct.h"
QResArscParser::QResArscParser(QObject* parent)
	: QObject(parent)
{
	m_publicFinal = new QPublicFinal(this);
	m_StringPool = new QStringPool(true, this);
	g_publicStrPool = m_StringPool;
	g_publicFinal = m_publicFinal;
}

QResArscParser::~QResArscParser()
{
	g_publicFinal = NULL;
	g_publicStrPool = NULL;
}

void QResArscParser::reset(void)
{
	m_tablePackages.clear();
}
void QResArscParser::readFile(const QString& _path)
{
	QFile t_file(_path);
	if (t_file.exists() && t_file.open(QIODevice::ReadOnly))
	{
		QByteArray t_buff = t_file.readAll();
		readBuff(t_buff);
		t_file.close();
	}
}
void QResArscParser::readBuff(const QByteArray& _buff)
{
	reset();
	const char* t_pBuff = _buff.constData();
	ResTable_header t_tableHeader = *reinterpret_cast<const ResTable_header*>(t_pBuff);
	Q_ASSERT(t_tableHeader.header.type == RES_TYPE::RES_TABLE_TYPE);
	Q_ASSERT(t_tableHeader.header.headerSize == sizeof(ResTable_header));
	Q_ASSERT(t_tableHeader.header.size == _buff.size());
	t_pBuff += sizeof(t_tableHeader);

	while (t_pBuff < _buff.constData() + _buff.size())
	{
		const ResChunk_header* t_pHeader = reinterpret_cast<const ResChunk_header*>(t_pBuff);
		switch (t_pHeader->type)
		{
		case RES_TYPE::RES_STRING_POOL_TYPE:
			m_StringPool->readBuff(t_pBuff);
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
			Q_ASSERT(false);
			break;
		}
		t_pBuff += t_pHeader->size;
	}
}
bool QResArscParser::writeFile(const QString& _path)
{
	QFile t_file(_path);
	if (t_file.open(QIODevice::WriteOnly))
	{
		QByteArray t_buff;
		writeBuff(t_buff);
		t_file.write(t_buff);
		t_file.close();
		return true;
	}
	return false;
}
void QResArscParser::writeBuff(QByteArray& _buff)
{
	ResTable_header t_tableHeader;
	t_tableHeader.header.type = RES_TYPE::RES_TABLE_TYPE;
	t_tableHeader.header.headerSize = sizeof(ResTable_header);
	t_tableHeader.packageCount = m_tablePackages.size();
	_buff.append(reinterpret_cast<const char*>(&t_tableHeader), sizeof(t_tableHeader));

	m_StringPool->writeBuff(_buff);
	for (QMap<QString, PTablePackage>::iterator i = m_tablePackages.begin(); i != m_tablePackages.end(); ++i)
	{
		i.value()->writeBuff(_buff);
	}
	ResChunk_header* t_head = reinterpret_cast<ResChunk_header*>(_buff.data());
	t_head->size = _buff.size();
}
void QResArscParser::traversalAllTablePackage(TTRAVERSAL_ALL_TABLEPACKAGE _callBack)
{
	for (QMap<QString, PTablePackage>::iterator i = m_tablePackages.begin(); i != m_tablePackages.end(); ++i)
		_callBack(i.key(), i.value());
}
