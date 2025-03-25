#include "StringPoolExtend.h"
#include <QtDebug>
#include "QUtf8.h"
#include "basicDefine.h"

static quint32 writeStringLen_16(char* _pBuff, const QString& _str)
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
static quint32 writeStringLen_8(char* _pBuff, const QString& _str, QByteArray& _utf8Array)
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
static quint32 readStringLen_16(const char*& _pBuff)
{
	//看逻辑，UTF16字符串最长可以有0x7FFF FFFF长
	quint16 t_u16len = readValue<quint16>(_pBuff);
	if ((t_u16len & 0x8000) == 0)
		return t_u16len;
	else
		return ((t_u16len & 0x7FFF) << 16) + readValue<ushort>(_pBuff);
}
static uint readStringLen_8(const char*& _pBuff)
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
static void readString(QString& _string, const char* _pBuff, bool _isUTF8)
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
static void writeString(QByteArray& _buff, const QString& _string, bool _isUTF8)
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
//用于计算引用计数
static void referenceCountInc(Res_value::_DataType _type, quint32& _index, QVector<int>* _referenceCount)
{
	if (_type != Res_value::_DataType::TYPE_STRING)
		return;
	Q_ASSERT(_index < (quint32)_referenceCount->size());
	(*_referenceCount)[_index]++;
}
//_beginIdx后面的所有值都加_addValue
static void incIdxValue(Res_value::_DataType _type, quint32& _index, quint32 _beginIndex, int _addValue)
{
	if (_type != Res_value::_DataType::TYPE_STRING)
		return;
	if (_index >= _beginIndex)
		_index += _addValue;
}
//将某个值所有的引用都改成另一个值
static void changIdxValue(Res_value::_DataType _type, quint32& _index, quint32 _oldIdx, quint32 _newIdx, int* _count)
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

//-----------------------------------------------------------
void TStringPool::makeIndexs(void)
{
	strIndexs.clear();

	for (int i = 0; i < strings.size(); ++i)
	{
		TRichString t_richStr;
		if (i < styles.size())
			t_richStr = TRichString(strings[i], styles[i]);
		else
			t_richStr.str = strings[i];

		Q_ASSERT(!strIndexs.contains(t_richStr));
		strIndexs.insert(t_richStr, quint32(i));
	}
}
void TStringPool::reset(void)
{
	strings.resize(0);
	styles.resize(0);
	strIndexs.clear();
	referenceCount.resize(0);
}
void TStringPool::readStrPool(const char* _pBegin)
{
	StringPoolHeader = *reinterpret_cast<const ResStringPool_header*>(_pBegin);
	Q_ASSERT(StringPoolHeader.header.type == RES_TYPE::RES_STRING_POOL_TYPE);
	const char* t_pBuff = _pBegin + sizeof(StringPoolHeader);

	QVector<quint32> t_stringOffsets(StringPoolHeader.stringCount);
	memcpy(t_stringOffsets.data(), t_pBuff, StringPoolHeader.stringCount * sizeof(quint32));
	t_pBuff += StringPoolHeader.stringCount * sizeof(quint32);
	QVector<quint32> t_styleOffsets(StringPoolHeader.styleCount);
	memcpy(t_styleOffsets.data(), t_pBuff, StringPoolHeader.styleCount * sizeof(quint32));

	bool t_isUTF8 = (StringPoolHeader.flags & ResStringPool_header::UTF8_FLAG) != 0;

	t_pBuff = _pBegin + StringPoolHeader.stringsStart;
	strings.resize(StringPoolHeader.stringCount);
	for (uint i = 0; i < StringPoolHeader.stringCount; i++)
		readString(strings[i], t_pBuff + t_stringOffsets[i], t_isUTF8);

	t_pBuff = _pBegin + StringPoolHeader.stylesStart;
	styles.resize(StringPoolHeader.styleCount);
	for (uint i = 0; i < StringPoolHeader.styleCount; i++)
		readPoolSpan(styles[i], t_pBuff + t_styleOffsets[i]);

	makeIndexs();
}
void TStringPool::writeStrPool(QByteArray& _buff) const
{
	//因为后面还要修改，_buff的内存随着变大申请新内存，可能会移动到别处，所以不能保存指针使用。只能随用随取。
	qint32 t_stringPoolHeader_pos = _buff.size();
	_buff.append(reinterpret_cast<const char*>(&StringPoolHeader), sizeof(StringPoolHeader));

	qint32 t_stringOffsets_pos = _buff.size();
	QVector<qint32> t_tmp(StringPoolHeader.stringCount);
	_buff.append(reinterpret_cast<const char*>(t_tmp.data()), StringPoolHeader.stringCount * sizeof(qint32));

	qint32 t_styleOffsets_pos = _buff.size();
	t_tmp.resize(StringPoolHeader.styleCount);
	_buff.append(reinterpret_cast<const char*>(t_tmp.data()), StringPoolHeader.styleCount * sizeof(qint32));

	bool t_isUTF8 = (StringPoolHeader.flags & ResStringPool_header::UTF8_FLAG) != 0;

	reinterpret_cast<ResStringPool_header*>(_buff.data() + t_stringPoolHeader_pos)->stringsStart = _buff.size() - t_stringPoolHeader_pos;
	qint32 t_stringsBegin_pos = _buff.size();
	for (uint i = 0; i < StringPoolHeader.stringCount; i++)
	{
		reinterpret_cast<quint32*>(_buff.data() + t_stringOffsets_pos)[i] = _buff.size() - t_stringsBegin_pos;
		writeString(_buff, strings[i], t_isUTF8);
	}

	qint32 t_alignmentFill = (4 - (_buff.size() % 4)) % 4;
	if (t_alignmentFill > 0)
	{
		const char t_tmp[4] = { 0,0,0,0 };
		_buff.append(t_tmp, t_alignmentFill);
	}

	if (StringPoolHeader.styleCount > 0)
	{
		reinterpret_cast<ResStringPool_header*>(_buff.data() + t_stringPoolHeader_pos)->stylesStart = _buff.size() - t_stringPoolHeader_pos;
		qint32 t_stylesBegin_pos = _buff.size();
		for (uint i = 0; i < StringPoolHeader.styleCount; i++)
		{
			qint32 t_offset = _buff.size() - t_stylesBegin_pos;
			quint32* t_p = reinterpret_cast<quint32*>(_buff.data() + t_styleOffsets_pos);
			reinterpret_cast<quint32*>(_buff.data() + t_styleOffsets_pos)[i] = _buff.size() - t_stylesBegin_pos;
			writePoolSpan(_buff, styles[i]);
		}
		//PoolSpan以2个0xFFFFFFFF结束
		quint32 t_tmp[2] = { 0xFFFFFFFF,0xFFFFFFFF };
		_buff.append(reinterpret_cast<char*>(t_tmp), sizeof(t_tmp));
	}
	reinterpret_cast<ResTable_package*>(_buff.data() + t_stringPoolHeader_pos)->header.size = _buff.size() - t_stringPoolHeader_pos;
}
void TStringPool::readPoolSpan(TStringPoolSpans& _span, const char* _pBuff)
{
	while (*reinterpret_cast<const uint*>(_pBuff) != ResStringPool_span::END)
	{
		ResStringPool_span t_span = *reinterpret_cast<const ResStringPool_span*>(_pBuff);
		_span.spans.append(t_span);
		_pBuff += sizeof(ResStringPool_span);
	}
}
void TStringPool::writePoolSpan(QByteArray& _buff, const TStringPoolSpans& _span)
{
	_buff.append(reinterpret_cast<const char*>(_span.spans.data()), _span.spans.size() * sizeof(ResStringPool_span));
	quint32 t_tmp = ResStringPool_span::END;
	_buff.append(reinterpret_cast<const char*>(&t_tmp), sizeof(t_tmp));
}
quint32 TStringPool::insertString(const QString& _str, uint32_t _addRefCount, bool* _isInsert, TTRAVERSAL_ALL_VALUE _traversalAllValue)
{
	TStringPoolSpans t_spanEx;
	QString t_str(_str);
	decodeRichText(t_str, t_spanEx, *this);
	uint32_t t_newIndex = 0;
	if (t_spanEx.spans.size() > 0)
		t_newIndex = insertRichString(t_str, t_spanEx, _isInsert, _traversalAllValue);
	else
		t_newIndex = insertSimpleString(t_str, _isInsert);
	referenceCount[t_newIndex] += _addRefCount;
	qDebug() << "number of citations add " << _addRefCount << " chang to:" << referenceCount[t_newIndex] << " :" << strings[t_newIndex];
	return t_newIndex;
}
quint32 TStringPool::insertSimpleString(const QString& _str, bool* _isInsert)
{
	TRichString t_richStr;
	t_richStr.str = _str;
	if (strIndexs.contains(t_richStr))
	{
		if (_isInsert != NULL)
			*_isInsert = false;
		return strIndexs[t_richStr];
	}
	else
	{
		qDebug() << "insert simple string:" + _str;
		if (_isInsert != NULL)
			*_isInsert = true;
		strings.append(_str);
		referenceCount.append(0);
		strIndexs.insert(t_richStr, StringPoolHeader.stringCount);
		//加在最后，所以返回的索引值就是原来的个数，并且因为加在最后，所以其他的索引都不需要改动
		return StringPoolHeader.stringCount++;
	}
}
quint32 TStringPool::insertRichString(const QString& _str, TStringPoolSpans& _span, bool* _isInsert, TTRAVERSAL_ALL_VALUE _traversalAllValue)
{
	TRichString t_richStr(_str, _span);
	if (strIndexs.contains(t_richStr))
	{
		if (_isInsert != NULL)
			*_isInsert = false;
		return strIndexs[t_richStr];
	}
	else
	{
		//先遍历所有字符串引用，将所有简单字符串的编号+1，在富文本字符串的最后添加
		qDebug() << "insert rich string:" + _str;
		if (_isInsert != NULL)
			*_isInsert = true;

		//插入数据
		quint32 t_idx = styles.size();
		strings.insert(t_idx, _str);
		referenceCount.insert(t_idx, 0);
		styles.append(_span);

		for (QMap<TRichString, quint32>::iterator i = strIndexs.begin(); i != strIndexs.end(); ++i)
			if (i.value() >= t_idx)		//包括原来的t_idx，也得往后挪一个
				i.value()++;
		strIndexs.insert(t_richStr, t_idx);

		//遍历修改所有需要修改的引用数据
		_traversalAllValue(std::bind(&incIdxValue, std::placeholders::_1, std::placeholders::_2, styles.size() - 1, 1));

		StringPoolHeader.stringCount++;
		return StringPoolHeader.styleCount++;
	}
}
bool TStringPool::deleteString(quint32 _index, bool _force, TTRAVERSAL_ALL_VALUE _traversalAllValue)
{
	//如果引用数大于1，并且不是强制删除，那减引用数，否则删字符串
	if (referenceCount[_index] > 1 && !_force)
	{
		qDebug() << "number of citations --:" + strings[_index];
		referenceCount[_index]--;
		return false;
	}
	else
	{
		TRichString t_richStr;
		t_richStr.str = strings[_index];
		qDebug() << "delete string:" + strings[_index];
		//删除实际字符串
		StringPoolHeader.stringCount--;
		strings.remove(_index, 1);
		//删除可能的格式表
		if (_index < (quint32)styles.size())
		{
			t_richStr.span = styles[_index];
			styles.remove(_index, 1);
			StringPoolHeader.styleCount--;
		}
		//删除引用计数
		referenceCount.remove(_index, 1);
		//修改字符串内容和索引的对应关系
		strIndexs.remove(t_richStr);
		for (QMap<TRichString, quint32>::iterator i = strIndexs.begin(); i != strIndexs.end(); ++i)
			if (i.value() > _index)
				i.value()--;
		//修改所有用到索引的地方，大于这个索引的，都减1
		_traversalAllValue(std::bind(&incIdxValue, std::placeholders::_1, std::placeholders::_2, _index + 1, -1));
		return true;
	}
}
quint32 TStringPool::replaceString(quint32 _oldIndex, const QString& _str, bool _force, TTRAVERSAL_ALL_VALUE _traversalAllValue)
{
	//引用数等于1的强制修改，等于不强制
	if (_force && referenceCount[_oldIndex] == 1)
		_force = false;

	TStringPoolSpans t_newSpan;
	QString t_newStr(_str);
	decodeRichText(t_newStr, t_newSpan, *this);

	if (t_newStr == strings[_oldIndex])
	{
		if ((_oldIndex >= (uint32_t)styles.size() && t_newSpan.spans.size() == 0) ||
			(_oldIndex < (uint32_t)styles.size() && t_newSpan == styles[_oldIndex]))
		{
			qDebug() << "string no chang: " << _str;
			return _oldIndex;
		}
	}

	if (referenceCount[_oldIndex] == 1 || _force)
	{
		//对于修改，比较复杂，这个字符串是否是富文本如果变化，需要先删除旧字符串，然后添加一个新的，否则就直接修改就好了。
		if ((t_newSpan.spans.size() > 0) != (_oldIndex < (quint32)styles.size()))
		{
			//字符串是否是富文本有变化，添加一个新的，删除老的。
			uint32_t t_rCount = referenceCount[_oldIndex];
			bool t_realInsert;	//是真的插入了新的，还是只是修改了引用数?
			uint32_t t_newIndex = insertString(_str, t_rCount, &t_realInsert, _traversalAllValue);
			//真的插入了，而且插入的在_oldIndex之前，_oldIndex需要往后挪一位
			if (t_realInsert && t_newIndex <= _oldIndex)
				_oldIndex++;

			//将所有指向旧的字符串的，都指向新的。
			int t_chgCount = 0;
			_traversalAllValue(std::bind(&changIdxValue, std::placeholders::_1, std::placeholders::_2, _oldIndex, t_newIndex, &t_chgCount));
			qDebug() << "find number of citations is " << t_chgCount << " :" << strings[_oldIndex];
			Q_ASSERT(t_chgCount == t_rCount);
			//删除旧的
			bool t_realDelete = deleteString(_oldIndex, _force, _traversalAllValue);
			//如果删除的_oldIndex在t_newIndex之前，t_newIndex需要往前挪一位
			if (t_realDelete && _oldIndex < t_newIndex)
				t_newIndex--;
			return t_newIndex;
		}
		else
		{
			//是否是富文本没有变化，直接修改
			TRichString t_oldRichStr;
			if (_oldIndex < (quint32)styles.size())
				t_oldRichStr = TRichString(strings[_oldIndex], styles[_oldIndex]);
			else
				t_oldRichStr.str = strings[_oldIndex];
			TRichString t_newRichStr(t_newStr, t_newSpan);

			//如果新字符串和某个其他字符串相等，那就删除旧的，相等的那个引用加1
			if (strIndexs.contains(t_newRichStr))
			{
				//新指向的字符串加上引用数，然后把指向旧字符串的指向，都重定向到新字符串
				uint32_t t_rCount = referenceCount[_oldIndex];
				quint32 t_newIndex = strIndexs[t_newRichStr];
				referenceCount[t_newIndex] += t_rCount;
				qDebug() << "number of citations add " << t_rCount << " chang to " << referenceCount[t_newIndex] << " :" << strings[t_newIndex];

				int t_chgCount = 0;
				_traversalAllValue(std::bind(&changIdxValue, std::placeholders::_1, std::placeholders::_2, _oldIndex, t_newIndex, &t_chgCount));
				Q_ASSERT(t_chgCount == t_rCount);
				//最后删除旧字符串
				bool t_realDelete = deleteString(_oldIndex, _force, _traversalAllValue);

				if (t_realDelete && _oldIndex < t_newIndex)
					t_newIndex--;
				return t_newIndex;
			}
			else
			{
				strIndexs.remove(t_oldRichStr);
				qDebug() << "index: " << _oldIndex << " str:" << strings[_oldIndex] << " chang to:" << t_newStr;
				strings[_oldIndex] = t_newStr;
				strIndexs.insert(TRichString(t_newStr, t_newSpan), _oldIndex);
				if (t_newSpan.spans.size() > 0)
				{
					Q_ASSERT(_oldIndex < (quint32)styles.size());
					styles[_oldIndex] = t_newSpan;
				}
				return _oldIndex;
			}
		}
	}
	else
	{
		//原来的字符串只是引用数减1，然后加新字符串
		referenceCount[_oldIndex]--;
		qDebug() << "number of citations --:" + strings[_oldIndex];
		return insertString(_str, 1, NULL, _traversalAllValue);
	}
}
void TStringPool::initStringReferenceCount(TTRAVERSAL_ALL_VALUE _traversalAllValue)
{
	referenceCount = QVector<int>(strings.size(), 0);

	_traversalAllValue(std::bind(&referenceCountInc, std::placeholders::_1, std::placeholders::_2, &referenceCount));

	//如果一切正常，不应该有没用的字符串
	for (int i = 0; i < referenceCount.size(); ++i)
	{
		Q_ASSERT(referenceCount[i] != 0);
	}

}
