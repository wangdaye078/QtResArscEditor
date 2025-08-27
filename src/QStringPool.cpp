#include "QArscString.h"
#include "QStringPool.h"
#include <QDebug>
#include <QVector>

bool TArscStringStyle::operator<(const TArscStringStyle& _other) const
{
	if (ref->guid != _other.ref->guid)
		return ref->guid < _other.ref->guid;
	if (firstChar != _other.firstChar)
		return firstChar < _other.firstChar;
	if (lastChar != _other.lastChar)
		return lastChar < _other.lastChar;
	return false;	//相等
}

bool TArscStringStyle::operator==(const TArscStringStyle& _other) const
{
	return ref->guid == _other.ref->guid && firstChar == _other.firstChar && lastChar == _other.lastChar;
}

bool TArscRichString::operator<(const TArscRichString& _other) const
{
	if (string != _other.string)
		return string < _other.string;
	//Qt5中的QVector的operator==的写法不兼容C++20，所以不用==和!=，只能用<来比较
	//如果一定要使用，需要在项目中定义_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING或者_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS
	//Qt6中已经修正
	//if (styles != _other.styles)
	//	return styles < _other.styles;
	if (styles < _other.styles)
		return true;
	if (styles > _other.styles)
		return false;
	return false;	//相等
}


QStringPool* g_publicStrPool;

QStringPool::QStringPool(bool _removeUnuse, QObject* _parent)
	: QObject(_parent), m_removeUnuse(_removeUnuse), m_stringPoolHeader()
	, m_strings(ArscRichStringLessThanFuncWrapper(&styleFirstLessThan))
	, m_string_to_guid(ArscRichStringLessThanFuncWrapper(&strongLessThan))
	//m_strings(styleFirstLessThan), m_string_to_guid(strongLessThan)
{
}

QStringPool::~QStringPool()
{
}
void QStringPool::reset(void)
{
	m_GuidFactory.reset();
	m_strings.clear();
	m_guid_to_string.clear();
	m_string_to_guid.clear();
}
void QStringPool::readBuff(const char* _buff)
{
	reset();
	m_stringPoolHeader = *reinterpret_cast<const ResStringPool_header*>(_buff);
	Q_ASSERT(m_stringPoolHeader.header.type == RES_TYPE::RES_STRING_POOL_TYPE);
	const char* t_pBuff = _buff + sizeof(m_stringPoolHeader);

	QVector<uint32_t> t_stringsOffsets(m_stringPoolHeader.stringCount);
	memcpy(t_stringsOffsets.data(), t_pBuff, m_stringPoolHeader.stringCount * sizeof(uint32_t));
	t_pBuff += m_stringPoolHeader.stringCount * sizeof(uint32_t);
	QVector<uint32_t> t_stylesOffsets(m_stringPoolHeader.styleCount);
	memcpy(t_stylesOffsets.data(), t_pBuff, m_stringPoolHeader.styleCount * sizeof(uint32_t));

	//理论上，每个字符串都可以有style，如果style为空，则style结构就剩结尾的FFFFFFFF标记，如果所有空的style都保留，
	//就会有很多FFFFFFFF，所以为了节约空间，把style不为空的都排前面，最后style以2个FFFFFFFF为整个的结束标记，
	//styleCount需要和结束标记所代表的个数相等。
	bool t_isUTF8 = (m_stringPoolHeader.flags & ResStringPool_header::UTF8_FLAG) != 0;

	QVector<PArscRichString> t_strings(m_stringPoolHeader.stringCount);
	const char* t_pStringsBuff = _buff + m_stringPoolHeader.stringsStart;
	for (uint32_t i = 0; i < m_stringPoolHeader.stringCount; i++)
	{
		PArscRichString t_pArscRichString(new TArscRichString());
		t_pArscRichString->guid = m_GuidFactory.getNewGuid();
		t_pArscRichString->string = readString(t_pStringsBuff + t_stringsOffsets[i], t_isUTF8);
		t_strings[i] = t_pArscRichString;
	}
	const char* t_pStylesBuff = _buff + m_stringPoolHeader.stylesStart;
	for (uint32_t i = 0; i < m_stringPoolHeader.styleCount; i++)
	{
		const char* t_pStyleBuff = t_pStylesBuff + t_stylesOffsets[i];
		PArscRichString t_pArscRichString = t_strings[i];
		while (*reinterpret_cast<const uint32_t*>(t_pStyleBuff) != ResStringPool_span::END)
		{
			ResStringPool_span t_span = *reinterpret_cast<const ResStringPool_span*>(t_pStyleBuff);
			TArscStringStyle t_style;
			t_style.ref = t_strings[t_span.name.index];
			t_style.firstChar = t_span.firstChar;
			t_style.lastChar = t_span.lastChar;
			t_strings[i]->styles.append(t_style);
			t_pStyleBuff += sizeof(ResStringPool_span);
		}
	}
	for (QVector<PArscRichString>::iterator i = t_strings.begin(); i != t_strings.end(); ++i)
	{
		m_strings.insert(ArscRichStringMap::value_type(*i, (*i)->guid));
		m_guid_to_string.insert((*i)->guid, *i);
		//理论上，同样的字符串不应该重复出现。
		Q_ASSERT(m_string_to_guid.find(*i) == m_string_to_guid.end());
		m_string_to_guid.insert(ArscRichStringMap::value_type(*i, (*i)->guid));
	}
	t_strings.clear();
}
void QStringPool::writeBuff(QByteArray& _buff)
{
	if (m_removeUnuse)
		removeUnusedString();

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
	uint32_t t_stringsBegin_pos = _buff.size();
	Q_ASSERT(m_stringPoolHeader.stringCount == m_strings.size());
	uint32_t t_idx = 0;
	for (ArscRichStringMap::iterator i = m_strings.begin(); i != m_strings.end(); ++i)
	{
		reinterpret_cast<uint32_t*>(_buff.data() + t_stringOffsets_pos)[t_idx] = _buff.size() - t_stringsBegin_pos;
		writeString(_buff, i->first->string, t_isUTF8);
		//不设置index也可以，用size_type rank(key_type const& key)也可得到等价的index，但是每次计算可能会慢那么一点
		i->first->index = t_idx++;
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
		uint32_t t_styleCount = 0;
		uint32_t t_SpanEnd[2] = { (uint32_t)ResStringPool_span::END, (uint32_t)ResStringPool_span::END };
		for (ArscRichStringMap::iterator i = m_strings.begin(); i != m_strings.end(); ++i)
		{
			PArscRichString t_pArscRichString = i->first;
			//根据styleFirstLessThan的排序规则，有styles的都排在前面，所以发现这个styles为空，那后面的所有styles都为空
			if (t_pArscRichString->styles.size() == 0) break;
			reinterpret_cast<uint32_t*>(_buff.data() + t_styleOffsets_pos)[t_styleCount] = _buff.size() - t_stylesBegin_pos;
			for (int j = 0; j < t_pArscRichString->styles.size(); ++j)
			{
				ResStringPool_span t_span;
				//等价，但是用rank可能会慢一点
				//t_span.name.index = (uint32_t)m_strings.rank(t_pArscRichString->styles[j].ref);
				t_span.name.index = t_pArscRichString->styles[j].ref->index;
				t_span.firstChar = t_pArscRichString->styles[j].firstChar;
				t_span.lastChar = t_pArscRichString->styles[j].lastChar;
				_buff.append(reinterpret_cast<const char*>(&t_span), sizeof(ResStringPool_span));
			}
			//每一组PoolSpan以1个0xFFFFFFFF结束
			_buff.append(reinterpret_cast<const char*>(t_SpanEnd), sizeof(uint32_t));
			t_styleCount++;
		}
		Q_ASSERT(m_stringPoolHeader.styleCount == t_styleCount);
		//所有PoolSpan后再以2个0xFFFFFFFF结束整个段
		_buff.append(reinterpret_cast<char*>(t_SpanEnd), sizeof(uint32_t) * 2);
	}
	reinterpret_cast<ResTable_package*>(_buff.data() + t_stringPoolHeader_pos)->header.size = _buff.size() - t_stringPoolHeader_pos;
}

bool QStringPool::styleFirstLessThan(const PArscRichString& _p1, const PArscRichString& _p2)
{
	//都是0或者都不是0的情况，就按guid，小的排前面，原始的arsc文件里面，字符串也没有按编码排序，所以我们也就不用了。
	//而且对于typeStringPool和keyStringPool来说，以当前代码的逻辑，他们的顺序不能改变，所以也不能按string的内容排序，只能使用guid来排序。
	if ((_p1->styles.size() == 0 && _p2->styles.size() == 0) || (_p1->styles.size() > 0 && _p2->styles.size() > 0))
	{
		return _p1->guid < _p2->guid;
	}
	//一个是0一个不是0的时候，不是0的排前面
	return _p1->styles.size() > _p2->styles.size();
}
bool QStringPool::strongLessThan(const PArscRichString& _p1, const PArscRichString& _p2)
{
	return *(_p1.get()) < *(_p2.get());
}
PArscRichString QStringPool::getGuidRef(uint32_t _guid) const
{
	QMap<int, PArscRichString>::const_iterator t_pIter = m_guid_to_string.find(_guid);
	return (t_pIter == m_guid_to_string.end()) ? PArscRichString() : t_pIter.value();
}
uint32_t QStringPool::getRefIndex(const PArscRichString& _s) const
{
	return (uint32_t)m_strings.rank(_s);
}
uint32_t QStringPool::getRefCount(PArscRichString _rich)
{
	ArscRichStringMap::iterator t_iter = m_string_to_guid.find(_rich);
	if (t_iter == m_string_to_guid.end())
		return 0;
	else
		return (uint32_t)m_guid_to_string[t_iter->second].use_count();
}
PArscRichString QStringPool::getRichString(PArscRichString _rich)
{
	ArscRichStringMap::iterator t_iter = m_string_to_guid.find(_rich);
	if (t_iter == m_string_to_guid.end())
	{
		_rich->guid = m_GuidFactory.getNewGuid();
		m_strings.insert(ArscRichStringMap::value_type(_rich, _rich->guid));
		m_guid_to_string.insert(_rich->guid, _rich);
		m_string_to_guid.insert(ArscRichStringMap::value_type(_rich, _rich->guid));
		qDebug() << "add string " << _rich->string;
		m_stringPoolHeader.stringCount++;
		if (_rich->styles.size() > 0)
			m_stringPoolHeader.styleCount++;
		return _rich;
	}
	else
	{
		return m_guid_to_string[t_iter->second];
	}
}
void QStringPool::insertRichString(PArscRichString _rich)
{
	m_strings.insert(ArscRichStringMap::value_type(_rich, _rich->guid));
	m_guid_to_string.insert(_rich->guid, _rich);
	m_string_to_guid.insert(ArscRichStringMap::value_type(_rich, _rich->guid));
	m_stringPoolHeader.stringCount++;
	if (_rich->styles.size() > 0)
		m_stringPoolHeader.styleCount++;
}
void QStringPool::removeRichString(PArscRichString _rich)
{
	m_guid_to_string.remove(_rich->guid);
	m_strings.erase(_rich);
	m_string_to_guid.erase(_rich);
	m_stringPoolHeader.stringCount--;
	if (_rich->styles.size() > 0)
		m_stringPoolHeader.styleCount--;
}
void QStringPool::removeUnusedString(void)
{
	int t_idx = 0;
	for (ArscRichStringMap::iterator i = m_strings.begin(); i != m_strings.end();)
	{
		if (i->first.use_count() > LEAST_REF_COUNT)
		{
			++i;
		}
		else
		{
			qDebug() << "remove string " << t_idx++ << " " << i->first->string;
			m_stringPoolHeader.stringCount--;
			if (i->first->styles.size() > 0)
				m_stringPoolHeader.styleCount--;
			m_guid_to_string.remove(i->first->guid);
			m_string_to_guid.erase(i->first);
			i = m_strings.erase(i);
		}
	}
}
void QStringPool::printRefCount(void)
{
	qDebug() << QString("style string count: %1").arg(m_stringPoolHeader.styleCount, 8, 16, QChar('0'));
	uint32_t t_id = 0;
	for (ArscRichStringMap::iterator i = m_strings.begin(); i != m_strings.end(); ++i)
	{
		qDebug() << QString("0x%1").arg(t_id++, 8, 16, QChar('0')) << " " << i->first.use_count() << " " << i->first->string << " ";
	}

}
