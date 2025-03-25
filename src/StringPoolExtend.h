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

struct TStringPool
{
public:
	ResStringPool_header StringPoolHeader;
	QVector<QString> strings;
	QVector<TStringPoolSpans> styles;

	QMap<TRichString, uint32_t> strIndexs;
	QVector<int> referenceCount;
public:
	void reset(void);
	void readStrPool(const char* _pBegin);
	void writeStrPool(QByteArray& _buff) const;
	quint32 replaceString(quint32 _oldIndex, const QString& _str, bool _force, TTRAVERSAL_ALL_VALUE _traversalAllValue);
	quint32 insertString(const QString& _str, uint32_t _addRefCount, bool* _isInsert, TTRAVERSAL_ALL_VALUE _traversalAllValue);
	//返回是否真的删除了。
	bool deleteString(quint32 _index, bool _force, TTRAVERSAL_ALL_VALUE _traversalAllValue);
	void initStringReferenceCount(TTRAVERSAL_ALL_VALUE _traversalAllValue);
private:
	void makeIndexs(void);
	static void readPoolSpan(TStringPoolSpans& _span, const char* _pBuff);
	static void writePoolSpan(QByteArray& _buff, const TStringPoolSpans& _span);
public:
	//返回指定字符串的索引值，如果是插入新字符串，新字符串的引用计数为0，如果是旧字符串，则引用计数不变，都需要在后续处理
	quint32 insertSimpleString(const QString& _str, bool* _isInsert);
	quint32 insertRichString(const QString& _str, TStringPoolSpans& _span, bool* _isInsert, TTRAVERSAL_ALL_VALUE _traversalAllValue);
};

extern void encodeRichText(QString& _input, const TStringPoolSpans& _spanEx, const TStringPool& _stringPool);
extern void decodeRichText(QString& _input, TStringPoolSpans& _spanEx, const TStringPool& _stringPool, int _endPos = -1);

#endif // StringPoolExtend_h__
