//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor\src\QResArscParser.h
//	desc:		
//
//	created:	wangdaye 14:3:2025   20:46
//********************************************************************
#ifndef QResArscParser_h__
#define QResArscParser_h__
#include <QObject>
#include "StringPoolExtend.h"

class QPublicFinal;

struct TTableTypeData	//一个资源类
{
	TTableTypeSpecEx typeSpec;	//包括这个类的一些基础数据
	QVector<TTableTypeEx> typeDatas;	//和多个分组的具体数据
};

class QResArscParser : public QObject
{
	Q_OBJECT
public:
	QResArscParser(QObject* parent);
	~QResArscParser();
	void reset(void);
	void readFile(const QString& _path);
	void readBuff(const QByteArray& _buff);
	bool writeFile(const QString& _path);
	void writeBuff(QByteArray& _buff);

	const ResTable_package& tablePackage(void);
	const TStringPool& typeString(void);
	const QMap<uint, TTableTypeData>& tableTypeDatas(void);
	QString keyString(qint32 _key);
	QString resValue2String(const Res_value& _value);
	TTableTypeEx& getTableType(uint32_t _typeid, uint32_t _specid);
	//获得引用的目标
	QString getReferenceDestination(const ResTable_config& _config, qint32 _data);
	//将_typeid下面默认组的_id数据，拷贝一份到_specid组（先复制一份默认数据，然后再修改）
	void addLocale(uint32_t _typeid, const ResTable_config& _config);
	void copyValue(uint32_t _typeid, uint32_t _specid, uint32_t _id);
	void deleteValue(uint32_t _typeid, uint32_t _specid, uint32_t _id);
	//设置值包括是普通数据还是数组，是字符串还是其他
	void setValue(uint32_t _typeid, uint32_t _specid, uint32_t _id, Res_value::_DataType _dataYype, uint32_t _data);
	void setValue(uint32_t _typeid, uint32_t _specid, uint32_t _id, uint32_t _idx, Res_value::_DataType _dataYype, uint32_t _data);
	void setValue(uint32_t _typeid, uint32_t _specid, uint32_t _id, const QString& _data, bool _force);
	void setValue(uint32_t _typeid, uint32_t _specid, uint32_t _id, uint32_t _idx, const QString& _data, bool _force);
	//获得这个字符串的引用数
	quint32 getReferenceCount(quint32 _index);
private:
	QSharedPointer<ResTable_entry>& getValue(uint32_t _typeid, uint32_t _specid, uint32_t _id);
	QSharedPointer<ResTable_entry>& getValueOrInsert(uint32_t _typeid, uint32_t _specid, uint32_t _id);

	//返回这个字符串的新index
	quint32 replaceString(quint32 _oldIdx, const QString& _str, bool _force = false);  //_force，当引用数大于1时也修改该字符串，而不是重新生成一个，会导致所有使用该字符串的都改变
	//用于遍历所有值，可用来修改字符串引用，查找某个值是否被使用等
	void traversalAllValue(TRAVERSE_STRIDX_CALLBACK _callBack);
private:
	void writeTablePackage(QByteArray& _buff);
	void writeTableTypeSpec(QByteArray& _buff, const TTableTypeSpecEx& _span);
	void writeTableType(QByteArray& _buff, const TTableTypeEx& _type);
	void writeTableMapEntry(QByteArray& _buff, const TTableMapEntryEx* _mapEntry);

	uint readTablePackage(const char* _pBegin);
	void readTableTypeSpec(TTableTypeSpecEx& _span, const char* _pBuff);
	void readTableType(TTableTypeEx& _type, const char* _pBegin);
	TTableMapEntryEx* readTableMapEntry(TTableMapEntryEx& _map, const char* _pBuff);

	QString getStyleString(qint32 _strIndex);

private:
	TStringPool m_StringPool;

	ResTable_package m_tablePackage;
	TStringPool m_typeStringPool;
	TStringPool m_keyStringPool;
	QMap<uint, TTableTypeData> m_tableTypeDatas;

	QPublicFinal* m_publicFinal;
};

Q_DECLARE_METATYPE(ResTable_config)

#endif // QResArscParser_h__
