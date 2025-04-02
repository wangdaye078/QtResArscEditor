//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor\src\StringPoolExtend.h
//	desc:		
//
//	created:	wangdaye 25:3:2025   8:52
//********************************************************************
#ifndef StringPoolExtend_h__
#define StringPoolExtend_h__
#include "ResArscStruct.h"

template<typename T>
T readValue(const char*& _buff)
{
	T t = *reinterpret_cast<const T*>(_buff);
	_buff += sizeof(T);
	return t;
}
typedef	std::function< void(Res_value::_DataType, uint32_t&) > TRAVERSE_STRIDX_CALLBACK;
typedef	std::function< void(TRAVERSE_STRIDX_CALLBACK) > TTRAVERSAL_ALL_VALUE;

class TStringPool
{
private:
	ResStringPool_header m_stringPoolHeader;
	QVector<TStringPoolSpans> m_styles;
	QVector<QString> m_strings;

	QVector<int> m_referenceCounts;
	QMap<TRichString, uint32_t> m_strIndexs;
public:
	TStringPool();
	void reset(void);
	void readStrPool(const char* _pBegin);
	void writeStrPool(QByteArray& _buff) const;
	QString getStyleString(uint32_t _strIndex) const;
	QString getString(uint32_t _strIndex) const;
	uint32_t getStringIndex(const TRichString& _richStr) const;
	uint32_t incReferenceCount(uint32_t _strIndex, int _inc);
	uint32_t replaceString(uint32_t _oldIndex, const TRichString& _newRich, bool _force, TTRAVERSAL_ALL_VALUE _traversalAllValue);
	uint32_t insertString(const TRichString& _rich, uint32_t _addRefCount, bool* _isInsert, TTRAVERSAL_ALL_VALUE _traversalAllValue);
	//返回是否真的删除了。
	bool deleteString(uint32_t _index, bool _force, TTRAVERSAL_ALL_VALUE _traversalAllValue);
	void initStringReferenceCount(TTRAVERSAL_ALL_VALUE _traversalAllValue);
	void traversalAllValue(TRAVERSE_STRIDX_CALLBACK _callBack);
private:
	void makeIndexs(void);
	static void readPoolSpan(TStringPoolSpans& _span, const char* _pBuff);
	static void writePoolSpan(QByteArray& _buff, const TStringPoolSpans& _span);
	//返回指定字符串的索引值，如果是插入新字符串，新字符串的引用计数为0，如果是旧字符串，则引用计数不变，都需要在后续处理
	uint32_t insertSimpleString(const QString& _str, bool* _isInsert);
	uint32_t insertRichString(const TRichString& _rich, bool* _isInsert, TTRAVERSAL_ALL_VALUE _traversalAllValue);
};

extern QString encodeRichText(const QString& _input, const TStringPoolSpans& _spans, const TStringPool& _stringPool);
extern TRichString decodeRichText(const QString& _input, const TStringPool& _stringPool);
#endif // StringPoolExtend_h__
