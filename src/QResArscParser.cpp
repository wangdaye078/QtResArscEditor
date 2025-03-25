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
	m_typeStringPool.reset();
	m_keyStringPool.reset();
	m_tableTypeDatas.clear();
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
	Q_ASSERT(t_tableHeader.packageCount == 1);
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
			readTablePackage(t_pBuff);
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
	t_tableHeader.packageCount = 1;
	_buff.append(reinterpret_cast<const char*>(&t_tableHeader), sizeof(t_tableHeader));

	m_StringPool.writeStrPool(_buff);
	writeTablePackage(_buff);

	ResChunk_header* t_head = reinterpret_cast<ResChunk_header*>(_buff.data());
	t_head->size = _buff.size();
}
const ResTable_package& QResArscParser::tablePackage(void)
{
	return m_tablePackage;
}
const TStringPool& QResArscParser::typeString(void)
{
	return m_typeStringPool;
}
const QMap<uint, TTableTypeData>& QResArscParser::tableTypeDatas(void)
{
	return m_tableTypeDatas;
}
QString QResArscParser::keyString(qint32 _key)
{
	return m_keyStringPool.strings[_key];
}
static float complexToFloat(quint32 _v)
{
	return ((int)(_v & (0xffffff << 8))) * RADIX_MULTS[(_v >> 4) & 3];
}
QString QResArscParser::getStyleString(qint32 _strIndex)
{
	//安卓编译器会把所有带格式的字符串都放在前面，所以styles的数量会明显比strings少
	//如果要加一个带格式的字符串，则需要把所有普通字符串都后移，然后把所有引用的地方都修改
	if (_strIndex >= m_StringPool.strings.size())
		return QString();
	QString t_str = m_StringPool.strings[_strIndex];
	if (_strIndex < m_StringPool.styles.size())
		encodeRichText(t_str, m_StringPool.styles[_strIndex], m_StringPool);

	return t_str.replace(QChar(0x0A), QString("\\n"));
}
QString QResArscParser::resValue2String(const Res_value& _value)
{
	QString t_svalue;
	switch (_value.dataType)
	{
	case Res_value::_DataType::TYPE_NULL:
		t_svalue = "NULL";
		break;
	case Res_value::_DataType::TYPE_DYNAMIC_REFERENCE:
	case Res_value::_DataType::TYPE_REFERENCE:			//7f??????格式的在m_tableType里找对应的字符串，01??????是系统资源(在public-final.xml中有定义)
		t_svalue = QString("@0x%1").arg(_value.data, 8, 16, QChar('0'));
		break;
	case Res_value::_DataType::TYPE_DYNAMIC_ATTRIBUTE:
	case Res_value::_DataType::TYPE_ATTRIBUTE:			//7f??????格式的在m_tableType里找对应的字符串，01??????是系统资源(在public-final.xml中有定义)
		t_svalue = QString("?0x%1").arg(_value.data, 0, 16);
		break;
	case Res_value::_DataType::TYPE_STRING:
		t_svalue = getStyleString(_value.data);
		break;
	case Res_value::_DataType::TYPE_FLOAT:
		t_svalue = QString::number(*reinterpret_cast<const float*>(&_value.data));
		break;
	case Res_value::_DataType::TYPE_DIMENSION:
		t_svalue = QString::number(complexToFloat(_value.data)) + DIMENSION_UNIT_STRS[_value.data & 0x7];
		break;
	case Res_value::_DataType::TYPE_FRACTION:
		t_svalue = QString::number(complexToFloat(_value.data) * 100) + FRACTION_UNIT_STRS[_value.data & 0x1];
		break;
	case Res_value::_DataType::TYPE_FIRST_INT:
		t_svalue = QString::number(_value.data);
		break;
	case Res_value::_DataType::TYPE_INT_HEX:
		t_svalue = QString("0x%1").arg((uint)_value.data, 0, 16);
		break;
	case Res_value::_DataType::TYPE_INT_COLOR_ARGB8:
	case Res_value::_DataType::TYPE_INT_COLOR_RGB8:
	case Res_value::_DataType::TYPE_INT_COLOR_ARGB4:
	case Res_value::_DataType::TYPE_INT_COLOR_RGB4:
		t_svalue = QString("#0x%1").arg((uint)_value.data, 0, 16);
		break;
	case Res_value::_DataType::TYPE_INT_BOOLEAN:
		t_svalue = (_value.data != 0) ? "true" : "false";
		break;
	default:
		Q_ASSERT(false);
		break;
	}
	return t_svalue;
}
QString QResArscParser::getReferenceDestination(const ResTable_config& _config, qint32 _data)
{
	if ((_data >> 24) == 1 && (_data & 0xFF0000) != 0)
		return m_publicFinal->getDataName(_data);
	else if ((_data >> 24) == 1 && (_data & 0xFF0000) == 0)
		return QString("0x%1").arg(_data, 8, 16, QChar('0'));
	else
	{
		uint32_t t_typeID = (_data & 0xFF0000) >> 16;
		uint32_t t_ID = _data & 0xFFFF;
		Q_ASSERT(m_tableTypeDatas.contains(t_typeID));
		TTableTypeData& t_TypeData = m_tableTypeDatas[t_typeID];
		//尽量找到和当前config相等的那一组数据
		uint32_t t_specid = 0;
		for (int i = 0; i < t_TypeData.typeDatas.size(); ++i)
		{
			if (t_TypeData.typeDatas[i].config == _config)
			{
				t_specid = i;
				break;
			}
		}
		//config相等的那一组数据里没有目标指向，那只能回归默认的那组
		TTableTypeEx& t_tableType = t_TypeData.typeDatas[t_specid];
		Q_ASSERT(t_ID < (uint32_t)t_tableType.entryValue.size());
		if (t_tableType.entryValue[t_ID].isNull())
		{
			t_specid = 0;
			t_tableType = t_TypeData.typeDatas[t_specid];
		}
		QSharedPointer<ResTable_entry> t_tableEntry = t_tableType.entryValue[t_ID];
		//默认的那组也没有数据，这种情况是存在的，某些数据只存在特定的组，但是这种数据似乎不太会成为其他不同config组的引用
		if (t_tableEntry.isNull())
			return "not find!";
		if ((*t_tableEntry).size == sizeof(ResTable_entry))
		{
			TTableValueEntry* t_pValueEntry = reinterpret_cast<TTableValueEntry*>(t_tableEntry.get());
			//这个引用指向的还是引用，递归继续找
			if (t_pValueEntry->value.dataType == Res_value::_DataType::TYPE_DYNAMIC_REFERENCE || t_pValueEntry->value.dataType == Res_value::_DataType::TYPE_REFERENCE)
				return getReferenceDestination(_config, t_pValueEntry->value.data);
			else
				return resValue2String(t_pValueEntry->value);
		}
		else
		{
			TTableMapEntryEx* t_pMapValue = reinterpret_cast<TTableMapEntryEx*>(t_tableEntry.get());
			return keyString(t_pMapValue->key.index);
		}
	}
}


void QResArscParser::readTableTypeSpec(TTableTypeSpecEx& _span, const char* _pBuff)
{
	_span = *reinterpret_cast<const ResTable_typeSpec*>(_pBuff);
	_pBuff += sizeof(ResTable_typeSpec);
	_span.configmask.resize(_span.entryCount);
	memcpy(_span.configmask.data(), _pBuff, _span.entryCount * sizeof(uint));
}
void QResArscParser::writeTableTypeSpec(QByteArray& _buff, const TTableTypeSpecEx& _span)
{
	qint32 t_tableTypeSpec_pos = _buff.size();
	_buff.append(reinterpret_cast<const char*>(&_span), sizeof(ResTable_typeSpec));
	_buff.append(reinterpret_cast<const char*>(_span.configmask.data()), _span.configmask.size() * sizeof(quint32));
	reinterpret_cast<ResTable_typeSpec*>(_buff.data() + t_tableTypeSpec_pos)->header.size = _buff.size() - t_tableTypeSpec_pos;
}
TTableMapEntryEx* QResArscParser::readTableMapEntry(TTableMapEntryEx& _map, const char* _pBuff)
{
	_map = *(reinterpret_cast<const ResTable_map_entry*>(_pBuff));
	Q_ASSERT(_map.size == sizeof(ResTable_map_entry));
	//QString t_skey = m_keyStringPool.strings[_map.key.index];
	_pBuff += sizeof(ResTable_map_entry);
	for (uint i = 0; i < _map.count; ++i)
	{
		_map.tablemap.append(*reinterpret_cast<const ResTable_map*>(_pBuff));
		//字符串转换
		//t_tableMap.name.indent 大于等于0x01010000从public - final中找，否则就是简单ID
		//QString t_svalue = resValue2String(t_tableMap.value);
		_pBuff += sizeof(ResTable_map);
	}
	return &_map;
}
void QResArscParser::writeTableMapEntry(QByteArray& _buff, const TTableMapEntryEx* _mapEntry)
{
	_buff.append(reinterpret_cast<const char*>(_mapEntry), sizeof(ResTable_map_entry));
	Q_ASSERT(_mapEntry->count == _mapEntry->tablemap.size());
	for (int i = 0; i < _mapEntry->tablemap.size(); ++i)
	{
		const ResTable_map& t_tableMap = _mapEntry->tablemap[i];
		_buff.append(reinterpret_cast<const char*>(&t_tableMap), sizeof(ResTable_map));
	}
}
void QResArscParser::readTableType(TTableTypeEx& _type, const char* _pBegin)
{
	const char* t_pBuff = _pBegin;
	_type = *reinterpret_cast<const ResTable_type*>(t_pBuff);
	Q_ASSERT(_type.header.headerSize == sizeof(ResTable_type));
	Q_ASSERT(_type.config.size == sizeof(ResTable_config));
	t_pBuff += sizeof(ResTable_type);

	QVector<uint> t_entryOffsets(_type.entryCount);
	memcpy(t_entryOffsets.data(), t_pBuff, _type.entryCount * sizeof(uint));
	t_pBuff += _type.entryCount * sizeof(uint);
	t_pBuff = _pBegin + _type.entriesStart;
	_type.entryValue.resize(_type.entryCount);
	for (uint i = 0; i < _type.entryCount; ++i)
	{
		if (t_entryOffsets[i] == 0xFFFFFFFF)
			continue;
		if (*reinterpret_cast<const quint16*>(t_pBuff + t_entryOffsets[i]) == sizeof(ResTable_entry))
		{
			TTableValueEntry* t_EntryValue = _type.createEntry<TTableValueEntry>(i);
			*t_EntryValue = *reinterpret_cast<const TTableValueEntry*>(t_pBuff + t_entryOffsets[i]);
			/*字符串转换
			QString t_skey = m_keyStringPool.strings[t_EntryValue->key.index];
			QString t_svalue = resValue2String(t_EntryValue->value)*/;
		}
		else
		{
			TTableMapEntryEx* t_map = readTableMapEntry(*(_type.createEntry<TTableMapEntryEx>(i)), t_pBuff + t_entryOffsets[i]);
		}
	}
}
void QResArscParser::writeTableType(QByteArray& _buff, const TTableTypeEx& _type)
{
	qint32 t_tableType_pos = _buff.size();
	_buff.append(reinterpret_cast<const char*>(&_type), sizeof(ResTable_type));
	qint32 t_entryOffsets_pos = _buff.size();
	QVector<quint32> t_entryOffsets(_type.entryCount);
	_buff.append(reinterpret_cast<const char*>(t_entryOffsets.data()), _type.entryCount * sizeof(uint));
	reinterpret_cast<ResTable_type*>(_buff.data() + t_tableType_pos)->entriesStart = _buff.size() - t_tableType_pos;
	qint32 t_entryValue_pos = _buff.size();

	for (int i = 0; i < _type.entryValue.size(); ++i)
	{
		QSharedPointer<ResTable_entry> t_ptrEntry = _type.entryValue[i];
		qint32 t_off = _buff.size() - t_entryValue_pos;
		reinterpret_cast<quint32*>(_buff.data() + t_entryOffsets_pos)[i] = _type.entryValue[i].isNull() ? 0xFFFFFFFF : _buff.size() - t_entryValue_pos;
		if (_type.entryValue[i].isNull())
			continue;

		if ((*t_ptrEntry).size == sizeof(ResTable_entry))
		{
			TTableValueEntry* t_pValueEntry = reinterpret_cast<TTableValueEntry*>(t_ptrEntry.get());
			_buff.append(reinterpret_cast<const char*>(t_pValueEntry), sizeof(TTableValueEntry));
		}
		else
		{
			TTableMapEntryEx* t_pMapValue = reinterpret_cast<TTableMapEntryEx*>(t_ptrEntry.get());
			writeTableMapEntry(_buff, t_pMapValue);
		}
	}
	reinterpret_cast<ResTable_type*>(_buff.data() + t_tableType_pos)->header.size = _buff.size() - t_tableType_pos;
}
uint QResArscParser::readTablePackage(const char* _pBegin)
{
	m_tablePackage = *reinterpret_cast<const ResTable_package*>(_pBegin);
	Q_ASSERT(m_tablePackage.header.headerSize == sizeof(ResTable_package));
	Q_ASSERT(m_tablePackage.typeStrings == sizeof(ResTable_package));
	Q_ASSERT(m_tablePackage.typeIdOffset == 0);

	const char* t_pBuff = _pBegin + m_tablePackage.header.headerSize;
	while (t_pBuff < _pBegin + m_tablePackage.header.size)
	{
		ResChunk_header t_header = *reinterpret_cast<const ResChunk_header*>(t_pBuff);
		switch (t_header.type)
		{
		case RES_TYPE::RES_STRING_POOL_TYPE:
			if (t_pBuff == _pBegin + m_tablePackage.keyStrings)
				m_keyStringPool.readStrPool(t_pBuff);
			else if (t_pBuff == _pBegin + m_tablePackage.typeStrings)
				m_typeStringPool.readStrPool(t_pBuff);
			else
				Q_ASSERT(false);
			break;
		case RES_TYPE::RES_TABLE_TYPE_SPEC_TYPE:
			{
				qint8 t_id = (*reinterpret_cast<const ResTable_typeSpec*>(t_pBuff)).id;//id代表什么资源，是string还array等
				Q_ASSERT(!m_tableTypeDatas.contains(t_id));		//资源类不可重复
				readTableTypeSpec(m_tableTypeDatas[t_id].typeSpec, t_pBuff);
			}
			break;
		case RES_TYPE::RES_TABLE_TYPE_TYPE:
			{
				qint8 t_id = (*reinterpret_cast<const ResTable_type*>(t_pBuff)).id;		//id代表什么资源，是string还array等
				Q_ASSERT(m_tableTypeDatas.contains(t_id));		//在有具体数据前，肯定已经有了typeSpec
				QVector<TTableTypeEx>& t_typeDatas = m_tableTypeDatas[t_id].typeDatas;
				t_typeDatas.push_back(TTableTypeEx());
				TTableTypeEx& t_typeData = t_typeDatas.last();
				readTableType(t_typeData, t_pBuff);
				Q_ASSERT(t_typeData.entryValue.size() == m_tableTypeDatas[t_id].typeSpec.entryCount);
			}
			break;
		default:
			Q_ASSERT(false);
			break;
		}
		t_pBuff += t_header.size;
	}
	return m_tablePackage.header.size;
}
void QResArscParser::writeTablePackage(QByteArray& _buff)
{
	qint32 t_tablePackageHeader_pos = _buff.size();
	_buff.append(reinterpret_cast<const char*>(&m_tablePackage), sizeof(m_tablePackage));
	reinterpret_cast<ResTable_package*>(_buff.data() + t_tablePackageHeader_pos)->typeStrings = _buff.size() - t_tablePackageHeader_pos;
	m_typeStringPool.writeStrPool(_buff);
	reinterpret_cast<ResTable_package*>(_buff.data() + t_tablePackageHeader_pos)->keyStrings = _buff.size() - t_tablePackageHeader_pos;
	m_keyStringPool.writeStrPool(_buff);
	for (QMap<uint, TTableTypeData>::iterator i = m_tableTypeDatas.begin(); i != m_tableTypeDatas.end(); ++i)
	{
		TTableTypeData& t_tableTypeData = i.value();
		t_tableTypeData.typeSpec.res1 = t_tableTypeData.typeDatas.size();
		writeTableTypeSpec(_buff, t_tableTypeData.typeSpec);
		for (int j = 0; j < t_tableTypeData.typeDatas.size(); ++j)
		{
			writeTableType(_buff, t_tableTypeData.typeDatas[j]);
		}
	}
	reinterpret_cast<ResTable_package*>(_buff.data() + t_tablePackageHeader_pos)->header.size = _buff.size() - t_tablePackageHeader_pos;
}
static void traverseTableType(const TTableTypeEx& _typeData, TRAVERSE_STRIDX_CALLBACK& _callBack)
{
	for (int i = 0; i < _typeData.entryValue.size(); ++i)
	{
		QSharedPointer<ResTable_entry> t_ptrEntry = _typeData.entryValue[i];
		if (t_ptrEntry.isNull())
			continue;
		if ((*t_ptrEntry).size == sizeof(ResTable_entry))
		{
			TTableValueEntry* t_pValueEntry = reinterpret_cast<TTableValueEntry*>(t_ptrEntry.get());
			_callBack(t_pValueEntry->value.dataType, t_pValueEntry->value.data);
		}
		else
		{
			TTableMapEntryEx* t_pMapValue = reinterpret_cast<TTableMapEntryEx*>(t_ptrEntry.get());
			for (quint32 j = 0; j < t_pMapValue->count; ++j)
			{
				ResTable_map& t_tableMap = t_pMapValue->tablemap[j];
				_callBack(t_tableMap.value.dataType, t_tableMap.value.data);
			}
		}
	}
}
void QResArscParser::traversalAllValue(TRAVERSE_STRIDX_CALLBACK _callBack)
{
	for (QMap<uint, TTableTypeData>::iterator i = m_tableTypeDatas.begin(); i != m_tableTypeDatas.end(); ++i)
	{
		QVector<TTableTypeEx>& t_typeDatas = i.value().typeDatas;
		for (int j = 0; j < t_typeDatas.size(); ++j)
		{
			TTableTypeEx& t_typeData = t_typeDatas[j];
			traverseTableType(t_typeData, _callBack);
		}
	}
	for (int i = 0; i < m_StringPool.styles.size(); ++i)
	{
		TStringPoolSpans& t_spans = m_StringPool.styles[i];
		for (int j = 0; j < t_spans.spans.size(); ++j)
			_callBack(Res_value::_DataType::TYPE_STRING, t_spans.spans[j].name.index);
	}
	for (QMap<TRichString, uint32_t>::iterator i = m_StringPool.strIndexs.begin(); i != m_StringPool.strIndexs.end(); ++i)
	{
		//正常不应该这么搞，但是下面的修改并不会影响key的大小比较，所以应该没问题。
		TRichString& t_richString = const_cast<TRichString&>(i.key());
		for (int j = 0; j < t_richString.span.spans.size(); ++j)
		{
			_callBack(Res_value::_DataType::TYPE_STRING, t_richString.span.spans[j].name.index);
		}
	}
}
void QResArscParser::addLocale(uint32_t _typeid, const ResTable_config& _config)
{
	Q_ASSERT(m_tableTypeDatas.contains(_typeid));
	TTableTypeData& t_tableTypeData = m_tableTypeDatas[_typeid];
	for (int i = 0; i < t_tableTypeData.typeDatas.size(); ++i)
	{
		if (t_tableTypeData.typeDatas[i].config == _config)
			return;
	}

	TTableTypeEx t_newTableType;
	//复制头部，子项先全部填空，修改config字段
#pragma warning(push)
#pragma warning(disable: 26437)
	* reinterpret_cast<ResTable_type*>(&t_newTableType) = *reinterpret_cast<ResTable_type*>(&t_tableTypeData.typeDatas[0]);
#pragma warning(pop)
	t_newTableType.entryValue.resize(t_tableTypeData.typeSpec.entryCount);
	t_newTableType.config = _config;
	//添加到数组里去
	t_tableTypeData.typeSpec.res1++;
	t_tableTypeData.typeDatas.append(t_newTableType);

	uint32_t t_specid = t_tableTypeData.typeDatas.size() - 1;
	uint32_t t_configMask = getTableConfigMask(_config);
	QVector<uint32_t>& t_configMasks = t_tableTypeData.typeSpec.configmask;
	const TTableTypeEx& t_defaultType = t_tableTypeData.typeDatas[0];

	for (int i = 0; i < t_defaultType.entryValue.size(); ++i)
	{
		//这个字段在当前配置（比如语言本地化，比如分辨率），是不需要处理的
		if ((t_configMasks[i] & t_configMask) != t_configMask)
			continue;

		QSharedPointer<ResTable_entry> t_ptrDefaultEntry = t_defaultType.entryValue[i];
		if (t_ptrDefaultEntry.isNull())
			continue;
		copyValue(_typeid, t_specid, i);
	}
}
void QResArscParser::copyValue(uint32_t _typeid, uint32_t _specid, uint32_t _id)
{
	Q_ASSERT(m_tableTypeDatas.contains(_typeid));
	QVector<TTableTypeEx>& t_tableType = m_tableTypeDatas[_typeid].typeDatas;
	Q_ASSERT(_specid < (quint32)t_tableType.size());
	const TTableTypeEx& t_defaultType = t_tableType[0];
	TTableTypeEx& t_type = t_tableType[_specid];

	const QSharedPointer<ResTable_entry>& t_ptrDefaultEntry = t_defaultType.entryValue[_id];
	QSharedPointer<ResTable_entry> t_ptrEntry = t_type.entryValue[_id];
	if (t_ptrDefaultEntry.isNull())
		return;
	Q_ASSERT(t_ptrEntry.isNull());

	if ((*t_ptrDefaultEntry).size == sizeof(ResTable_entry))
	{
		TTableValueEntry* t_EntryValue = t_type.createEntry<TTableValueEntry>(_id);
		*t_EntryValue = *reinterpret_cast<const TTableValueEntry*>(t_ptrDefaultEntry.get());
		if (t_EntryValue->value.dataType == Res_value::_DataType::TYPE_STRING)
		{
			m_StringPool.referenceCount[t_EntryValue->value.data]++;
			qDebug() << "number of citations ++:" + m_StringPool.strings[t_EntryValue->value.data];
		}
	}
	else
	{
		TTableMapEntryEx* t_EntryValue = t_type.createEntry<TTableMapEntryEx>(_id);
		*t_EntryValue = *reinterpret_cast<const TTableMapEntryEx*>(t_ptrDefaultEntry.get());
		for (int i = 0; i < t_EntryValue->tablemap.size(); ++i)
		{
			if (t_EntryValue->tablemap[i].value.dataType == Res_value::_DataType::TYPE_STRING)
			{
				m_StringPool.referenceCount[t_EntryValue->tablemap[i].value.data]++;
				qDebug() << "number of citations ++:" + m_StringPool.strings[t_EntryValue->tablemap[i].value.data];
			}
		}
	}
}
TTableTypeEx& QResArscParser::getTableType(uint32_t _typeid, uint32_t _specid)
{
	Q_ASSERT(m_tableTypeDatas.contains(_typeid));
	QVector<TTableTypeEx>& t_tableType = m_tableTypeDatas[_typeid].typeDatas;
	Q_ASSERT(_specid < (quint32)t_tableType.size());
	return t_tableType[_specid];
}
QSharedPointer<ResTable_entry>& QResArscParser::getValue(uint32_t _typeid, uint32_t _specid, uint32_t _id)
{
	TTableTypeEx& t_type = getTableType(_typeid, _specid);
	Q_ASSERT(_id < (quint32)t_type.entryValue.size());
	return t_type.entryValue[_id];
}
QSharedPointer<ResTable_entry>& QResArscParser::getValueOrInsert(uint32_t _typeid, uint32_t _specid, uint32_t _id)
{
	QSharedPointer<ResTable_entry>& t_ptrEntry = getValue(_typeid, _specid, _id);
	if (t_ptrEntry.isNull())
	{
		copyValue(_typeid, _specid, _id);
		t_ptrEntry = getValue(_typeid, _specid, _id);
	}
	return t_ptrEntry;
}
void QResArscParser::deleteValue(uint32_t _typeid, uint32_t _specid, uint32_t _id)
{
	QSharedPointer<ResTable_entry>& t_ptrEntry = getValue(_typeid, _specid, _id);
	Q_ASSERT(!t_ptrEntry.isNull());
	if ((*t_ptrEntry).size == sizeof(ResTable_entry))
	{
		TTableValueEntry* t_EntryValue = reinterpret_cast<TTableValueEntry*>(t_ptrEntry.get());
		if (t_EntryValue->value.dataType == Res_value::_DataType::TYPE_STRING)
			m_StringPool.deleteString(t_EntryValue->value.data, false, std::bind(&QResArscParser::traversalAllValue, this, std::placeholders::_1));
	}
	else
	{
		TTableMapEntryEx* t_EntryValue = reinterpret_cast<TTableMapEntryEx*>(t_ptrEntry.get());
		for (int i = 0; i < t_EntryValue->tablemap.size(); ++i)
		{
			if (t_EntryValue->tablemap[i].value.dataType == Res_value::_DataType::TYPE_STRING)
				m_StringPool.deleteString(t_EntryValue->tablemap[i].value.data, false, std::bind(&QResArscParser::traversalAllValue, this, std::placeholders::_1));
		}
	}
	t_ptrEntry = QSharedPointer<ResTable_entry>();
}
void QResArscParser::setValue(uint32_t _typeid, uint32_t _specid, uint32_t _id, Res_value::_DataType _dataYype, uint32_t _data)
{
	QSharedPointer<ResTable_entry>& t_ptrEntry = getValueOrInsert(_typeid, _specid, _id);
	Q_ASSERT(!t_ptrEntry.isNull());
	Q_ASSERT((*t_ptrEntry).size == sizeof(ResTable_entry));
	TTableValueEntry* t_EntryValue = reinterpret_cast<TTableValueEntry*>(t_ptrEntry.get());
	if (t_EntryValue->value.dataType == Res_value::_DataType::TYPE_STRING)
		m_StringPool.deleteString(t_EntryValue->value.data, false, std::bind(&QResArscParser::traversalAllValue, this, std::placeholders::_1));

	t_EntryValue->value.dataType = _dataYype;
	t_EntryValue->value.data = _data;
}
void QResArscParser::setValue(uint32_t _typeid, uint32_t _specid, uint32_t _id, uint32_t _idx, Res_value::_DataType _dataYype, uint32_t _data)
{
	QSharedPointer<ResTable_entry>& t_ptrEntry = getValueOrInsert(_typeid, _specid, _id);
	Q_ASSERT(!t_ptrEntry.isNull());
	Q_ASSERT((*t_ptrEntry).size != sizeof(ResTable_entry));
	TTableMapEntryEx* t_pMapValue = reinterpret_cast<TTableMapEntryEx*>(t_ptrEntry.get());
	Q_ASSERT(_idx < (uint32_t)t_pMapValue->tablemap.size());
	ResTable_map& t_tableMap = t_pMapValue->tablemap[_idx];
	if (t_tableMap.value.dataType == Res_value::_DataType::TYPE_STRING)
		m_StringPool.deleteString(t_tableMap.value.data, false, std::bind(&QResArscParser::traversalAllValue, this, std::placeholders::_1));
	t_tableMap.value.dataType = _dataYype;
	t_tableMap.value.data = _data;
}
void QResArscParser::setValue(uint32_t _typeid, uint32_t _specid, uint32_t _id, const QString& _data, bool _force)
{
	QString t_str = _data;
	t_str.replace(QString("\\n"), QChar(0x0A));

	QSharedPointer<ResTable_entry>& t_ptrEntry = getValueOrInsert(_typeid, _specid, _id);
	Q_ASSERT(!t_ptrEntry.isNull());
	Q_ASSERT((*t_ptrEntry).size == sizeof(ResTable_entry));
	TTableValueEntry* t_EntryValue = reinterpret_cast<TTableValueEntry*>(t_ptrEntry.get());
	if (t_EntryValue->value.dataType == Res_value::_DataType::TYPE_STRING)
		t_EntryValue->value.data = m_StringPool.replaceString(t_EntryValue->value.data, t_str, _force, std::bind(&QResArscParser::traversalAllValue, this, std::placeholders::_1));
	else
		t_EntryValue->value.data = m_StringPool.insertString(t_str, 1, NULL, std::bind(&QResArscParser::traversalAllValue, this, std::placeholders::_1));
}
void QResArscParser::setValue(uint32_t _typeid, uint32_t _specid, uint32_t _id, uint32_t _idx, const QString& _data, bool _force)
{
	QString t_str = _data;
	t_str.replace(QString("\\n"), QChar(0x0A));

	QSharedPointer<ResTable_entry>& t_ptrEntry = getValueOrInsert(_typeid, _specid, _id);
	Q_ASSERT(!t_ptrEntry.isNull());
	Q_ASSERT((*t_ptrEntry).size != sizeof(ResTable_entry));
	TTableMapEntryEx* t_pMapValue = reinterpret_cast<TTableMapEntryEx*>(t_ptrEntry.get());
	Q_ASSERT(_idx < (uint32_t)t_pMapValue->tablemap.size());
	ResTable_map& t_tableMap = t_pMapValue->tablemap[_idx];
	if (t_tableMap.value.dataType == Res_value::_DataType::TYPE_STRING)
		t_tableMap.value.data = m_StringPool.replaceString(t_tableMap.value.data, t_str, _force, std::bind(&QResArscParser::traversalAllValue, this, std::placeholders::_1));
	else
		t_tableMap.value.data = m_StringPool.insertString(t_str, 1, NULL, std::bind(&QResArscParser::traversalAllValue, this, std::placeholders::_1));
}
quint32 QResArscParser::getReferenceCount(quint32 _index)
{
	Q_ASSERT(_index < (quint32)m_StringPool.referenceCount.size());
	return m_StringPool.referenceCount[_index];
}
