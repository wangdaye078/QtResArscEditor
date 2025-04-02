#include "StringPoolExtend.h"
#include <QtDebug>
#include "QUtf8.h"
#include "basicDefine.h"

static uint32_t writeStringLen_16(char* _pBuff, const QString& _str)
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
static uint32_t writeStringLen_8(char* _pBuff, const QString& _str, QByteArray& _utf8Array)
{
	qint8* t_pBuff = reinterpret_cast<qint8*>(_pBuff);
	uint32_t t_pos = 0;
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
static uint32_t readStringLen_16(const char*& _pBuff)
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
		uint32_t t_lenlen = writeStringLen_16(t_len, _string);
		_buff.append(t_len, t_lenlen);
		_buff.append(reinterpret_cast<const char*>(_string.utf16()), (_string.length() + 1) * sizeof(ushort));
	}
	else
	{
		QByteArray t_utf8Array;
		uint32_t t_lenlen = writeStringLen_8(t_len, _string, t_utf8Array);
		_buff.append(t_len, t_lenlen);
		_buff.append(t_utf8Array.data(), t_utf8Array.size() + 1);
	}
}
//用于计算引用计数
static void referenceCountInc(Res_value::_DataType _type, uint32_t& _index, QVector<int>* _referenceCount)
{
	if (_type != Res_value::_DataType::TYPE_STRING)
		return;
	Q_ASSERT(_index < (uint32_t)_referenceCount->size());
	(*_referenceCount)[_index]++;
}
//_beginIdx后面的所有值都加_addValue
static void incIdxValue(Res_value::_DataType _type, uint32_t& _index, uint32_t _beginIndex, int _addValue)
{
	if (_type != Res_value::_DataType::TYPE_STRING)
		return;
	if (_index >= _beginIndex)
		_index += _addValue;
}
//将某个值所有的引用都改成另一个值
static void changIdxValue(Res_value::_DataType _type, uint32_t& _index, uint32_t _oldIdx, uint32_t _newIdx, int* _count)
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
TStringPool::TStringPool()
{

}
void TStringPool::makeIndexs(void)
{
	m_strIndexs.clear();

	for (int i = 0; i < m_strings.size(); ++i)
	{
		TRichString t_richStr;
		if (i < m_styles.size())
			t_richStr = TRichString(m_strings[i], m_styles[i]);
		else
			t_richStr.str = m_strings[i];

		Q_ASSERT(!m_strIndexs.contains(t_richStr));
		m_strIndexs.insert(t_richStr, uint32_t(i));
	}
}
void TStringPool::reset(void)
{
	m_strings.resize(0);
	m_styles.resize(0);
	m_strIndexs.clear();
	m_referenceCounts.resize(0);
}
void TStringPool::readStrPool(const char* _pBegin)
{
	m_stringPoolHeader = *reinterpret_cast<const ResStringPool_header*>(_pBegin);
	Q_ASSERT(m_stringPoolHeader.header.type == RES_TYPE::RES_STRING_POOL_TYPE);
	const char* t_pBuff = _pBegin + sizeof(m_stringPoolHeader);

	QVector<uint32_t> t_stringOffsets(m_stringPoolHeader.stringCount);
	memcpy(t_stringOffsets.data(), t_pBuff, m_stringPoolHeader.stringCount * sizeof(uint32_t));
	t_pBuff += m_stringPoolHeader.stringCount * sizeof(uint32_t);
	QVector<uint32_t> t_styleOffsets(m_stringPoolHeader.styleCount);
	memcpy(t_styleOffsets.data(), t_pBuff, m_stringPoolHeader.styleCount * sizeof(uint32_t));

	bool t_isUTF8 = (m_stringPoolHeader.flags & ResStringPool_header::UTF8_FLAG) != 0;

	t_pBuff = _pBegin + m_stringPoolHeader.stringsStart;
	m_strings.resize(m_stringPoolHeader.stringCount);
	for (uint i = 0; i < m_stringPoolHeader.stringCount; i++)
		readString(m_strings[i], t_pBuff + t_stringOffsets[i], t_isUTF8);

	t_pBuff = _pBegin + m_stringPoolHeader.stylesStart;
	m_styles.resize(m_stringPoolHeader.styleCount);
	for (uint i = 0; i < m_stringPoolHeader.styleCount; i++)
		readPoolSpan(m_styles[i], t_pBuff + t_styleOffsets[i]);

	makeIndexs();
}
void TStringPool::writeStrPool(QByteArray& _buff) const
{
	//因为后面还要修改，_buff的内存随着变大申请新内存，可能会移动到别处，所以不能保存指针使用。只能随用随取。
	int t_stringPoolHeader_pos = _buff.size();
	_buff.append(reinterpret_cast<const char*>(&m_stringPoolHeader), sizeof(m_stringPoolHeader));

	int t_stringOffsets_pos = _buff.size();
	QVector<uint32_t> t_tmp(m_stringPoolHeader.stringCount);
	_buff.append(reinterpret_cast<const char*>(t_tmp.data()), m_stringPoolHeader.stringCount * sizeof(uint32_t));

	int t_styleOffsets_pos = _buff.size();
	t_tmp.resize(m_stringPoolHeader.styleCount);
	_buff.append(reinterpret_cast<const char*>(t_tmp.data()), m_stringPoolHeader.styleCount * sizeof(uint32_t));

	bool t_isUTF8 = (m_stringPoolHeader.flags & ResStringPool_header::UTF8_FLAG) != 0;

	reinterpret_cast<ResStringPool_header*>(_buff.data() + t_stringPoolHeader_pos)->stringsStart = _buff.size() - t_stringPoolHeader_pos;
	qint32 t_stringsBegin_pos = _buff.size();
	for (uint i = 0; i < m_stringPoolHeader.stringCount; i++)
	{
		reinterpret_cast<uint32_t*>(_buff.data() + t_stringOffsets_pos)[i] = _buff.size() - t_stringsBegin_pos;
		writeString(_buff, m_strings[i], t_isUTF8);
	}
	//字符串部分最后需要4字节对齐。
	int t_alignmentFill = (4 - (_buff.size() % 4)) % 4;
	if (t_alignmentFill > 0)
	{
		const char t_tmp[4] = { 0,0,0,0 };
		_buff.append(t_tmp, t_alignmentFill);
	}

	if (m_stringPoolHeader.styleCount > 0)
	{
		reinterpret_cast<ResStringPool_header*>(_buff.data() + t_stringPoolHeader_pos)->stylesStart = _buff.size() - t_stringPoolHeader_pos;
		int t_stylesBegin_pos = _buff.size();
		for (uint i = 0; i < m_stringPoolHeader.styleCount; i++)
		{
			reinterpret_cast<uint32_t*>(_buff.data() + t_styleOffsets_pos)[i] = _buff.size() - t_stylesBegin_pos;
			writePoolSpan(_buff, m_styles[i]);
		}
		//PoolSpan以2个0xFFFFFFFF结束
		uint32_t t_tmp[2] = { (uint32_t)ResStringPool_span::END, (uint32_t)ResStringPool_span::END };
		_buff.append(reinterpret_cast<char*>(t_tmp), sizeof(t_tmp));
	}
	reinterpret_cast<ResTable_package*>(_buff.data() + t_stringPoolHeader_pos)->header.size = _buff.size() - t_stringPoolHeader_pos;
}
void TStringPool::readPoolSpan(TStringPoolSpans& _span, const char* _pBuff)
{
	while (*reinterpret_cast<const uint*>(_pBuff) != ResStringPool_span::END)
	{
		ResStringPool_span t_span = *reinterpret_cast<const ResStringPool_span*>(_pBuff);
		_span.append(t_span);
		_pBuff += sizeof(ResStringPool_span);
	}
}
void TStringPool::writePoolSpan(QByteArray& _buff, const TStringPoolSpans& _span)
{
	_buff.append(reinterpret_cast<const char*>(_span.data()), _span.size() * sizeof(ResStringPool_span));
	uint32_t t_tmp = ResStringPool_span::END;
	_buff.append(reinterpret_cast<const char*>(&t_tmp), sizeof(t_tmp));
}
QString TStringPool::getStyleString(uint32_t _strIndex) const
{
	//安卓编译器会把所有带格式的字符串都放在前面，所以styles的数量会明显比strings少
	//如果要加一个带格式的字符串，则需要把所有普通字符串都后移，然后把所有引用的地方都修改
	Q_ASSERT(_strIndex < (uint32_t)m_strings.size());
	QString t_str = m_strings[_strIndex];
	if (_strIndex < (uint32_t)m_styles.size())
		t_str = encodeRichText(t_str, m_styles[_strIndex], *this);

	return t_str.replace(QChar(0x0A), QString("\\n"));
}
QString TStringPool::getString(uint32_t _strIndex) const
{
	Q_ASSERT(_strIndex < (uint32_t)m_strings.size());
	return m_strings[_strIndex];
}
uint32_t TStringPool::getStringIndex(const TRichString& _richStr) const
{
	if (m_strIndexs.contains(_richStr))
		return m_strIndexs[_richStr];
	return ResStringPool_ref::END;
}
uint32_t TStringPool::incReferenceCount(uint32_t _strIndex, int _inc)
{
	Q_ASSERT(_strIndex < (uint32_t)m_strings.size());
	Q_ASSERT(m_referenceCounts[_strIndex] + _inc > 0);
	m_referenceCounts[_strIndex] += _inc;
	return m_referenceCounts[_strIndex];
}
uint32_t TStringPool::insertString(const TRichString& _rich, uint32_t _addRefCount, bool* _isInsert, TTRAVERSAL_ALL_VALUE _traversalAllValue)
{
	uint32_t t_newIndex = 0;
	if (_rich.spans.size() > 0)
		t_newIndex = insertRichString(_rich, _isInsert, _traversalAllValue);
	else
		t_newIndex = insertSimpleString(_rich.str, _isInsert);
	m_referenceCounts[t_newIndex] += _addRefCount;
	qDebug() << "number of citations add " << _addRefCount << " chang to:" << m_referenceCounts[t_newIndex] << " :" << m_strings[t_newIndex];
	return t_newIndex;
}
uint32_t TStringPool::insertSimpleString(const QString& _str, bool* _isInsert)
{
	TRichString t_richStr;
	t_richStr.str = _str;
	if (m_strIndexs.contains(t_richStr))
	{
		if (_isInsert != NULL)
			*_isInsert = false;
		return m_strIndexs[t_richStr];
	}
	else
	{
		qDebug() << "insert simple string:" + _str;
		if (_isInsert != NULL)
			*_isInsert = true;
		m_strings.append(_str);
		m_referenceCounts.append(0);
		m_strIndexs.insert(t_richStr, m_stringPoolHeader.stringCount);
		//加在最后，所以返回的索引值就是原来的个数，并且因为加在最后，所以其他的索引都不需要改动
		return m_stringPoolHeader.stringCount++;
	}
}
uint32_t TStringPool::insertRichString(const TRichString& _rich, bool* _isInsert, TTRAVERSAL_ALL_VALUE _traversalAllValue)
{
	if (m_strIndexs.contains(_rich))
	{
		if (_isInsert != NULL)
			*_isInsert = false;
		return m_strIndexs[_rich];
	}
	else
	{
		//先遍历所有字符串引用，将所有简单字符串的编号+1，在富文本字符串的最后添加
		qDebug() << "insert rich string:" + _rich.str;
		if (_isInsert != NULL)
			*_isInsert = true;

		//插入数据
		uint32_t t_idx = m_styles.size();
		m_strings.insert(t_idx, _rich.str);
		m_referenceCounts.insert(t_idx, 0);
		m_styles.append(_rich.spans);

		for (QMap<TRichString, uint32_t>::iterator i = m_strIndexs.begin(); i != m_strIndexs.end(); ++i)
			if (i.value() >= t_idx)		//包括原来的t_idx，也得往后挪一个
				i.value()++;
		m_strIndexs.insert(_rich, t_idx);

		//遍历修改所有需要修改的引用数据
		_traversalAllValue(std::bind(&incIdxValue, std::placeholders::_1, std::placeholders::_2, m_styles.size() - 1, 1));

		m_stringPoolHeader.stringCount++;
		return m_stringPoolHeader.styleCount++;
	}
}
bool TStringPool::deleteString(uint32_t _index, bool _force, TTRAVERSAL_ALL_VALUE _traversalAllValue)
{
	//如果引用数大于1，并且不是强制删除，那减引用数，否则删字符串
	if (m_referenceCounts[_index] > 1 && !_force)
	{
		qDebug() << "number of citations --:" + m_strings[_index];
		m_referenceCounts[_index]--;
		return false;
	}
	else
	{
		TRichString t_richStr;
		t_richStr.str = m_strings[_index];
		qDebug() << "delete string:" + m_strings[_index];
		//删除实际字符串
		m_stringPoolHeader.stringCount--;
		m_strings.remove(_index, 1);
		//删除可能的格式表
		if (_index < (uint32_t)m_styles.size())
		{
			t_richStr.spans = m_styles[_index];
			m_styles.remove(_index, 1);
			m_stringPoolHeader.styleCount--;
		}
		//删除引用计数
		m_referenceCounts.remove(_index, 1);
		//修改字符串内容和索引的对应关系
		m_strIndexs.remove(t_richStr);
		for (QMap<TRichString, uint32_t>::iterator i = m_strIndexs.begin(); i != m_strIndexs.end(); ++i)
			if (i.value() > _index)
				i.value()--;
		//修改所有用到索引的地方，大于这个索引的，都减1
		_traversalAllValue(std::bind(&incIdxValue, std::placeholders::_1, std::placeholders::_2, _index + 1, -1));
		return true;
	}
}
uint32_t TStringPool::replaceString(uint32_t _oldIndex, const TRichString& _newRich, bool _force, TTRAVERSAL_ALL_VALUE _traversalAllValue)
{
	//引用数等于1的强制修改，等于不强制
	if (_force && m_referenceCounts[_oldIndex] == 1)
		_force = false;

	if (m_strIndexs.contains(_newRich) && m_strIndexs[_newRich] == _oldIndex)
	{
		qDebug() << "string no chang: " << _newRich.str;
		return _oldIndex;
	}

	if (m_referenceCounts[_oldIndex] == 1 || _force)
	{
		//对于修改，比较复杂，这个字符串是否是富文本如果变化，需要先删除旧字符串，然后添加一个新的，否则就直接修改就好了。
		if ((_newRich.spans.size() > 0) != (_oldIndex < (uint32_t)m_styles.size()))
		{
			//字符串是否是富文本有变化，添加一个新的，删除老的。
			uint32_t t_rCount = m_referenceCounts[_oldIndex];
			bool t_realInsert;	//是真的插入了新的，还是只是修改了引用数?
			uint32_t t_newIndex = insertString(_newRich, t_rCount, &t_realInsert, _traversalAllValue);
			//真的插入了，而且插入的在_oldIndex之前，_oldIndex需要往后挪一位
			if (t_realInsert && t_newIndex <= _oldIndex)
				_oldIndex++;

			//将所有指向旧的字符串的，都指向新的。
			int t_chgCount = 0;
			_traversalAllValue(std::bind(&changIdxValue, std::placeholders::_1, std::placeholders::_2, _oldIndex, t_newIndex, &t_chgCount));
			qDebug() << "find number of citations is " << t_chgCount << " :" << m_strings[_oldIndex];
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
			TRichString t_oldRichStr(m_strings[_oldIndex], _oldIndex < (uint32_t)m_styles.size() ? m_styles[_oldIndex] : TStringPoolSpans());

			//如果新字符串和某个其他字符串相等，那就删除旧的，相等的那个引用加1
			if (m_strIndexs.contains(_newRich))
			{
				//新指向的字符串加上引用数，然后把指向旧字符串的指向，都重定向到新字符串
				uint32_t t_rCount = m_referenceCounts[_oldIndex];
				uint32_t t_newIndex = m_strIndexs[_newRich];
				m_referenceCounts[t_newIndex] += t_rCount;
				qDebug() << "number of citations add " << t_rCount << " chang to " << m_referenceCounts[t_newIndex] << " :" << m_strings[t_newIndex];

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
				m_strIndexs.remove(t_oldRichStr);
				qDebug() << "index: " << _oldIndex << " str:" << m_strings[_oldIndex] << " chang to:" << _newRich.str;
				m_strings[_oldIndex] = _newRich.str;
				m_strIndexs.insert(_newRich, _oldIndex);
				if (_newRich.spans.size() > 0)
				{
					Q_ASSERT(_oldIndex < (uint32_t)m_styles.size());
					m_styles[_oldIndex] = _newRich.spans;
				}
				return _oldIndex;
			}
		}
	}
	else
	{
		//原来的字符串只是引用数减1，然后加新字符串
		m_referenceCounts[_oldIndex]--;
		qDebug() << "number of citations --:" + m_strings[_oldIndex];
		return insertString(_newRich, 1, NULL, _traversalAllValue);
	}
}
void TStringPool::initStringReferenceCount(TTRAVERSAL_ALL_VALUE _traversalAllValue)
{
	m_referenceCounts = QVector<int>(m_strings.size(), 0);

	_traversalAllValue(std::bind(&referenceCountInc, std::placeholders::_1, std::placeholders::_2, &m_referenceCounts));

	//如果一切正常，不应该有没用的字符串
	for (int i = 0; i < m_referenceCounts.size(); ++i)
	{
		Q_ASSERT(m_referenceCounts[i] != 0);
	}
}
void TStringPool::traversalAllValue(TRAVERSE_STRIDX_CALLBACK _callBack)
{
	for (int i = 0; i < m_styles.size(); ++i)
	{
		TStringPoolSpans& t_spans = m_styles[i];
		for (int j = 0; j < t_spans.size(); ++j)
			_callBack(Res_value::_DataType::TYPE_STRING, t_spans[j].name.index);
	}
	for (QMap<TRichString, uint32_t>::iterator i = m_strIndexs.begin(); i != m_strIndexs.end(); ++i)
	{
		//正常不应该这么搞，但是下面的修改并不会影响key的大小比较，所以应该没问题。
		TRichString& t_richString = const_cast<TRichString&>(i.key());
		for (int j = 0; j < t_richString.spans.size(); ++j)
		{
			_callBack(Res_value::_DataType::TYPE_STRING, t_richString.spans[j].name.index);
		}
	}
}
