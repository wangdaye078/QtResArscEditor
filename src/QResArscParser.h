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
#include "TablePackageExtend.h"

class QPublicFinal;

typedef	std::function< void(const QString&, const TTablePackage&) > TTRAVERSAL_ALL_TABLEPACKAGE;


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
	void traversalAllTablePackage(TTRAVERSAL_ALL_TABLEPACKAGE _callBack);
	//用于遍历所有值，可用来修改字符串引用，查找某个值是否被使用等
	void traversalAllValue(TRAVERSE_STRIDX_CALLBACK _callBack);
	TTablePackage& tablePackage(const QString& _name);
	//设置值包括是普通数据还是数组，是字符串还是其他
	const Res_value* setValue(TTablePackage& _package, uint32_t _typeid, uint32_t _specid, uint32_t _id, Res_value::_DataType _dataYype, uint32_t _data);
	const Res_value* setValue(TTablePackage& _package, uint32_t _typeid, uint32_t _specid, uint32_t _id, uint32_t _idx, Res_value::_DataType _dataYype, uint32_t _data);
	const Res_value* setValue(TTablePackage& _package, uint32_t _typeid, uint32_t _specid, uint32_t _id, const QString& _data, bool _force);
	const Res_value* setValue(TTablePackage& _package, uint32_t _typeid, uint32_t _specid, uint32_t _id, uint32_t _idx, const QString& _data, bool _force);
	//获得这个字符串的引用数
	quint32 getReferenceCount(quint32 _index);
private:
	const QSharedPointer<ResTable_entry>& getValue(TTablePackage& _package, uint32_t _typeid, uint32_t _specid, uint32_t _id) const;
	const QSharedPointer<ResTable_entry>& getValueOrInsert(TTablePackage& _package, uint32_t _typeid, uint32_t _specid, uint32_t _id);
private:
	TStringPool m_StringPool;
	QMap<QString, TTablePackage> m_tablePackages;

	QPublicFinal* m_publicFinal;
};

Q_DECLARE_METATYPE(ResTable_config)

#endif // QResArscParser_h__
