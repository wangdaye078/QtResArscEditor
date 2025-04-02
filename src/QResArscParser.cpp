#include "QResArscParser.h"
#include <QFile>
#include <QDataStream>
#include <QtGlobal> 
#include <QtDebug>
#include <functional>
#include "ResArscStruct.h"
#include "basicDefine.h"
#include "SimpleRichText.h"
#include "QPublicFinal.h"


QResArscParser::QResArscParser(QObject* parent)
	: QObject(parent)
{
	m_publicFinal = new QPublicFinal(this);
}

QResArscParser::~QResArscParser()
{

}
void QResArscParser::reset(void)
{
	m_StringPool.reset();
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
			m_StringPool.readStrPool(t_pBuff);
			break;
		case RES_TYPE::RES_TABLE_PACKAGE_TYPE:
			{
				QString t_packageName = WCHARToQString(reinterpret_cast<const ResTable_package*>(t_pBuff)->name);
				m_tablePackages[t_packageName].readTablePackage(t_pBuff);
				m_tablePackages[t_packageName].setPublicData(&m_StringPool, m_publicFinal);
			}
			break;
		}
		t_pBuff += t_pHeader->size;
	}
	m_StringPool.initStringReferenceCount(std::bind(&QResArscParser::traversalAllValue, this, std::placeholders::_1));
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

	m_StringPool.writeStrPool(_buff);
	for (QMap<QString, TTablePackage>::iterator i = m_tablePackages.begin(); i != m_tablePackages.end(); ++i)
	{
		i.value().writeTablePackage(_buff);
	}
	ResChunk_header* t_head = reinterpret_cast<ResChunk_header*>(_buff.data());
	t_head->size = _buff.size();
}
void QResArscParser::traversalAllTablePackage(TTRAVERSAL_ALL_TABLEPACKAGE _callBack)
{
	for (QMap<QString, TTablePackage>::iterator i = m_tablePackages.begin(); i != m_tablePackages.end(); ++i)
		_callBack(i.key(), i.value());
}
TTablePackage& QResArscParser::tablePackage(const QString& _name)
{
	Q_ASSERT(m_tablePackages.contains(_name));
	return m_tablePackages[_name];
}
void QResArscParser::traversalAllValue(TRAVERSE_STRIDX_CALLBACK _callBack)
{
	for (QMap<QString, TTablePackage>::iterator i = m_tablePackages.begin(); i != m_tablePackages.end(); ++i)
	{
		i.value().traversalAllValue(_callBack);
	}
	m_StringPool.traversalAllValue(_callBack);
}
const QSharedPointer<ResTable_entry>& QResArscParser::getValue(TTablePackage& _package, uint32_t _typeid, uint32_t _specid, uint32_t _id) const
{
	const TTableTypeEx& t_type = _package.getTableType(_typeid, _specid);
	Q_ASSERT(_id < (quint32)t_type.entryValue.size());
	return t_type.entryValue[_id];
}
const QSharedPointer<ResTable_entry>& QResArscParser::getValueOrInsert(TTablePackage& _package, uint32_t _typeid, uint32_t _specid, uint32_t _id)
{
	const QSharedPointer<ResTable_entry>& t_ptrEntry = getValue(_package, _typeid, _specid, _id);
	if (!t_ptrEntry.isNull())
		return t_ptrEntry;
	_package.copyValue(_typeid, _specid, _id);
	return getValue(_package, _typeid, _specid, _id);
}
const Res_value* QResArscParser::setValue(TTablePackage& _package, uint32_t _typeid, uint32_t _specid, uint32_t _id, Res_value::_DataType _dataYype, uint32_t _data)
{
	const QSharedPointer<ResTable_entry>& t_ptrEntry = getValueOrInsert(_package, _typeid, _specid, _id);
	Q_ASSERT(!t_ptrEntry.isNull());
	Q_ASSERT((*t_ptrEntry).size == sizeof(ResTable_entry));
	TTableValueEntry* t_EntryValue = reinterpret_cast<TTableValueEntry*>(t_ptrEntry.get());
	if (t_EntryValue->value.dataType == Res_value::_DataType::TYPE_STRING)
		m_StringPool.deleteString(t_EntryValue->value.data, false, std::bind(&QResArscParser::traversalAllValue, this, std::placeholders::_1));

	t_EntryValue->value.dataType = _dataYype;
	t_EntryValue->value.data = _data;
	return &t_EntryValue->value;
}
const Res_value* QResArscParser::setValue(TTablePackage& _package, uint32_t _typeid, uint32_t _specid, uint32_t _id, uint32_t _idx, Res_value::_DataType _dataYype, uint32_t _data)
{
	const QSharedPointer<ResTable_entry>& t_ptrEntry = getValueOrInsert(_package, _typeid, _specid, _id);
	Q_ASSERT(!t_ptrEntry.isNull());
	Q_ASSERT((*t_ptrEntry).size != sizeof(ResTable_entry));
	TTableMapEntry* t_pMapValue = reinterpret_cast<TTableMapEntry*>(t_ptrEntry.get());
	Q_ASSERT(_idx < (uint32_t)t_pMapValue->tablemap.size());
	ResTable_map& t_tableMap = t_pMapValue->tablemap[_idx];
	if (t_tableMap.value.dataType == Res_value::_DataType::TYPE_STRING)
		m_StringPool.deleteString(t_tableMap.value.data, false, std::bind(&QResArscParser::traversalAllValue, this, std::placeholders::_1));
	t_tableMap.value.dataType = _dataYype;
	t_tableMap.value.data = _data;
	return &t_tableMap.value;
}
const Res_value* QResArscParser::setValue(TTablePackage& _package, uint32_t _typeid, uint32_t _specid, uint32_t _id, const QString& _data, bool _force)
{
	QString t_str = _data;
	t_str.replace(QString("\\n"), QChar(0x0A));

	const QSharedPointer<ResTable_entry>& t_ptrEntry = getValueOrInsert(_package, _typeid, _specid, _id);
	Q_ASSERT(!t_ptrEntry.isNull());
	Q_ASSERT((*t_ptrEntry).size == sizeof(ResTable_entry));
	TRichString t_rich = decodeRichText(t_str, m_StringPool);
	TTableValueEntry* t_EntryValue = reinterpret_cast<TTableValueEntry*>(t_ptrEntry.get());
	if (t_EntryValue->value.dataType == Res_value::_DataType::TYPE_STRING)
		t_EntryValue->value.data = m_StringPool.replaceString(t_EntryValue->value.data, t_rich, _force, std::bind(&QResArscParser::traversalAllValue, this, std::placeholders::_1));
	else
		t_EntryValue->value.data = m_StringPool.insertString(t_rich, 1, NULL, std::bind(&QResArscParser::traversalAllValue, this, std::placeholders::_1));
	return &t_EntryValue->value;
}
const Res_value* QResArscParser::setValue(TTablePackage& _package, uint32_t _typeid, uint32_t _specid, uint32_t _id, uint32_t _idx, const QString& _data, bool _force)
{
	QString t_str = _data;
	t_str.replace(QString("\\n"), QChar(0x0A));

	const QSharedPointer<ResTable_entry>& t_ptrEntry = getValueOrInsert(_package, _typeid, _specid, _id);
	Q_ASSERT(!t_ptrEntry.isNull());
	Q_ASSERT((*t_ptrEntry).size != sizeof(ResTable_entry));
	TTableMapEntry* t_pMapValue = reinterpret_cast<TTableMapEntry*>(t_ptrEntry.get());
	Q_ASSERT(_idx < (uint32_t)t_pMapValue->tablemap.size());
	TRichString t_rich = decodeRichText(t_str, m_StringPool);
	ResTable_map& t_tableMap = t_pMapValue->tablemap[_idx];
	if (t_tableMap.value.dataType == Res_value::_DataType::TYPE_STRING)
		t_tableMap.value.data = m_StringPool.replaceString(t_tableMap.value.data, t_rich, _force, std::bind(&QResArscParser::traversalAllValue, this, std::placeholders::_1));
	else
		t_tableMap.value.data = m_StringPool.insertString(t_rich, 1, NULL, std::bind(&QResArscParser::traversalAllValue, this, std::placeholders::_1));
	return &t_tableMap.value;
}
quint32 QResArscParser::getReferenceCount(quint32 _index)
{
	return m_StringPool.incReferenceCount(_index, 0);
}
