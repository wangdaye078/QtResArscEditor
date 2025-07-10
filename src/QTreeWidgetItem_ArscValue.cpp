#include "QTreeWidgetItem_ArscValue.h"
#include "QTableType.h"
#include "QTablePackage.h"
QTreeWidgetItem_ArscValue::QTreeWidgetItem_ArscValue(QTreeWidget* _parent)
	: QTreeWidgetItem(_parent)
{
}
QTreeWidgetItem_ArscValue::QTreeWidgetItem_ArscValue(QTreeWidgetItem* _parent)
	: QTreeWidgetItem(_parent)
{

}
QTreeWidgetItem_ArscValue::~QTreeWidgetItem_ArscValue()
{
}

QVariant QTreeWidgetItem_ArscValue::data(int _column, int _role) const
{
	if (_column == 2 && _role == Qt::ToolTipRole)
	{
		PTablePackage t_package = data(0, eValueItemRole_package).value<PTablePackage>();
		PResValue t_pv = data(0, eValueItemRole_entry).value<PResValue>();
		Res_value* t_value;
		PArscRichString t_sv = t_pv->getValue(&t_value);
		if (data(0, eValueItemRole_type) == eValueItemType_array)
		{
			uint32_t t_parent = reinterpret_cast<TTableMapEntry*>(t_pv.get())->entry.parent.ident;
			return (t_parent != 0) ? QVariant(QString("parent(%1)").arg(t_package->getKeyString("@", true, t_parent))) : QVariant();
		}
		else if (t_value == NULL)
			return QVariant();
		else if (t_value->dataType == Res_value::_DataType::TYPE_STRING)
			//应用计数g_publicStrPool里面默认用了LEAST_REF_COUNT次，上面t_sv又用了一次，所以需要减这些，才是字符串的真实引用次数
			return QVariant(QString("0x%1(refCount: %2)").arg(g_publicStrPool->getRefIndex(t_sv), 8, 16, QChar('0')).arg(t_sv.use_count() - LEAST_REF_COUNT - 1));
		else if (t_value->dataType == Res_value::_DataType::TYPE_REFERENCE || t_value->dataType == Res_value::_DataType::TYPE_DYNAMIC_REFERENCE)
			return QVariant(QString("0x%1(%2)").arg(t_value->data, 8, 16, QChar('0')).arg(t_package->getKeyString("@", true, t_value->data)));
		else if (t_value->dataType == Res_value::_DataType::TYPE_ATTRIBUTE || t_value->dataType == Res_value::_DataType::TYPE_DYNAMIC_ATTRIBUTE)
			return QVariant(QString("0x%1(%2)").arg(t_value->data, 8, 16, QChar('0')).arg(t_package->getKeyString("?", false, t_value->data)));
		else
			return QVariant(QString("0x%1").arg(t_value->data, 8, 16, QChar('0')));
	}
	else
		return QTreeWidgetItem::data(_column, _role);
}
