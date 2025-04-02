//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor\src\TablePackageExtend.h
//	desc:		
//
//	created:	wangdaye 26:3:2025   12:26
//********************************************************************
#ifndef TablePackageExtend_h__
#define TablePackageExtend_h__
#include "ResArscStruct.h"
#include "StringPoolExtend.h"

struct TTableTypeData	//一个资源类
{
	TTableTypeSpecEx typeSpec;	//包括这个类的一些基础数据
	QVector<TTableTypeEx> types;	//和多个分组的具体数据
};
enum ETreeItemType
{
	eTreeItemType_package,
	eTreeItemType_type,
	eTreeItemType_spec
};
class QPublicFinal;
typedef	std::function< void(const QString& _packageName, ETreeItemType _type, uint32_t _id1, uint32_t _id2, const QString& _name) > TRAVERSE_PACKAGE_DATA_CALLBACK;

class TTablePackage
{
private:
	QMap<uint, TTableTypeData> m_tableTypeDatas;
	ResTable_package m_packageInfo;
	TStringPool m_typeStringPool;
	TStringPool m_keyStringPool;
	TStringPool* m_publicStringPool;
	QPublicFinal* m_publicFinal;
public:
	TTablePackage();
	void reset(void);
	void setPublicData(TStringPool* _strStringPool, QPublicFinal* _publicFinal);
	void readTablePackage(const char* _pBegin);
	void writeTablePackage(QByteArray& _buff) const;
	//遍历所有的资源类及资源分组，但并不遍历具体的数据
	void traversalData(TRAVERSE_PACKAGE_DATA_CALLBACK _callBack) const;
	//遍历所有的资源数据
	void traversalAllValue(TRAVERSE_STRIDX_CALLBACK _callBack) const;
	const QVector<uint32_t>& getTableTypeMask(uint32_t _typeid) const;
	const TTableTypeEx& getTableType(uint32_t _typeid, uint32_t _specid) const;
	const TTableTypeEx* getTableType(uint32_t _typeid, const ResTable_config& _config) const;
	QString getKeyString(int _index) const;
	QString getTypeString(int _index) const;
	//获得引用的目标
	QString getReferenceDestination(const ResTable_config& _config, qint32 _data) const;
	QString resValue2String(const Res_value& _value) const;
	void addLocale(uint32_t _typeid, const ResTable_config& _config, TRAVERSE_PACKAGE_DATA_CALLBACK _callBack);
	void copyValue(uint32_t _typeid, uint32_t _specid, uint32_t _id);
	void deleteValue(uint32_t _typeid, uint32_t _specid, uint32_t _id, TTRAVERSAL_ALL_VALUE _traversalAllValue);
private:
	void readTableTypeSpec(TTableTypeSpecEx& _span, const char* _pBuff);
	void readTableType(TTableTypeEx& _type, const char* _pBegin);
	void readTableMapEntry(TTableMapEntry& _map, const char* _pBuff);

	void writeTableTypeSpec(QByteArray& _buff, const TTableTypeSpecEx& _span) const;
	void writeTableType(QByteArray& _buff, const TTableTypeEx& _type) const;
	void writeTableMapEntry(QByteArray& _buff, const TTableMapEntry* _mapEntry) const;
};
#endif // TablePackageExtend_h__