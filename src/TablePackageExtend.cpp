#include "TablePackageExtend.h"
#include <QtDebug>
#include "basicDefine.h"
#include "QPublicFinal.h"


TTablePackage::TTablePackage() :
	m_publicStringPool(NULL), m_publicFinal(NULL)
{
}
void TTablePackage::reset(void)
{
	m_typeStringPool.reset();
	m_keyStringPool.reset();
	m_tableTypeDatas.clear();
}
void TTablePackage::setPublicData(TStringPool* _strStringPool, QPublicFinal* _publicFinal)
{
	m_publicStringPool = _strStringPool;
	m_publicFinal = _publicFinal;
}
void TTablePackage::readTablePackage(const char* _pBegin)
{
	m_packageInfo = *reinterpret_cast<const ResTable_package*>(_pBegin);
	Q_ASSERT(m_packageInfo.header.headerSize == sizeof(ResTable_package));
	Q_ASSERT(m_packageInfo.typeStrings == sizeof(ResTable_package));
	Q_ASSERT(m_packageInfo.typeIdOffset == 0);

	const char* t_pBuff = _pBegin + m_packageInfo.header.headerSize;
	while (t_pBuff < _pBegin + m_packageInfo.header.size)
	{
		const ResChunk_header* t_header = reinterpret_cast<const ResChunk_header*>(t_pBuff);
		switch (t_header->type)
		{
		case RES_TYPE::RES_STRING_POOL_TYPE:
			if (t_pBuff == _pBegin + m_packageInfo.keyStrings)
				m_keyStringPool.readStrPool(t_pBuff);
			else if (t_pBuff == _pBegin + m_packageInfo.typeStrings)
				m_typeStringPool.readStrPool(t_pBuff);
			else
				Q_ASSERT(false);
			break;
		case RES_TYPE::RES_TABLE_TYPE_SPEC_TYPE:
			{
				uint t_id = (*reinterpret_cast<const ResTable_typeSpec*>(t_pBuff)).id;//id代表什么资源，是string还array等
				Q_ASSERT(!m_tableTypeDatas.contains(t_id));		//资源类不可重复
				readTableTypeSpec(m_tableTypeDatas[t_id].typeSpec, t_pBuff);
			}
			break;
		case RES_TYPE::RES_TABLE_TYPE_TYPE:
			{
				uint t_id = (*reinterpret_cast<const ResTable_type*>(t_pBuff)).id;		//id代表什么资源，是string还array等
				Q_ASSERT(m_tableTypeDatas.contains(t_id));		//在有具体数据前，肯定已经有了typeSpec
				QVector<TTableTypeEx>& t_typeDatas = m_tableTypeDatas[t_id].types;  //某个类型（比如array,string,color）的数据
				t_typeDatas.push_back(TTableTypeEx());		//在这个类型下增加一个语言，或者某个分辨率的配置
				readTableType(t_typeDatas.last(), t_pBuff);
			}
			break;
		default:
			Q_ASSERT(false);
			break;
		}
		t_pBuff += t_header->size;
	}
}
void TTablePackage::writeTablePackage(QByteArray& _buff) const
{
	int t_tablePackageHeader_pos = _buff.size();
	_buff.append(reinterpret_cast<const char*>(&m_packageInfo), sizeof(m_packageInfo));
	reinterpret_cast<ResTable_package*>(_buff.data() + t_tablePackageHeader_pos)->typeStrings = _buff.size() - t_tablePackageHeader_pos;
	m_typeStringPool.writeStrPool(_buff);
	reinterpret_cast<ResTable_package*>(_buff.data() + t_tablePackageHeader_pos)->keyStrings = _buff.size() - t_tablePackageHeader_pos;
	m_keyStringPool.writeStrPool(_buff);
	for (QMap<uint, TTableTypeData>::const_iterator i = m_tableTypeDatas.begin(); i != m_tableTypeDatas.end(); ++i)
	{
		const TTableTypeData& t_tableTypeData = i.value();
		Q_ASSERT(t_tableTypeData.typeSpec.res1 == t_tableTypeData.types.size());
		writeTableTypeSpec(_buff, t_tableTypeData.typeSpec);
		for (int j = 0; j < t_tableTypeData.types.size(); ++j)
			writeTableType(_buff, t_tableTypeData.types[j]);
	}
	reinterpret_cast<ResTable_package*>(_buff.data() + t_tablePackageHeader_pos)->header.size = _buff.size() - t_tablePackageHeader_pos;
}
void TTablePackage::readTableTypeSpec(TTableTypeSpecEx& _span, const char* _pBuff)
{
	_span = *reinterpret_cast<const ResTable_typeSpec*>(_pBuff);
	_pBuff += sizeof(ResTable_typeSpec);
	_span.configmask.resize(_span.entryCount);
	memcpy(_span.configmask.data(), _pBuff, _span.entryCount * sizeof(uint));
}
void TTablePackage::readTableType(TTableTypeEx& _type, const char* _pBegin)
{
	const char* t_pBuff = _pBegin;
	_type = *reinterpret_cast<const ResTable_type*>(t_pBuff);
	Q_ASSERT(_type.header.headerSize == sizeof(ResTable_type));
	Q_ASSERT(_type.config.size == sizeof(ResTable_config));
	t_pBuff += sizeof(ResTable_type);

	QVector<uint32_t> t_entryOffsets(_type.entryCount);
	memcpy(t_entryOffsets.data(), t_pBuff, _type.entryCount * sizeof(uint32_t));
	t_pBuff += _type.entryCount * sizeof(uint32_t);

	Q_ASSERT(t_pBuff == _pBegin + _type.entriesStart);

	_type.entryValue.resize(_type.entryCount);
	for (uint32_t i = 0; i < _type.entryCount; ++i)
	{
		if (t_entryOffsets[i] == ResTable_type::NO_ENTRY)
			continue;
		if (*reinterpret_cast<const quint16*>(t_pBuff + t_entryOffsets[i]) == sizeof(ResTable_entry))
			*(_type.createEntry<TTableValueEntry>(i)) = *reinterpret_cast<const TTableValueEntry*>(t_pBuff + t_entryOffsets[i]);
		else
			readTableMapEntry(*(_type.createEntry<TTableMapEntry>(i)), t_pBuff + t_entryOffsets[i]);
	}
}
void TTablePackage::readTableMapEntry(TTableMapEntry& _map, const char* _pBuff)
{
	_map = *(reinterpret_cast<const ResTable_map_entry*>(_pBuff));
	Q_ASSERT(_map.size == sizeof(ResTable_map_entry));
	const ResTable_map* t_pTableMap = reinterpret_cast<const ResTable_map*>(_pBuff + sizeof(ResTable_map_entry));
	for (uint i = 0; i < _map.count; ++i)
		_map.tablemap.append(t_pTableMap[i]);
}

void TTablePackage::writeTableTypeSpec(QByteArray& _buff, const TTableTypeSpecEx& _span) const
{
	int t_tableTypeSpec_pos = _buff.size();
	_buff.append(reinterpret_cast<const char*>(&_span), sizeof(ResTable_typeSpec));
	_buff.append(reinterpret_cast<const char*>(_span.configmask.data()), _span.configmask.size() * sizeof(uint32_t));
	reinterpret_cast<ResTable_typeSpec*>(_buff.data() + t_tableTypeSpec_pos)->header.size = _buff.size() - t_tableTypeSpec_pos;
}
void TTablePackage::writeTableType(QByteArray& _buff, const TTableTypeEx& _type) const
{
	int t_tableType_pos = _buff.size();
	_buff.append(reinterpret_cast<const char*>(&_type), sizeof(ResTable_type));
	int t_entryOffsets_pos = _buff.size();
	QVector<uint32_t> t_entryOffsets(_type.entryCount);
	_buff.append(reinterpret_cast<const char*>(t_entryOffsets.data()), _type.entryCount * sizeof(uint32_t));
	reinterpret_cast<ResTable_type*>(_buff.data() + t_tableType_pos)->entriesStart = _buff.size() - t_tableType_pos;
	int t_entryValue_pos = _buff.size();

	for (int i = 0; i < _type.entryValue.size(); ++i)
	{
		QSharedPointer<ResTable_entry> t_ptrEntry = _type.entryValue[i];
		reinterpret_cast<uint32_t*>(_buff.data() + t_entryOffsets_pos)[i] = _type.entryValue[i].isNull() ? ResTable_type::NO_ENTRY : _buff.size() - t_entryValue_pos;
		if (_type.entryValue[i].isNull())
			continue;

		if ((*t_ptrEntry).size == sizeof(ResTable_entry))
			_buff.append(reinterpret_cast<const char*>(t_ptrEntry.get()), sizeof(TTableValueEntry));
		else
			writeTableMapEntry(_buff, reinterpret_cast<TTableMapEntry*>(t_ptrEntry.get()));
	}
	reinterpret_cast<ResTable_type*>(_buff.data() + t_tableType_pos)->header.size = _buff.size() - t_tableType_pos;
}
void TTablePackage::writeTableMapEntry(QByteArray& _buff, const TTableMapEntry* _mapEntry) const
{
	_buff.append(reinterpret_cast<const char*>(_mapEntry), sizeof(ResTable_map_entry));
	Q_ASSERT(_mapEntry->count == _mapEntry->tablemap.size());
	for (int i = 0; i < _mapEntry->tablemap.size(); ++i)
		_buff.append(reinterpret_cast<const char*>(&_mapEntry->tablemap[i]), sizeof(ResTable_map));
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
			TTableMapEntry* t_pMapValue = reinterpret_cast<TTableMapEntry*>(t_ptrEntry.get());
			for (quint32 j = 0; j < t_pMapValue->count; ++j)
			{
				ResTable_map& t_tableMap = t_pMapValue->tablemap[j];
				_callBack(t_tableMap.value.dataType, t_tableMap.value.data);
			}
		}
	}
}
void TTablePackage::traversalData(TRAVERSE_PACKAGE_DATA_CALLBACK _callBack) const
{
	QString t_packageName = WCHARToQString(m_packageInfo.name);
	for (QMap<uint, TTableTypeData>::const_iterator i = m_tableTypeDatas.begin(); i != m_tableTypeDatas.end(); ++i)
	{
		_callBack(t_packageName, eTreeItemType_type, i.key(), 0, getTypeString(i.key() - 1));
		const QVector<TTableTypeEx>& t_tableType = i.value().types;
		for (int j = 0; j < t_tableType.size(); ++j)
		{
			_callBack(t_packageName, eTreeItemType_spec, i.key(), j, tableConfig2String(getTypeString(i.key() - 1), t_tableType[j].config));
		}
	}
}
void TTablePackage::traversalAllValue(TRAVERSE_STRIDX_CALLBACK _callBack) const
{
	for (QMap<uint, TTableTypeData>::const_iterator i = m_tableTypeDatas.begin(); i != m_tableTypeDatas.end(); ++i)
	{
		const QVector<TTableTypeEx>& t_typeDatas = i.value().types;
		for (int j = 0; j < t_typeDatas.size(); ++j)
		{
			traverseTableType(t_typeDatas[j], _callBack);
		}
	}
}
const QVector<uint32_t>& TTablePackage::getTableTypeMask(uint32_t _typeid) const
{
	Q_ASSERT(m_tableTypeDatas.contains(_typeid));
	return m_tableTypeDatas.find(_typeid).value().typeSpec.configmask;
}
const TTableTypeEx& TTablePackage::getTableType(uint32_t _typeid, uint32_t _specid) const
{
	Q_ASSERT(m_tableTypeDatas.contains(_typeid));
	const QVector<TTableTypeEx>& t_tableType = m_tableTypeDatas[_typeid].types;
	Q_ASSERT(_specid < (quint32)t_tableType.size());
	return t_tableType[_specid];
}
const TTableTypeEx* TTablePackage::getTableType(uint32_t _typeid, const ResTable_config& _config) const
{
	Q_ASSERT(m_tableTypeDatas.contains(_typeid));
	const QVector<TTableTypeEx>& t_tableType = m_tableTypeDatas[_typeid].types;
	for (int i = 0; i < t_tableType.size(); ++i)
		if (t_tableType[i].config == _config)
			return &t_tableType[i];
	return NULL;
}
QString TTablePackage::getKeyString(int _index) const
{
	return  m_keyStringPool.getString(_index);
}
QString TTablePackage::getTypeString(int _index) const
{
	return  m_typeStringPool.getString(_index);
}
QString TTablePackage::getReferenceDestination(const ResTable_config& _config, qint32 _data) const
{
	if ((_data >> 24) == 1 && (_data & 0xFF0000) != 0)
		return m_publicFinal->getDataName(_data);
	else if ((_data >> 24) == 1 && (_data & 0xFF0000) == 0)
		return QString("0x%1").arg(_data, 8, 16, QChar('0'));
	else
	{
		uint32_t t_typeID = (_data & 0xFF0000) >> 16;
		uint32_t t_ID = _data & 0xFFFF;

		const TTableTypeEx* t_tableType = getTableType(t_typeID, _config);
		if (t_tableType == NULL)
			t_tableType = &getTableType(t_typeID, 0);

		Q_ASSERT(t_ID < (uint32_t)t_tableType->entryValue.size());
		if (t_tableType->entryValue[t_ID].isNull())
			t_tableType = &getTableType(t_typeID, 0);

		const QSharedPointer<ResTable_entry>& t_tableEntry = t_tableType->entryValue[t_ID];
		//默认的那组也没有数据，这种情况是存在的，某些数据只存在特定的组，但是这种数据似乎不太会成为其他不同config组的引用
		if (t_tableEntry.isNull())
			return "not find!";
		if ((*t_tableEntry).size == sizeof(ResTable_entry))
		{
			const TTableValueEntry* t_pValueEntry = reinterpret_cast<const TTableValueEntry*>(t_tableEntry.get());
			//这个引用指向的还是引用，递归继续找
			if (t_pValueEntry->value.dataType == Res_value::_DataType::TYPE_DYNAMIC_REFERENCE || t_pValueEntry->value.dataType == Res_value::_DataType::TYPE_REFERENCE)
				return getReferenceDestination(_config, t_pValueEntry->value.data);
			else
				return resValue2String(t_pValueEntry->value);
		}
		else
		{
			const TTableMapEntry* t_pMapValue = reinterpret_cast<const TTableMapEntry*>(t_tableEntry.get());
			return getKeyString(t_pMapValue->key.index);
		}
	}
}
static float complexToFloat(quint32 _v)
{
	return ((int)(_v & (0xffffff << 8))) * RADIX_MULTS[(_v >> 4) & 3];
}
QString TTablePackage::resValue2String(const Res_value& _value) const
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
		t_svalue = m_publicStringPool->getStyleString(_value.data);
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
void TTablePackage::addLocale(uint32_t _typeid, const ResTable_config& _config, TRAVERSE_PACKAGE_DATA_CALLBACK _callBack)
{
	Q_ASSERT(m_tableTypeDatas.contains(_typeid));
	TTableTypeData& t_tableTypeData = m_tableTypeDatas[_typeid];

	TTableTypeEx t_newTableType;
	//复制头部，子项先全部填空，修改config字段
#pragma warning(push)
#pragma warning(disable: 26437)
	* reinterpret_cast<ResTable_type*>(&t_newTableType) = *reinterpret_cast<const ResTable_type*>(&t_tableTypeData.types[0]);
#pragma warning(pop)
	t_newTableType.entryValue.resize(t_tableTypeData.typeSpec.entryCount);
	t_newTableType.config = _config;
	//添加到数组里去
	t_tableTypeData.typeSpec.res1++;
	t_tableTypeData.types.append(t_newTableType);

	uint32_t t_specid = t_tableTypeData.types.size() - 1;
	uint32_t t_configMask = getTableConfigMask(_config);
	QVector<uint32_t>& t_configMasks = t_tableTypeData.typeSpec.configmask;
	const TTableTypeEx& t_defaultType = t_tableTypeData.types[0];

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
	_callBack(WCHARToQString(m_packageInfo.name), eTreeItemType_spec, _typeid, t_specid, tableConfig2String(getTypeString(_typeid - 1), _config));
}
void TTablePackage::copyValue(uint32_t _typeid, uint32_t _specid, uint32_t _id)
{
	const TTableTypeEx& t_defaultType = getTableType(_typeid, 0);
	TTableTypeEx& t_type = const_cast<TTableTypeEx&>(getTableType(_typeid, _specid));

	const QSharedPointer<ResTable_entry>& t_ptrDefaultEntry = t_defaultType.entryValue[_id];
	const QSharedPointer<ResTable_entry>& t_ptrEntry = t_type.entryValue[_id];
	if (t_ptrDefaultEntry.isNull())
		return;
	Q_ASSERT(t_ptrEntry.isNull());

	if ((*t_ptrDefaultEntry).size == sizeof(ResTable_entry))
	{
		TTableValueEntry* t_EntryValue = t_type.createEntry<TTableValueEntry>(_id);
		*t_EntryValue = *reinterpret_cast<const TTableValueEntry*>(t_ptrDefaultEntry.get());
		if (t_EntryValue->value.dataType == Res_value::_DataType::TYPE_STRING)
		{
			m_publicStringPool->incReferenceCount(t_EntryValue->value.data, 1);
			qDebug() << "number of citations ++:" + m_publicStringPool->getStyleString(t_EntryValue->value.data);
		}
	}
	else
	{
		TTableMapEntry* t_EntryValue = t_type.createEntry<TTableMapEntry>(_id);
		*t_EntryValue = *reinterpret_cast<const TTableMapEntry*>(t_ptrDefaultEntry.get());
		for (int i = 0; i < t_EntryValue->tablemap.size(); ++i)
		{
			if (t_EntryValue->tablemap[i].value.dataType == Res_value::_DataType::TYPE_STRING)
			{
				m_publicStringPool->incReferenceCount(t_EntryValue->tablemap[i].value.data, 1);
				qDebug() << "number of citations ++:" + m_publicStringPool->getStyleString(t_EntryValue->tablemap[i].value.data);
			}
		}
	}
}
void TTablePackage::deleteValue(uint32_t _typeid, uint32_t _specid, uint32_t _id, TTRAVERSAL_ALL_VALUE _traversalAllValue)
{
	TTableTypeEx& t_tableType = const_cast<TTableTypeEx&>(getTableType(_typeid, _specid));
	Q_ASSERT(_id < (quint32)t_tableType.entryValue.size());
	QSharedPointer<ResTable_entry>& t_ptrEntry = t_tableType.entryValue[_id];
	Q_ASSERT(!t_ptrEntry.isNull());
	if ((*t_ptrEntry).size == sizeof(ResTable_entry))
	{
		const TTableValueEntry* t_EntryValue = reinterpret_cast<const TTableValueEntry*>(t_ptrEntry.get());
		if (t_EntryValue->value.dataType == Res_value::_DataType::TYPE_STRING)
			m_publicStringPool->deleteString(t_EntryValue->value.data, false, _traversalAllValue);
	}
	else
	{
		const TTableMapEntry* t_EntryValue = reinterpret_cast<const TTableMapEntry*>(t_ptrEntry.get());
		for (int i = 0; i < t_EntryValue->tablemap.size(); ++i)
		{
			if (t_EntryValue->tablemap[i].value.dataType == Res_value::_DataType::TYPE_STRING)
				m_publicStringPool->deleteString(t_EntryValue->tablemap[i].value.data, false, _traversalAllValue);
		}
	}
	t_ptrEntry.clear();
}
