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

	t_pBuff += parserStrPool(t_pBuff, m_StringPool);
	t_pBuff += parserTablePackage(t_pBuff);
	initStringReferenceCount();
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

	writeStrPool(_buff, m_StringPool);
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
float complexToFloat(quint32 _v)
{
	return ((int)(_v & (0xffffff << 8))) * RADIX_MULTS[(_v >> 4) & 3];
}
uint32_t complexToUint(float _v)
{
	return 0;
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
	case Res_value::_DataType::TYPE_REFERENCE:			//7f??????格式的在m_tableType里找对应的字符串，01??????是系统资源(在public-final.xml中有定义)
		t_svalue = QString("@0x%1").arg(_value.data, 8, 16, QChar('0'));
		break;
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
		t_svalue = QString::number(complexToFloat(_value.data)) + DIMENSION_UNIT_STRS[_value.data & 0xf];
		break;
	case Res_value::_DataType::TYPE_FRACTION:
		t_svalue = QString::number(complexToFloat(_value.data) * 100) + FRACTION_UNIT_STRS[_value.data & 0xf];
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
	if ((_data >> 24) == 1)
		return m_publicFinal->getDataName(_data);
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
			return "it is a array";
	}
}
quint32 writeStringLen_16(char* _pBuff, const QString& _str)
{
	quint16* t_pBuff = reinterpret_cast<quint16*>(_pBuff);
	if (_str.size() < 0x8000)
	{
		t_pBuff[0] = _str.size();
		return sizeof(quint16);
	}
	else
	{
		t_pBuff[0] = (_str.size() >> 16) | 0x8000;
		t_pBuff[1] = _str.size() & 0xffff;
		return sizeof(quint16) * 2;
	}
}
quint32 writeStringLen_8(char* _pBuff, const QString& _str, QByteArray& _utf8Array)
{
	qint8* t_pBuff = reinterpret_cast<qint8*>(_pBuff);
	quint32 t_pos = 0;
	if (_str.size() < 0x80)
		t_pBuff[t_pos++] = _str.size();
	else
	{
		t_pBuff[t_pos++] = (_str.size() >> 8) | 0x80;
		t_pBuff[t_pos++] = _str.size() & 0xff;
	}
	_utf8Array = QString_to_utf8(_str);
	if (_utf8Array.size() < 0x80)
		t_pBuff[t_pos++] = _utf8Array.size();
	else
	{
		t_pBuff[t_pos++] = (_utf8Array.size() >> 8) | 0x80;
		t_pBuff[t_pos++] = _utf8Array.size() & 0xff;
	}
	return t_pos;
}
quint32 readStringLen_16(const char*& _pBuff)
{
	//看逻辑，UTF16字符串最长可以有0x7FFF FFFF长
	quint16 t_u16len = readValue<quint16>(_pBuff);
	if ((t_u16len & 0x8000) == 0)
		return t_u16len;
	else
		return ((t_u16len & 0x7FFF) << 16) + readValue<ushort>(_pBuff);
}
uint readStringLen_8(const char*& _pBuff)
{
	//看逻辑，UTF8字符串最长可以有0x7F FF长
	uchar t_u16len = readValue<uchar>(_pBuff);
	if ((t_u16len & 0x80) != 0)
		uchar t_u16len_fix = readValue<uchar>(_pBuff);

	uchar t_u8len = readValue<uchar>(_pBuff);
	if ((t_u8len & 0x80) == 0)
		return t_u8len;
	else
		return ((t_u8len & 0x7F) << 8) + readValue<uchar>(_pBuff);
}

void readString(QString& _string, const char* _pBuff, bool _isUTF8)
{
	if (!_isUTF8)
	{
		uint t_len = readStringLen_16(_pBuff);
		_string = WCHARToQStringN(_pBuff, t_len);
		Q_ASSERT(*(reinterpret_cast<const ushort*>(_pBuff) + t_len) == 0);
	}
	else
	{
		uint t_len = readStringLen_8(_pBuff);
		_string = utf8_to_QString(_pBuff, t_len);
		QByteArray t_arr = _string.toUtf8();
		Q_ASSERT(_pBuff[t_len] == 0);
	}
}
void writeString(QByteArray& _buff, const QString& _string, bool _isUTF8)
{
	char t_len[4];
	if (!_isUTF8)
	{
		quint32 t_lenlen = writeStringLen_16(t_len, _string);
		_buff.append(t_len, t_lenlen);
		_buff.append(reinterpret_cast<const char*>(_string.utf16()), (_string.length() + 1) * sizeof(ushort));
	}
	else
	{
		QByteArray t_utf8Array;
		quint32 t_lenlen = writeStringLen_8(t_len, _string, t_utf8Array);
		_buff.append(t_len, t_lenlen);
		_buff.append(t_utf8Array.data(), t_utf8Array.size() + 1);
	}
}
void QResArscParser::readPoolSpan(TStringPoolSpans& _span, const char* _pBuff, TStringPool& _stringPool)
{
	while (*reinterpret_cast<const uint*>(_pBuff) != ResStringPool_span::END)
	{
		ResStringPool_span t_span = *reinterpret_cast<const ResStringPool_span*>(_pBuff);
		_span.spans.append(t_span);
		_pBuff += sizeof(ResStringPool_span);
	}
}
void QResArscParser::writePoolSpan(QByteArray& _buff, const TStringPoolSpans& _span)
{
	_buff.append(reinterpret_cast<const char*>(_span.spans.data()), _span.spans.size() * sizeof(ResStringPool_span));
	quint32 t_tmp = ResStringPool_span::END;
	_buff.append(reinterpret_cast<const char*>(&t_tmp), sizeof(t_tmp));
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
uint QResArscParser::parserStrPool(const char* _pBegin, TStringPool& _StringPool)
{
	_StringPool.StringPoolHeader = *reinterpret_cast<const ResStringPool_header*>(_pBegin);
	Q_ASSERT(_StringPool.StringPoolHeader.header.type == RES_TYPE::RES_STRING_POOL_TYPE);
	const char* t_pBuff = _pBegin + sizeof(_StringPool.StringPoolHeader);

	QVector<quint32> t_stringOffsets(_StringPool.StringPoolHeader.stringCount);
	memcpy(t_stringOffsets.data(), t_pBuff, _StringPool.StringPoolHeader.stringCount * sizeof(quint32));
	t_pBuff += _StringPool.StringPoolHeader.stringCount * sizeof(quint32);
	QVector<quint32> t_styleOffsets(_StringPool.StringPoolHeader.styleCount);
	memcpy(t_styleOffsets.data(), t_pBuff, _StringPool.StringPoolHeader.styleCount * sizeof(quint32));

	bool t_isUTF8 = (_StringPool.StringPoolHeader.flags & ResStringPool_header::UTF8_FLAG) != 0;

	t_pBuff = _pBegin + _StringPool.StringPoolHeader.stringsStart;
	_StringPool.strings.resize(_StringPool.StringPoolHeader.stringCount);
	for (uint i = 0; i < _StringPool.StringPoolHeader.stringCount; i++)
		readString(_StringPool.strings[i], t_pBuff + t_stringOffsets[i], t_isUTF8);

	t_pBuff = _pBegin + _StringPool.StringPoolHeader.stylesStart;
	_StringPool.styles.resize(_StringPool.StringPoolHeader.styleCount);
	for (uint i = 0; i < _StringPool.StringPoolHeader.styleCount; i++)
		readPoolSpan(_StringPool.styles[i], t_pBuff + t_styleOffsets[i], _StringPool);

	_StringPool.makeIndexs();

	return _StringPool.StringPoolHeader.header.size;
}
uint QResArscParser::parserTablePackage(const char* _pBegin)
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
				parserStrPool(t_pBuff, m_keyStringPool);
			else if (t_pBuff == _pBegin + m_tablePackage.typeStrings)
				parserStrPool(t_pBuff, m_typeStringPool);
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
void QResArscParser::writeStrPool(QByteArray& _buff, const TStringPool& _StringPool)
{
	//因为后面还要修改，_buff的内存随着变大申请新内存，可能会移动到别处，所以不能保存指针使用。只能随用随取。
	qint32 t_stringPoolHeader_pos = _buff.size();
	_buff.append(reinterpret_cast<const char*>(&_StringPool.StringPoolHeader), sizeof(_StringPool.StringPoolHeader));

	qint32 t_stringOffsets_pos = _buff.size();
	QVector<qint32> t_tmp(_StringPool.StringPoolHeader.stringCount);
	_buff.append(reinterpret_cast<const char*>(t_tmp.data()), _StringPool.StringPoolHeader.stringCount * sizeof(qint32));

	qint32 t_styleOffsets_pos = _buff.size();
	t_tmp.resize(_StringPool.StringPoolHeader.styleCount);
	_buff.append(reinterpret_cast<const char*>(t_tmp.data()), _StringPool.StringPoolHeader.styleCount * sizeof(qint32));

	bool t_isUTF8 = (_StringPool.StringPoolHeader.flags & ResStringPool_header::UTF8_FLAG) != 0;

	reinterpret_cast<ResStringPool_header*>(_buff.data() + t_stringPoolHeader_pos)->stringsStart = _buff.size() - t_stringPoolHeader_pos;
	qint32 t_stringsBegin_pos = _buff.size();
	for (uint i = 0; i < _StringPool.StringPoolHeader.stringCount; i++)
	{
		reinterpret_cast<quint32*>(_buff.data() + t_stringOffsets_pos)[i] = _buff.size() - t_stringsBegin_pos;
		writeString(_buff, _StringPool.strings[i], t_isUTF8);
	}

	qint32 t_alignmentFill = (4 - (_buff.size() % 4)) % 4;
	if (t_alignmentFill > 0)
	{
		const char t_tmp[4] = { 0,0,0,0 };
		_buff.append(t_tmp, t_alignmentFill);
	}

	if (_StringPool.StringPoolHeader.styleCount > 0)
	{
		reinterpret_cast<ResStringPool_header*>(_buff.data() + t_stringPoolHeader_pos)->stylesStart = _buff.size() - t_stringPoolHeader_pos;
		qint32 t_stylesBegin_pos = _buff.size();
		for (uint i = 0; i < _StringPool.StringPoolHeader.styleCount; i++)
		{
			qint32 t_offset = _buff.size() - t_stylesBegin_pos;
			quint32* t_p = reinterpret_cast<quint32*>(_buff.data() + t_styleOffsets_pos);
			reinterpret_cast<quint32*>(_buff.data() + t_styleOffsets_pos)[i] = _buff.size() - t_stylesBegin_pos;
			writePoolSpan(_buff, _StringPool.styles[i]);
		}
		quint32 t_tmp[2] = { 0xFFFFFFFF,0xFFFFFFFF };
		_buff.append(reinterpret_cast<char*>(t_tmp), sizeof(t_tmp));
	}
	reinterpret_cast<ResTable_package*>(_buff.data() + t_stringPoolHeader_pos)->header.size = _buff.size() - t_stringPoolHeader_pos;
}
void QResArscParser::writeTablePackage(QByteArray& _buff)
{
	qint32 t_tablePackageHeader_pos = _buff.size();
	_buff.append(reinterpret_cast<const char*>(&m_tablePackage), sizeof(m_tablePackage));
	reinterpret_cast<ResTable_package*>(_buff.data() + t_tablePackageHeader_pos)->typeStrings = _buff.size() - t_tablePackageHeader_pos;
	writeStrPool(_buff, m_typeStringPool);
	reinterpret_cast<ResTable_package*>(_buff.data() + t_tablePackageHeader_pos)->keyStrings = _buff.size() - t_tablePackageHeader_pos;
	writeStrPool(_buff, m_keyStringPool);
	for (QMap<uint, TTableTypeData>::iterator i = m_tableTypeDatas.begin(); i != m_tableTypeDatas.end(); ++i)
	{
		TTableTypeData& t_tableTypeData = i.value();
		writeTableTypeSpec(_buff, t_tableTypeData.typeSpec);
		for (int j = 0; j < t_tableTypeData.typeDatas.size(); ++j)
		{
			writeTableType(_buff, t_tableTypeData.typeDatas[j]);
		}
	}
	reinterpret_cast<ResTable_package*>(_buff.data() + t_tablePackageHeader_pos)->header.size = _buff.size() - t_tablePackageHeader_pos;
}
void traverseTableType(const TTableTypeEx& _typeData, TRAVERSE_STRIDX_CALLBACK& _callBack)
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
}
//用于计算引用计数
void referenceCountInc(Res_value::_DataType _type, quint32& _index, QVector<int>* _referenceCount)
{
	if (_type != Res_value::_DataType::TYPE_STRING)
		return;
	Q_ASSERT(_index < (quint32)_referenceCount->size());
	(*_referenceCount)[_index]++;
}
//_beginIdx后面的所有值都加_addValue
void incIdxValue(Res_value::_DataType _type, quint32& _index, quint32 _beginIndex, int _addValue)
{
	if (_type != Res_value::_DataType::TYPE_STRING)
		return;
	if (_index >= _beginIndex)
		_index += _addValue;
}
//将某个值所有的引用都改成另一个值
void changIdxValue(Res_value::_DataType _type, quint32& _index, quint32 _oldIdx, quint32 _newIdx, int* _count)
{
	if (_type != Res_value::_DataType::TYPE_STRING)
		return;
	if (_index == _oldIdx)
	{
		_index = _newIdx;
		if (_count != NULL)
			(*_count)++;
	}
}

void QResArscParser::initStringReferenceCount(void)
{
	m_StringPool.referenceCount = QVector<int>(m_StringPool.strings.size(), 0);

	traversalAllValue(std::bind(&referenceCountInc, std::placeholders::_1, std::placeholders::_2, &m_StringPool.referenceCount));

	//如果一切正常，不应该有没用的字符串
	for (int i = 0; i < m_StringPool.referenceCount.size(); ++i)
	{
		Q_ASSERT(m_StringPool.referenceCount[i] != 0);
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
	Q_ASSERT(!t_ptrDefaultEntry.isNull());
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
QSharedPointer<ResTable_entry>& QResArscParser::getValue(uint32_t _typeid, uint32_t _specid, uint32_t _id)
{
	Q_ASSERT(m_tableTypeDatas.contains(_typeid));
	QVector<TTableTypeEx>& t_tableType = m_tableTypeDatas[_typeid].typeDatas;
	Q_ASSERT(_specid < (quint32)t_tableType.size());
	TTableTypeEx& t_type = t_tableType[_specid];
	return t_type.entryValue[_id];
}
void QResArscParser::deleteValue(uint32_t _typeid, uint32_t _specid, uint32_t _id)
{
	QSharedPointer<ResTable_entry>& t_ptrEntry = getValue(_typeid, _specid, _id);
	Q_ASSERT(!t_ptrEntry.isNull());
	if ((*t_ptrEntry).size == sizeof(ResTable_entry))
	{
		TTableValueEntry* t_EntryValue = reinterpret_cast<TTableValueEntry*>(t_ptrEntry.get());
		if (t_EntryValue->value.dataType == Res_value::_DataType::TYPE_STRING)
			deleteString(t_EntryValue->value.data);
	}
	else
	{
		TTableMapEntryEx* t_EntryValue = reinterpret_cast<TTableMapEntryEx*>(t_ptrEntry.get());
		for (int i = 0; i < t_EntryValue->tablemap.size(); ++i)
		{
			if (t_EntryValue->tablemap[i].value.dataType == Res_value::_DataType::TYPE_STRING)
				deleteString(t_EntryValue->tablemap[i].value.data);
		}
	}
	t_ptrEntry = QSharedPointer<ResTable_entry>();
}
void QResArscParser::setValue(uint32_t _typeid, uint32_t _specid, uint32_t _id, Res_value::_DataType _dataYype, uint32_t _data)
{
	QSharedPointer<ResTable_entry>& t_ptrEntry = getValue(_typeid, _specid, _id);
	Q_ASSERT(!t_ptrEntry.isNull());
	Q_ASSERT((*t_ptrEntry).size == sizeof(ResTable_entry));
	TTableValueEntry* t_EntryValue = reinterpret_cast<TTableValueEntry*>(t_ptrEntry.get());
	if (t_EntryValue->value.dataType == Res_value::_DataType::TYPE_STRING)
		deleteString(t_EntryValue->value.data);

	t_EntryValue->value.dataType = _dataYype;
	t_EntryValue->value.data = _data;
}
void QResArscParser::setValue(uint32_t _typeid, uint32_t _specid, uint32_t _id, uint32_t _idx, Res_value::_DataType _dataYype, uint32_t _data)
{
	QSharedPointer<ResTable_entry>& t_ptrEntry = getValue(_typeid, _specid, _id);
	Q_ASSERT(!t_ptrEntry.isNull());
	Q_ASSERT((*t_ptrEntry).size != sizeof(ResTable_entry));
	TTableMapEntryEx* t_pMapValue = reinterpret_cast<TTableMapEntryEx*>(t_ptrEntry.get());
	Q_ASSERT(_idx < (uint32_t)t_pMapValue->tablemap.size());
	ResTable_map& t_tableMap = t_pMapValue->tablemap[_idx];
	if (t_tableMap.value.dataType == Res_value::_DataType::TYPE_STRING)
		deleteString(t_tableMap.value.data);
	t_tableMap.value.dataType = _dataYype;
	t_tableMap.value.data = _data;
}
void QResArscParser::setValue(uint32_t _typeid, uint32_t _specid, uint32_t _id, const QString& _data, bool _force)
{
	QString t_str = _data;
	t_str.replace(QString("\\n"), QChar(0x0A));

	QSharedPointer<ResTable_entry>& t_ptrEntry = getValue(_typeid, _specid, _id);
	Q_ASSERT(!t_ptrEntry.isNull());
	Q_ASSERT((*t_ptrEntry).size == sizeof(ResTable_entry));
	TTableValueEntry* t_EntryValue = reinterpret_cast<TTableValueEntry*>(t_ptrEntry.get());
	if (t_EntryValue->value.dataType == Res_value::_DataType::TYPE_STRING)
		t_EntryValue->value.data = replaceString(t_EntryValue->value.data, t_str, _force);
	else
		t_EntryValue->value.data = insertString(t_str);
}
void QResArscParser::setValue(uint32_t _typeid, uint32_t _specid, uint32_t _id, uint32_t _idx, const QString& _data, bool _force)
{
	QString t_str = _data;
	t_str.replace(QString("\\n"), QChar(0x0A));

	QSharedPointer<ResTable_entry>& t_ptrEntry = getValue(_typeid, _specid, _id);
	Q_ASSERT(!t_ptrEntry.isNull());
	Q_ASSERT((*t_ptrEntry).size != sizeof(ResTable_entry));
	TTableMapEntryEx* t_pMapValue = reinterpret_cast<TTableMapEntryEx*>(t_ptrEntry.get());
	Q_ASSERT(_idx < (uint32_t)t_pMapValue->tablemap.size());
	ResTable_map& t_tableMap = t_pMapValue->tablemap[_idx];
	if (t_tableMap.value.dataType == Res_value::_DataType::TYPE_STRING)
		t_tableMap.value.data = replaceString(t_tableMap.value.data, t_str, _force);
	else
		t_tableMap.value.data = insertString(t_str);
}
quint32 QResArscParser::getReferenceCount(quint32 _index)
{
	Q_ASSERT(_index < (quint32)m_StringPool.referenceCount.size());
	return m_StringPool.referenceCount[_index];
}
quint32 QResArscParser::replaceString(quint32 _oldIndex, const QString& _str, bool _force)
{
	//引用数等于1的强制修改，等于不强制
	if (_force && m_StringPool.referenceCount[_oldIndex] == 1)
		_force = false;

	if (m_StringPool.referenceCount[_oldIndex] == 1 || _force)
	{
		//对于修改，比较复杂，这个字符串是否是富文本如果变化，需要先删除旧字符串，然后添加一个新的，否则就直接修改就好了。
		TStringPoolSpans t_newSpanEx;
		QString t_newStr(_str);
		decodeRichText(t_newStr, t_newSpanEx, m_StringPool);

		if (t_newStr == m_StringPool.strings[_oldIndex])
		{
			if ((_oldIndex >= (uint32_t)m_StringPool.styles.size() && t_newSpanEx.spans.size() == 0) ||
				(_oldIndex < (uint32_t)m_StringPool.styles.size() && t_newSpanEx == m_StringPool.styles[_oldIndex]))
			{
				qDebug() << "string no chang: " << _str;
				return _oldIndex;
			}
		}

		if ((t_newSpanEx.spans.size() > 0) != (_oldIndex < (quint32)m_StringPool.styles.size()))
		{
			//字符串是否是富文本有变化，需要先删除旧字符串，然后添加一个新的
			//如果force，则需要把所有指向该字符串的数据都先指向一个特殊标记，然后删除老的，添加新的，最后把所有这个特殊标记的都指向新字符串
			//如果只有一个，就不需要这样修改，
			uint32_t t_rCount = m_StringPool.referenceCount[_oldIndex];
			if (_force)
			{
				int t_chgCount = 0;
				traversalAllValue(std::bind(&changIdxValue, std::placeholders::_1, std::placeholders::_2, _oldIndex, 0x78787878, &t_chgCount));
				qDebug() << "find number of citations is " << t_chgCount << " :" << m_StringPool.strings[_oldIndex];
				Q_ASSERT(t_chgCount == t_rCount && t_rCount > 1);
			}
			uint32_t t_newIndex = 0;
			deleteString(_oldIndex, _force);
			bool t_isInsert;	//是真的插入了新的，还是和以前的一样
			if (t_newSpanEx.spans.size() == 0)
				t_newIndex = insertSimpleString(t_newStr, t_isInsert);
			else
				t_newIndex = insertRichString(t_newStr, t_newSpanEx, t_isInsert);
			if (_force)
			{
				int t_chgCount = 0;
				traversalAllValue(std::bind(&changIdxValue, std::placeholders::_1, std::placeholders::_2, 0x78787878 - (t_isInsert ? 0 : 1), t_newIndex, &t_chgCount));
				Q_ASSERT(t_chgCount == t_rCount);
			}
			m_StringPool.referenceCount[t_newIndex] += t_rCount;
			qDebug() << "number of citations add " << t_rCount << " chang to " << m_StringPool.referenceCount[t_newIndex] << " :" << m_StringPool.strings[t_newIndex];
			return t_newIndex;
		}
		else
		{
			//直接修改
			TRichString t_oldRichStr;
			if (_oldIndex < (quint32)m_StringPool.styles.size())
				t_oldRichStr = TRichString(m_StringPool.strings[_oldIndex], m_StringPool.styles[_oldIndex]);
			else
				t_oldRichStr.str = m_StringPool.strings[_oldIndex];
			TRichString t_newRichStr(t_newStr, t_newSpanEx);

			//如果新字符串和某个其他字符串相等，那就删除旧的，相等的那个引用加1
			if (m_StringPool.strIndexs.contains(t_newRichStr))
			{
				//if force 引用不是加1，而是加其他的。另外需要象上面一样两次遍历
				uint32_t t_rCount = m_StringPool.referenceCount[_oldIndex];
				if (_force)
				{
					int t_chgCount = 0;
					traversalAllValue(std::bind(&changIdxValue, std::placeholders::_1, std::placeholders::_2, _oldIndex, 0x78787878, &t_chgCount));
					Q_ASSERT(t_chgCount == t_rCount && t_rCount > 1);
				}
				deleteString(_oldIndex, _force);
				quint32 t_newIndex = m_StringPool.strIndexs[t_newRichStr];

				m_StringPool.referenceCount[t_newIndex] += t_rCount;
				qDebug() << "number of citations add " << t_rCount << " chang to " << m_StringPool.referenceCount[t_newIndex] << " :" << m_StringPool.strings[t_newIndex];
				if (_force)
				{
					int t_chgCount = 0;
					//上面删除了一个字符串，字符串的索引数不可能有0x78787878，所以0x78787878肯定被减了1
					traversalAllValue(std::bind(&changIdxValue, std::placeholders::_1, std::placeholders::_2, 0x78787878 - 1, t_newIndex, &t_chgCount));
					Q_ASSERT(t_chgCount == t_rCount);
				}
				return t_newIndex;
			}
			else
			{

				m_StringPool.strIndexs.remove(t_oldRichStr);
				qDebug() << "index: " << _oldIndex << " str:" << m_StringPool.strings[_oldIndex] << " chang to:" << t_newStr;
				m_StringPool.strings[_oldIndex] = t_newStr;
				m_StringPool.strIndexs.insert(TRichString(t_newStr, t_newSpanEx), _oldIndex);
				if (t_newSpanEx.spans.size() > 0)
				{
					Q_ASSERT(_oldIndex < (quint32)m_StringPool.styles.size());
					m_StringPool.styles[_oldIndex] = t_newSpanEx;
				}
				return _oldIndex;
			}
		}
	}
	else
	{
		//原来的字符串只是引用数减1，然后加新字符串
		m_StringPool.referenceCount[_oldIndex]--;
		qDebug() << "number of citations --:" + m_StringPool.strings[_oldIndex];
		return insertString(_str);
	}
	return 0;
}
quint32 QResArscParser::insertString(const QString& _str)
{
	TStringPoolSpans t_spanEx;
	QString t_str(_str);
	decodeRichText(t_str, t_spanEx, m_StringPool);
	uint32_t t_newIndex = 0;
	bool t_isInsert;
	if (t_spanEx.spans.size() > 0)
		t_newIndex = insertRichString(t_str, t_spanEx, t_isInsert);
	else
		t_newIndex = insertSimpleString(t_str, t_isInsert);
	m_StringPool.referenceCount[t_newIndex]++;
	qDebug() << "number of citations ++:" + m_StringPool.strings[t_newIndex];
	return t_newIndex;
}
void QResArscParser::deleteString(quint32 _index, bool _force)
{
	//如果引用数大于1，并且不是强制删除，那减引用数，否则删字符串
	if (m_StringPool.referenceCount[_index] > 1 && !_force)
	{
		qDebug() << "number of citations --:" + m_StringPool.strings[_index];
		m_StringPool.referenceCount[_index]--;
		return;
	}
	else
	{
		TRichString t_richStr;
		t_richStr.str = m_StringPool.strings[_index];
		qDebug() << "delete string:" + m_StringPool.strings[_index];
		//删除实际字符串
		m_StringPool.StringPoolHeader.stringCount--;
		m_StringPool.strings.remove(_index, 1);
		//删除可能的格式表
		if (_index < (quint32)m_StringPool.styles.size())
		{
			t_richStr.span = m_StringPool.styles[_index];
			m_StringPool.styles.remove(_index, 1);
			m_StringPool.StringPoolHeader.styleCount--;
		}
		//删除引用计数
		m_StringPool.referenceCount.remove(_index, 1);
		//修改字符串内容和索引的对应关系
		m_StringPool.strIndexs.remove(t_richStr);
		for (QMap<TRichString, quint32>::iterator i = m_StringPool.strIndexs.begin(); i != m_StringPool.strIndexs.end(); ++i)
			if (i.value() > _index)
				i.value()--;
		//修改所有用到索引的地方，大于这个索引的，都减1
		traversalAllValue(std::bind(&incIdxValue, std::placeholders::_1, std::placeholders::_2, _index + 1, -1));
	}
}
quint32 QResArscParser::insertSimpleString(const QString& _str, bool& _isInsert)
{
	TRichString t_richStr;
	t_richStr.str = _str;
	if (m_StringPool.strIndexs.contains(t_richStr))
	{
		_isInsert = false;
		return m_StringPool.strIndexs[t_richStr];
	}
	else
	{
		qDebug() << "insert simple string:" + _str;
		_isInsert = true;
		m_StringPool.strings.append(_str);
		m_StringPool.referenceCount.append(0);
		m_StringPool.strIndexs.insert(t_richStr, m_StringPool.StringPoolHeader.stringCount);
		//加在最后，所以返回的索引值就是原来的个数，并且因为加在最后，所以其他的索引都不需要改动
		return m_StringPool.StringPoolHeader.stringCount++;
	}
}
quint32 QResArscParser::insertRichString(const QString& _str, TStringPoolSpans& _span, bool& _isInsert)
{
	TRichString t_richStr(_str, _span);
	if (m_StringPool.strIndexs.contains(t_richStr))
	{
		_isInsert = false;
		return m_StringPool.strIndexs[t_richStr];
	}
	else
	{
		//先遍历所有字符串引用，将所有简单字符串的编号+1，在富文本字符串的最后添加
		qDebug() << "insert rich string:" + _str;
		_isInsert = true;
		traversalAllValue(std::bind(&incIdxValue, std::placeholders::_1, std::placeholders::_2, m_StringPool.styles.size(), 1));
		quint32 t_idx = m_StringPool.styles.size();		//插入的富文本编号
		m_StringPool.strings.insert(t_idx, _str);
		m_StringPool.referenceCount.insert(t_idx, 0);

		for (QMap<TRichString, quint32>::iterator i = m_StringPool.strIndexs.begin(); i != m_StringPool.strIndexs.end(); ++i)
			if (i.value() >= t_idx)		//包括原来的t_idx，也得往后挪一个
				i.value()++;
		m_StringPool.strIndexs.insert(t_richStr, t_idx);

		//要插入的符号索引也得加1，其他的都在前面的traversalAllValue里加了。
		for (int i = 0; i < _span.spans.size(); ++i)
			_span.spans[i].name.index++;
		m_StringPool.styles.append(_span);
		m_StringPool.StringPoolHeader.stringCount++;
		return m_StringPool.StringPoolHeader.styleCount++;
	}
}
