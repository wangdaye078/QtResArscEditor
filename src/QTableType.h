//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor2\src\QTableType.h
//	desc:		
//
//	created:	wangdaye 24:6:2025   15:36
//********************************************************************
#ifndef QTableType_h__
#define QTableType_h__
#include <QObject>
#include <QVector>
#include <QSharedPointer>
#include "ResArscStruct.h"
#include "QStringPool.h"
#include <QVariant>


enum ETreeItemType
{
	eTreeItemType_package,
	eTreeItemType_type,
	eTreeItemType_spec
};
enum EValueItemType
{
	eValueItemType_array,
	eValueItemType_arrayitem,
	eValueItemType_value
};
enum EValueItemRole
{
	eValueItemRole_type = Qt::UserRole,		//是什么节点，数据，数组还是数组元素
	eValueItemRole_datatype,
	eValueItemRole_data,
	eValueItemRole_id,
	eValueItemRole_parentid,
	eValueItemRole_entry,
	eValueItemRole_package,
};

typedef	std::function< void(const QString& _packageName, ETreeItemType _type, uint32_t _typeId, const QString& _name, const QVariant& _v) > TRAVERSE_PACKAGE_DATA_CALLBACK;

class QTablePackage;
typedef QSharedPointer<QTablePackage> PTablePackage;

struct IResValue
{
	virtual ~IResValue() {};
	virtual uint32_t readBuff(const char* _buff) = 0;
	virtual void writeBuff(QTablePackage* _tablePackage, QByteArray& _buff) = 0;
	virtual EValueItemType getValueType(void) = 0;
	virtual PArscRichString getValue(Res_value** _value) = 0;
	virtual void setValue(const Res_value& _value) = 0;
	virtual uint32_t getKeyIndex(void) = 0;
	virtual IResValue* clone() const = 0;	//克隆一个新的对象，必须实现这个函数
};
typedef QSharedPointer<IResValue> PResValue;
Q_DECLARE_METATYPE(PResValue)

struct TTableValueEntry : public IResValue		//普通的值，一个名字对于一个值
{
	ResTable_entry entry;
	Res_value value;
	//
	PArscRichString svalue;
	~TTableValueEntry() override { svalue.reset(); };
	uint32_t readBuff(const char* _buff) override;
	void writeBuff(QTablePackage* _tablePackage, QByteArray& _buff) override;
	EValueItemType getValueType(void) override { return  eValueItemType_value; }
	PArscRichString getValue(Res_value** _value) override
	{
		if (_value != NULL)
			*_value = &value;
		return svalue;
	}
	void setValue(const Res_value& _value) override;
	uint32_t getKeyIndex(void) override
	{
		return entry.key.index;
	}
	IResValue* clone() const override
	{
		TTableValueEntry* t_clone = new TTableValueEntry();
		t_clone->entry = entry;
		t_clone->value = value;
		t_clone->svalue = svalue;
		return t_clone;
	}
};
struct ResTable_pairs : public IResValue
{
	ResTable_ref key;
	Res_value value;
	//
	PArscRichString svalue;
	~ResTable_pairs() override { svalue.reset(); };
	uint32_t readBuff(const char* _buff) override;
	void writeBuff(QTablePackage* _tablePackage, QByteArray& _buff) override;
	EValueItemType getValueType(void) override { return  eValueItemType_arrayitem; }
	PArscRichString getValue(Res_value** _value) override
	{
		if (_value != NULL)
			*_value = &value;
		return svalue;
	}
	void setValue(const Res_value& _value) override;
	uint32_t getKeyIndex(void) override
	{
		return key.ident;
	}
	IResValue* clone() const override
	{
		ResTable_pairs* t_clone = new ResTable_pairs();
		t_clone->key = key;
		t_clone->value = value;
		t_clone->svalue = svalue;
		return t_clone;
	}
};
struct TTableMapEntry : public IResValue	//数组，一个名字后面还有多个key和值的对
{
	ResTable_map_entry entry;
	QVector<PResValue> pairs;
	//
	~TTableMapEntry() override {};
	uint32_t readBuff(const char* _buff) override;
	void writeBuff(QTablePackage* _tablePackage, QByteArray& _buff) override;
	EValueItemType getValueType(void) override { return  eValueItemType_array; }
	PArscRichString getValue(Res_value** _value) override
	{
		if (_value != NULL)
			*_value = NULL;
		return PArscRichString();
	}
	void setValue(const Res_value& _value) override { Q_ASSERT(false); };
	uint32_t getKeyIndex(void) override
	{
		return entry.key.index;
	}
	IResValue* clone() const override
	{
		TTableMapEntry* t_clone = new TTableMapEntry();
		t_clone->entry = entry;
		t_clone->pairs.resize(pairs.size());
		for (int i = 0; i < pairs.size(); ++i)
			t_clone->pairs[i] = QSharedPointer<IResValue>(pairs[i]->clone());
		return t_clone;
	}
};

class QTreeWidgetItem;
typedef	std::function< QTreeWidgetItem* (QTreeWidgetItem* _parent, uint32_t _idx, EValueItemType _type, const QVariant& _v) > TRAVERSE_SPECIFIC_DATA_CALLBACK;

class TSpecificData
{
	//针对某个适配的具体数据，比如针对某个分辨率，或者某个语言
public:
	TSpecificData();
	TSpecificData(const TSpecificData&) = delete;
	TSpecificData& operator=(const TSpecificData&) = delete;
	~TSpecificData();
	void readBuff(const char* _buff);
	void writeBuff(QTablePackage* _tablePackage, QByteArray& _buff);
	const ResTable_type& getType() const;
	void traversalData(TRAVERSE_SPECIFIC_DATA_CALLBACK _callBack) const;
	void deleteValue(uint32_t _id);
	ResTable_type& getTType(void);
	void copyValue(const TSpecificData& _other, uint32_t _idx);
	uint32_t getEntryCount(void) const;
	void setEntryCount(uint32_t _count);
	PResValue getEntry(uint32_t _idx);
private:
	ResTable_type m_tableType;
	QVector<PResValue> m_entryValue;
};
typedef QSharedPointer<TSpecificData> PSpecificData;
Q_DECLARE_METATYPE(PSpecificData)

class QTableType : public QObject
{
	//一个资源类的数据，比如anim,array,string等
public:
	QTableType(QObject* parent = NULL);
	QTableType(const QTableType&) = delete;
	QTableType& operator=(const QTableType&) = delete;
	~QTableType();
	void readBuff_head(const char* _buff);
	void readBuff_specData(const char* _buff);
	void writeBuff(QTablePackage* _tablePackage, QByteArray& _buff);
	//遍历所有的资源类及资源分组，但并不遍历具体的数据
	void traversalData(TRAVERSE_PACKAGE_DATA_CALLBACK _callBack, const QString& _packageName, uint32_t _typeId, const QString& _typeName) const;
	uint32_t getKeyIndex(uint32_t _id);
	const QVector<uint32_t>& getConfigMask(void);
	PSpecificData getDefaultSpec(void);
	PSpecificData addLocale(const ResTable_config& _config);
private:
	QTreeWidgetItem* onRefreshSpecificData(QTreeWidgetItem* _parent, uint32_t _idx, EValueItemType _type, const QVariant& _v);
private:
	ResTable_typeSpec m_typeSpec;	//资源类的说明
	QVector<uint32_t> m_configmask;
	QVector<PSpecificData> m_SpecDatas;
	//ID到key的转换，因为不是每个适配里都有全部的ID，default也可能缺少部分，为了保证能快速找到，所以建这个表
	QMap<uint32_t, uint32_t> m_id_key;
};
typedef QSharedPointer<QTableType> PTableType;

extern QString resValue2String(const Res_value& _value, const PArscRichString& _svalue);
#endif // QTableType_h__
