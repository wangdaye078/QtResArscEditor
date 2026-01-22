#include "QAndroidAttribute.h"
#include "QAndroidParser.h"
#include "QTablePackage.h"
#include "QTableType.h"
#include "SimpleRichText.h"
#include <QDebug>

static float complexToFloat(quint32 _v)
{
	return ((int)(_v & (0xffffff << 8))) * RADIX_MULTS[(_v >> 4) & 3];
}

QRegularExpression g_DFRegExp("^([\\-0-9\\.]+)([%A-Za-z]*)$");
float G_MULT[] = { 1.0, 128.0, 256, 256 };
static uint32_t complexToUint2(float _value)
{
	float t_value = _value / MANTISSA_MULT;
	int t_multIdx = 0;
	//最后2位是0，就不用再乘了，大于 float(0xFFFFFF)再乘就溢出了，没有数可乘了。
	while ((int(t_value) & 0xFF) != 0 && t_value <= float(0xFFFFFF) && t_multIdx < _countof(G_MULT) - 1)
	{
		t_value *= G_MULT[++t_multIdx];
	}
	return (uint32_t(t_value) & 0xFFFFFF00) | (t_multIdx << 4);
}
uint32_t getDimensionFractionData(float _v, float _divisor, const QString& _e, const char* _suffix[], int _suffixCount)
{
	uint32_t t_value = complexToUint2(_v / _divisor);
	for (int i = 0; i < _suffixCount; ++i)
	{
		if (_e == QString(_suffix[i]))
		{
			t_value |= i;
			return t_value;
		}
	}
	return 0;
}

QString resValue2String(const QString& _name, const Res_value& _value, const PArscRichString& _svalue)
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
		t_svalue = encodeRichText(_svalue.get()).replace(QChar(0x0A), QString("\\n"));
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
	case Res_value::_DataType::TYPE_INT_DEC:
		t_svalue = g_androidAttribute->value2string(_name, _value.data, false);// QString::number(_value.data);
		break;
	case Res_value::_DataType::TYPE_INT_HEX:
		t_svalue = g_androidAttribute->value2string(_name, _value.data, true);//QString("0x%1").arg((uint)_value.data, 0, 16);
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
QRegularExpression g_reFloat("^[-+]?\\d*\\.?\\d+$");
QRegularExpression g_reInt("^[-+]?\\d+$");

Res_value string2ResValue(const QString& _str, bool _forceToString)
{
	//未完成
	Res_value t_value;
	t_value.data = 0;
	if (_forceToString)
	{
		t_value.dataType = Res_value::_DataType::TYPE_STRING;
	}
	else if (_str == "NULL")
	{
		t_value.dataType = Res_value::_DataType::TYPE_NULL;
	}
	else if (_str.startsWith("@0x"))
	{
		t_value.dataType = Res_value::_DataType::TYPE_REFERENCE;
		t_value.data = _str.mid(1).toUInt(nullptr, 16);
	}
	else if (_str.startsWith("?0x"))
	{
		t_value.dataType = Res_value::_DataType::TYPE_ATTRIBUTE;
		t_value.data = _str.mid(1).toUInt(nullptr, 16);
	}
	else if (_str.startsWith("#0x"))
	{
		t_value.dataType = Res_value::_DataType::TYPE_INT_COLOR_ARGB8;
		t_value.data = _str.mid(1).toUInt(nullptr, 16);
	}
	else if (_str.startsWith("0x"))
	{
		t_value.dataType = Res_value::_DataType::TYPE_INT_HEX;
		t_value.data = _str.toUInt(nullptr, 16);
	}
	else if (_str == QString("true"))
	{
		t_value.dataType = Res_value::_DataType::TYPE_INT_BOOLEAN;
		t_value.data = 1;
	}
	else if (_str == QString("false"))
	{
		t_value.dataType = Res_value::_DataType::TYPE_INT_BOOLEAN;
	}
	else if (_str.endsWith("px") || _str.endsWith("dip") || _str.endsWith("sp") || _str.endsWith("pt") || _str.endsWith("in") || _str.endsWith("mm"))
	{
		t_value.dataType = Res_value::_DataType::TYPE_DIMENSION;
		QRegularExpressionMatch t_match = g_DFRegExp.match(_str);
		if (t_match.hasMatch())
			t_value.data = getDimensionFractionData(t_match.captured(1).toFloat(), 1, t_match.captured(2), DIMENSION_UNIT_STRS, 6);
	}
	else if (_str.endsWith("%") || _str.endsWith("%p"))
	{
		t_value.dataType = Res_value::_DataType::TYPE_FRACTION;
		QRegularExpressionMatch t_match = g_DFRegExp.match(_str);
		if (t_match.hasMatch())
			t_value.data = getDimensionFractionData(t_match.captured(1).toFloat(), 100, t_match.captured(2), FRACTION_UNIT_STRS, 2);
	}
	else if (g_reInt.match(_str).hasMatch())
	{
		t_value.dataType = Res_value::_DataType::TYPE_INT_DEC;
		t_value.data = _str.toInt();
	}
	else if (g_reFloat.match(_str).hasMatch())
	{
		t_value.dataType = Res_value::_DataType::TYPE_FLOAT;
		*reinterpret_cast<float*>(&t_value.data) = _str.toFloat();
	}
	else
	{
		t_value.dataType = Res_value::_DataType::TYPE_STRING;
	}
	return t_value;
}
/*
* 对于Res_value::_DataType::TYPE_STRING，value.data是字符串池的索引值，由于字符串池的guid从0开始，可以替代索引，
* 问题是如果修改了字符串，那字符串的排序和guid就可能对不上了，而回写的时候会所有的value.data都会使用索引，如果
* 修改原来的值，会导致数据无法对应，所以回写的时候必须弄个新value，而老的value.data则不变
*/
uint32_t TTableValueEntry::readBuff(const char* _buff)
{
	entry = *reinterpret_cast<const ResTable_entry*>(_buff);
	_buff += entry.size;
	value = *reinterpret_cast<const Res_value*>(_buff);

	if (value.dataType == Res_value::_DataType::TYPE_STRING)
		svalue = m_stringPool->getGuidRef(value.data);
	return sizeof(ResTable_entry) + sizeof(Res_value);
};
void TTableValueEntry::writeBuff(const QStringPool* _keyPool, QByteArray& _buff)
{
	ResTable_entry t_newEntry = entry;
	if ((t_newEntry.key.index >> 16) == 0)
	{
		//如果m_keyStringPool内容或者排列有修改，那就需要重新计算索引，而且不能在原有entry上面修改，需要复制一个新的entry，
		uint32_t t_newIdx = _keyPool->getRefIndex(_keyPool->getGuidRef(t_newEntry.key.index));
		if (t_newIdx != t_newEntry.key.index)
			t_newEntry.key.index = t_newIdx;
	}

	_buff.append(reinterpret_cast<const char*>(&t_newEntry), sizeof(ResTable_entry));
	Res_value t_tmpv(value);
	if (t_tmpv.dataType == Res_value::_DataType::TYPE_STRING)
	{
		t_tmpv.data = m_stringPool->getRefIndex(svalue);
		Q_ASSERT(t_tmpv.data == svalue->index);
	}
	_buff.append(reinterpret_cast<const char*>(&t_tmpv), sizeof(Res_value));
};
void TTableValueEntry::setValue(const Res_value& _value)
{
	value = _value;
	if (value.dataType == Res_value::_DataType::TYPE_STRING)
		svalue = m_stringPool->getGuidRef(value.data);
	else
		svalue.reset();
}
uint32_t ResTable_pairs::readBuff(const char* _buff)
{
	key = *reinterpret_cast<const ResTable_ref*>(_buff);
	_buff += sizeof(ResTable_ref);
	value = *reinterpret_cast<const Res_value*>(_buff);

	if (value.dataType == Res_value::_DataType::TYPE_STRING)
		svalue = m_stringPool->getGuidRef(value.data);
	return sizeof(ResTable_ref) + sizeof(Res_value);
}
void ResTable_pairs::writeBuff(const QStringPool* _keyPool, QByteArray& _buff)
{
	ResTable_ref t_newKey = key;
	if ((t_newKey.ident >> 16) == 0)
	{
		uint32_t t_newIdent = _keyPool->getRefIndex(_keyPool->getGuidRef(t_newKey.ident)); ;
		if (t_newIdent != t_newKey.ident)
			t_newKey.ident = t_newIdent;
	}
	_buff.append(reinterpret_cast<const char*>(&t_newKey), sizeof(ResTable_ref));
	Res_value t_tmpv(value);
	if (t_tmpv.dataType == Res_value::_DataType::TYPE_STRING)
	{
		t_tmpv.data = m_stringPool->getRefIndex(svalue);
		Q_ASSERT(t_tmpv.data == svalue->index);
	}
	_buff.append(reinterpret_cast<const char*>(&t_tmpv), sizeof(Res_value));
}
void ResTable_pairs::setValue(const Res_value& _value)
{
	value = _value;
	if (value.dataType == Res_value::_DataType::TYPE_STRING)
		svalue = m_stringPool->getGuidRef(value.data);
	else
		svalue.reset();
}
uint32_t TTableMapEntry::readBuff(const char* _buff)
{
	const char* t_begin = _buff;
	entry = *reinterpret_cast<const ResTable_map_entry*>(t_begin);
	t_begin += entry.size;

	for (uint i = 0; i < entry.count; ++i)
	{
		ResTable_pairs* t_pair = new ResTable_pairs(m_stringPool);
		t_begin += t_pair->readBuff(t_begin);
		pairs.push_back(PResValue(t_pair));
	}
	return t_begin - _buff;
};
void TTableMapEntry::writeBuff(const QStringPool* _keyPool, QByteArray& _buff)
{
	ResTable_map_entry t_newEntry = entry;
	if ((t_newEntry.key.index >> 16) == 0)
	{
		uint32_t t_newIdx = _keyPool->getRefIndex(_keyPool->getGuidRef(t_newEntry.key.index));
		if (t_newIdx != t_newEntry.key.index)
			t_newEntry.key.index = t_newIdx;
	}
	_buff.append(reinterpret_cast<const char*>(&t_newEntry), sizeof(ResTable_map_entry));
	for (int i = 0; i < pairs.size(); ++i)
		pairs[i]->writeBuff(_keyPool, _buff);
};


//-------------------------------------------------------------------

TSpecificData::TSpecificData(QAndroidParser* _parent)
	: m_parentParser(_parent)
{

}
TSpecificData::~TSpecificData()
{

}
IResValue* readEntry(const char* _pBuff, QStringPool* _stringPool)
{
	IResValue* t_tableValue = NULL;
	uint16_t t_size = *reinterpret_cast<const uint16_t*>(_pBuff);
	if (t_size == sizeof(ResTable_entry))
	{
		t_tableValue = new TTableValueEntry(_stringPool);
		t_tableValue->readBuff(_pBuff);
	}
	else if (t_size == sizeof(ResTable_map_entry))
	{
		t_tableValue = new TTableMapEntry(_stringPool);
		t_tableValue->readBuff(_pBuff);
	}
	else
	{
		Q_ASSERT(false);
	}
	return t_tableValue;
}
void TSpecificData::readBuff(const char* _pBegin)
{
	const char* t_pBuff = _pBegin;
	m_tableType = *reinterpret_cast<const ResTable_type*>(t_pBuff);
	Q_ASSERT(m_tableType.header.headerSize == sizeof(ResTable_type));
	Q_ASSERT(m_tableType.config.size == sizeof(ResTable_config));
	t_pBuff += sizeof(ResTable_type);

	QVector<uint32_t> t_entryOffsets(m_tableType.entryCount);
	memcpy(t_entryOffsets.data(), t_pBuff, m_tableType.entryCount * sizeof(uint32_t));
	t_pBuff += m_tableType.entryCount * sizeof(uint32_t);

	Q_ASSERT(t_pBuff == _pBegin + m_tableType.entriesStart);

	m_entryValue.resize(m_tableType.entryCount);
	for (uint32_t i = 0; i < m_tableType.entryCount; ++i)
	{
		if (t_entryOffsets[i] == ResTable_type::NO_ENTRY)
			continue;
		m_entryValue[i] = PResValue(readEntry(t_pBuff + t_entryOffsets[i], m_parentParser->getStringPool()));
	}
}
void TSpecificData::writeBuff(const QStringPool* _keyPool, QByteArray& _buff)
{
	int t_tableType_pos = _buff.size();
	_buff.append(reinterpret_cast<const char*>(&m_tableType), sizeof(ResTable_type));
	int t_entryOffsets_pos = _buff.size();
	QVector<uint32_t> t_entryOffsets(m_tableType.entryCount);
	_buff.append(reinterpret_cast<const char*>(t_entryOffsets.data()), m_tableType.entryCount * sizeof(uint32_t));
	reinterpret_cast<ResTable_type*>(_buff.data() + t_tableType_pos)->entriesStart = _buff.size() - t_tableType_pos;
	int t_entryValue_pos = _buff.size();
	for (int i = 0; i < m_entryValue.size(); ++i)
	{
		PResValue t_ptrEntry = m_entryValue[i];
		reinterpret_cast<uint32_t*>(_buff.data() + t_entryOffsets_pos)[i] = m_entryValue[i].isNull() ? ResTable_type::NO_ENTRY : _buff.size() - t_entryValue_pos;
		if (m_entryValue[i].isNull())
			continue;
		t_ptrEntry->writeBuff(_keyPool, _buff);
	}
	reinterpret_cast<ResTable_type*>(_buff.data() + t_tableType_pos)->header.size = _buff.size() - t_tableType_pos;
}
const ResTable_type& TSpecificData::getType() const
{
	return m_tableType;
}
void TSpecificData::traversalData(TRAVERSE_SPECIFIC_DATA_CALLBACK _callBack) const
{
	for (uint32_t i = 0; i < (uint32_t)m_entryValue.size(); ++i)
	{
		PResValue t_ptrEntry = m_entryValue[i];
		if (t_ptrEntry.isNull())
			continue;
		EValueItemType t_ItemType = t_ptrEntry->getValueType();
		QTreeWidgetItem* t_TreeItem = _callBack(NULL, i, t_ItemType, QVariant::fromValue(t_ptrEntry));
		if (t_ItemType == eValueItemType_array)
		{
			TTableMapEntry* t_pMapEntry = reinterpret_cast<TTableMapEntry*>(t_ptrEntry.get());
			for (uint32_t j = 0; j < (uint32_t)t_pMapEntry->pairs.size(); ++j)
			{
				_callBack(t_TreeItem, j, eValueItemType_arrayitem, QVariant::fromValue(t_pMapEntry->pairs[j]));
			}
			_callBack(NULL, i, eValueItemType_arrayend, QVariant::fromValue(t_ptrEntry));
		}
	}
}
void TSpecificData::deleteValue(uint32_t _id)
{
	Q_ASSERT(_id < (uint32_t)m_entryValue.size());
	m_entryValue[_id].reset();
}
ResTable_type& TSpecificData::getTType(void)
{
	return m_tableType;
}
void TSpecificData::copyValue(const TSpecificData& _other, uint32_t _idx)
{
	Q_ASSERT(!_other.m_entryValue[_idx].isNull());
	Q_ASSERT(m_entryValue[_idx].isNull());
	m_entryValue[_idx] = PResValue(_other.m_entryValue[_idx]->clone());
}
uint32_t TSpecificData::getEntryCount(void) const
{
	return m_entryValue.size();
}
void TSpecificData::setEntryCount(uint32_t _count)
{
	m_entryValue.resize(_count);
}
PResValue TSpecificData::getEntry(uint32_t _idx)
{
	Q_ASSERT((uint32_t)m_entryValue.size() > _idx);
	return m_entryValue[_idx];
}
//-------------------------------------------------------------------
QTableType::QTableType(QAndroidParser* _parent)
	: QObject(_parent), m_parentParser(_parent), m_typeSpec()
{
}

QTableType::~QTableType()
{
}
void QTableType::readBuff_head(const char* _buff)
{
	m_typeSpec = *reinterpret_cast<const ResTable_typeSpec*>(_buff);
	_buff += sizeof(ResTable_typeSpec);
	m_configmask.resize(m_typeSpec.entryCount);
	memcpy(m_configmask.data(), _buff, m_typeSpec.entryCount * sizeof(uint32_t));
	m_id_key.clear();
}
void QTableType::readBuff_specData(const char* _buff)
{
	PSpecificData t_pSpecificData(new TSpecificData(m_parentParser));
	t_pSpecificData->readBuff(_buff);
	m_SpecDatas.push_back(t_pSpecificData);
	t_pSpecificData->traversalData(std::bind(&QTableType::onRefreshSpecificData, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}
void QTableType::writeBuff(const QStringPool* _keyPool, QByteArray& _buff)
{
	int t_tableTypeSpec_pos = _buff.size();
	_buff.append(reinterpret_cast<const char*>(&m_typeSpec), sizeof(ResTable_typeSpec));
	_buff.append(reinterpret_cast<const char*>(m_configmask.data()), m_configmask.size() * sizeof(uint32_t));
	reinterpret_cast<ResTable_typeSpec*>(_buff.data() + t_tableTypeSpec_pos)->header.size = _buff.size() - t_tableTypeSpec_pos;

	for (int i = 0; i < m_SpecDatas.size(); ++i)
		m_SpecDatas[i]->writeBuff(_keyPool, _buff);
}
void QTableType::traversalData(TRAVERSE_PACKAGE_DATA_CALLBACK _callBack, const QString& _packageName, uint32_t _typeId, const QString& _typeName) const
{
	for (int i = 0; i < m_SpecDatas.size(); ++i)
	{
		_callBack(_packageName, eTreeItemType_spec, _typeId, tableConfig2String(_typeName, m_SpecDatas[i]->getType().config), QVariant::fromValue(m_SpecDatas[i]));
	}
}
QTreeWidgetItem* QTableType::onRefreshSpecificData(QTreeWidgetItem* _parent, uint32_t _idx, EValueItemType _type, const QVariant& _v)
{
	if (_type == eValueItemType_array || _type == eValueItemType_value)
	{
		IResValue* t_pEntry = _v.value<PResValue>().get();
		m_id_key[_idx] = t_pEntry->getKeyIndex();
	}
	return NULL;
}
uint32_t QTableType::getKeyIndex(uint32_t _id)
{
	Q_ASSERT(m_id_key.contains(_id));
	return m_id_key[_id];
}
const QVector<uint32_t>& QTableType::getConfigMask(void)
{
	return m_configmask;
}
PSpecificData QTableType::getDefaultSpec(void)
{
	Q_ASSERT(m_SpecDatas.size() != 0);
	return m_SpecDatas[0];
}
PSpecificData QTableType::addLocale(const ResTable_config& _config)
{
	m_typeSpec.res1++;
	PSpecificData t_pSpecificData(new TSpecificData(m_parentParser));
	t_pSpecificData->getTType() = m_SpecDatas[0]->getTType();
	t_pSpecificData->getTType().config = _config;
	t_pSpecificData->setEntryCount(m_SpecDatas[0]->getEntryCount());
	m_SpecDatas.push_back(t_pSpecificData);
	return t_pSpecificData;
}

