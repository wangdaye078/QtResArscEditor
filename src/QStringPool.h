//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor2\src\QStringPool.h
//	desc:		
//
//	created:	wangdaye 22:6:2025   19:19
//********************************************************************
#ifndef QStringPool_h__
#define QStringPool_h__
#include "GuidFactory.h"
#include "ResArscStruct.h"
#include <QList>
#include <QMap>
#include <QObject>

//#include "common/bpptree_map.h"
#include "common/sbtree_map.h"
#include "common/shared_ptr.h"
struct TArscRichString;
using PArscRichString = SRombauts::shared_ptr<TArscRichString>;

struct TArscStringStyle
{
	PArscRichString ref;
	uint32_t firstChar;
	uint32_t lastChar;
	bool operator<(const TArscStringStyle& _other) const;
	bool operator==(const TArscStringStyle& _other) const;
	TArscStringStyle() :firstChar(0), lastChar(0) {};
};
struct TArscRichString
{
	uint32_t guid;			//唯一标识
	uint32_t index;		//平时不用，只有写文件的时候会重新排序写入使用
	QString string;		//文本
	uint32_t resId;		//资源ID，一般用于android:开头的属性ID，如果是app:开头的属性ID，得对照resources.arsc才能知道
	QVector<TArscStringStyle> styles;		//富文本标识
	bool operator<(const TArscRichString& _other) const;
	TArscRichString() :guid(ResStringPool_ref::END), index(0), resId(0) {};
};

//QMap不支持这样写，
//using ArscRichStringMap = std::map<PArscRichString, int, std::function<bool(const PArscRichString&, const PArscRichString&)>>;
// 好像有BUG，会导致智能指针的引用数不正常
//using ArscRichStringMap = bpptree_map <PArscRichString, int, std::function<bool(const PArscRichString&, const PArscRichString&)>>;

using ArscRichStringLessThanFunc = bool(*)(const PArscRichString&, const PArscRichString&, bool _stringSorting);
struct ArscRichStringLessThanFuncWrapper
{
	ArscRichStringLessThanFunc func;
	bool m_stringSorting;
	ArscRichStringLessThanFuncWrapper(ArscRichStringLessThanFunc _func, bool _stringSorting) : func(_func), m_stringSorting(_stringSorting) {}
	bool operator()(const PArscRichString& _p1, const PArscRichString& _p2) const
	{
		return func(_p1, _p2, m_stringSorting);
	}
};
using ArscRichStringMap = sbtree_multimap <PArscRichString, int, ArscRichStringLessThanFuncWrapper>;
//用仿函数比使用std::function要快很多，
//using ArscRichStringMap = sbtree_multimap <PArscRichString, int, std::function<bool(const PArscRichString&, const PArscRichString&)>>;
class QStringPool : public QObject
{
public:
	QStringPool(const QString& _name = "", bool _removeUnuse = false, bool _stringSorting = false, QObject* _parent = NULL);
	QStringPool(const QStringPool&) = delete;
	QStringPool& operator=(const QStringPool&) = delete;
	~QStringPool();
	void reset(void);
	void readBuff(const char* _buff, const QVector<uint32_t>& _resIDs);
	void writeBuff(QByteArray& _buff);
	PArscRichString getGuidRef(uint32_t _guid) const;
	uint32_t getRefIndex(const PArscRichString& _s) const;
	PArscRichString getIndexRef(uint32_t _index) const;
	//_rich里面不要求guid，只判决实际内容
	uint32_t getRefCount(PArscRichString _rich);
	//_rich里面没有guid，根据_rich.string和_rich.styles获得字符串池里面的PArscRichString，没有就加一个
	PArscRichString getRichString(PArscRichString _rich);
	//需要保证_rich里面的guid已经设置
	void insertRichString(PArscRichString _rich);
	//需要保证_rich里面的guid已经设置
	void removeRichString(PArscRichString _rich);
	const QVector<uint32_t>& getResIDs(void) const;
	void printRefCount(void);
private:
	//排序函数，让有格式的字符串排前面，是否有格式一样的，guid小的排前面
	static bool styleFirstLessThan(const PArscRichString& _p1, const PArscRichString& _p2, bool _stringSorting);
	//排序函数，并不比较guid的值，而是比较具体的字符串内容和格式，保证相同的格式字符串只出现一次
	static bool strongLessThan(const PArscRichString& _p1, const PArscRichString& _p2, bool _stringSorting);
	//删除掉没人引用的字符串
	void removeUnusedString(void);
private:
public:
	QString m_name;
	GuidFactory m_GuidFactory;
	bool m_removeUnuse;		//回写的时候删除未使用的
	ResStringPool_header m_stringPoolHeader;
	//使用styleFirstLessThan排序的map
	ArscRichStringMap m_strings;
	QMap<int, PArscRichString> m_guid_to_string;
	//使用strongLessThan排序的map
	ArscRichStringMap m_string_to_guid;
	QMap<int, int> m_repeat_string_guid_map;
	QVector<uint32_t> m_resIDs;	//只在writeBuff后才有效
};

//最小引用次数，因为上面有3个map，所以有3次引用，如果等于这个值，就可以释放掉了。
constexpr auto LEAST_REF_COUNT = (3);
//一个文件里面，公用的字符串池只有一个，所以用全局变量来保存。
//问题是如果需要同时打开多个文件的时候，这个变量就会冲突。但是真的很难去掉这个全局变量。
extern QStringPool* g_publicStrPool;

#endif // QStringPool_h__
