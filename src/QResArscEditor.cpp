#include "QResArscEditor.h"
#include "src/QResArscParser.h"
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QToolButton>
#include <QTreeWidget>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox> 
#include "basicDefine.h"
#include "QAppendDialog.h"
#include "QEditDialog.h"
#include "QAddLocaleDialog.h"

QResArscEditor::QResArscEditor(QWidget* _parent)
	: QResArscEditorUI(_parent), m_valueMenu(NULL), m_treeMenu(NULL)
{
	m_Parser = new QResArscParser(this);
}

QResArscEditor::~QResArscEditor()
{
}
void QResArscEditor::refreshArscTree()
{
	m_TW_tree->clear();
	const ResTable_package& t_tablePackage = m_Parser->tablePackage();
	QTreeWidgetItem* t_packageItem = new QTreeWidgetItem(m_TW_tree);
	t_packageItem->setText(0, WCHARToQString(t_tablePackage.name));
	t_packageItem->setData(0, eTreeItemRole_type, eTreeItemType_package);

	const TStringPool& t_typeString = m_Parser->typeString();
	const QMap<uint, TTableTypeData>& t_typeDatas = m_Parser->tableTypeDatas();

	for (QMap<uint, TTableTypeData>::const_iterator i = t_typeDatas.begin(); i != t_typeDatas.end(); ++i)
	{
		Q_ASSERT(i.key() >= 1);
		Q_ASSERT(i.key() <= (uint)t_typeString.strings.size());

		QTreeWidgetItem* t_typeItem = new QTreeWidgetItem(t_packageItem);
		t_typeItem->setText(0, t_typeString.strings[i.key() - 1]);
		t_typeItem->setData(0, eTreeItemRole_type, eTreeItemType_type);
		t_typeItem->setData(0, eTreeItemRole_typeid, i.key());
		const QVector<TTableTypeEx>& t_tableType = t_typeDatas[i.key()].typeDatas;
		for (int j = 0; j < t_tableType.size(); ++j)
		{
			QTreeWidgetItem* t_specItem = new QTreeWidgetItem(t_typeItem);
			t_specItem->setText(0, tableConfig2String(t_typeString.strings[i.key() - 1], t_tableType[j].config));
			t_specItem->setData(0, eTreeItemRole_type, eTreeItemType_spec);
			t_specItem->setData(0, eTreeItemRole_typeid, i.key());
			t_specItem->setData(0, eTreeItemRole_specid, j);
			t_specItem->setData(0, eTreeItemRole_tableConfig, QVariant::fromValue(t_tableType[j].config));

		}
	}
	t_packageItem->setExpanded(true);
}
void QResArscEditor::refreshResTableType(quint32 _typeid, quint32 _specid)
{
	const QMap<uint, TTableTypeData>& t_typeDatas = m_Parser->tableTypeDatas();
	Q_ASSERT(t_typeDatas.contains(_typeid));
	const QVector<TTableTypeEx>& t_tableType = t_typeDatas[_typeid].typeDatas;
	Q_ASSERT(_specid < (quint32)t_tableType.size());
	const TTableTypeEx& t_type = t_tableType[_specid];
	m_TW_value->clear();
	for (int i = 0; i < t_type.entryValue.size(); ++i)
	{
		QSharedPointer<ResTable_entry> t_ptrEntry = t_type.entryValue[i];
		if (t_ptrEntry.isNull())
			continue;
		if ((*t_ptrEntry).size == sizeof(ResTable_entry))
		{
			TTableValueEntry* t_pValueEntry = reinterpret_cast<TTableValueEntry*>(t_ptrEntry.get());
			QTreeWidgetItem* t_valueItem = new QTreeWidgetItem(m_TW_value);
			t_valueItem->setData(0, eValueItemRole_type, eValueItemType_value);
			t_valueItem->setData(0, eValueItemRole_data, t_pValueEntry->value.data);
			t_valueItem->setData(0, eValueItemRole_datatype, (uint32_t)t_pValueEntry->value.dataType);
			t_valueItem->setData(0, eValueItemRole_id, i);
			t_valueItem->setText(0, QString("0x7f%1%2").arg(_typeid, 2, 16, QChar('0')).arg(i, 4, 16, QChar('0')));
			t_valueItem->setText(1, m_Parser->keyString(t_pValueEntry->key.index));
			t_valueItem->setText(2, m_Parser->resValue2String(t_pValueEntry->value));
			t_valueItem->setToolTip(1, QString("0x%1").arg(t_pValueEntry->key.index, 8, 16, QChar('0')));
			QString t_toolTip = QString("0x%1").arg(t_pValueEntry->value.data, 8, 16, QChar('0'));
			if (t_pValueEntry->value.dataType == Res_value::_DataType::TYPE_STRING)
				t_toolTip += "(refCount:" + QString::number(m_Parser->getReferenceCount(t_pValueEntry->value.data)) + ")";
			t_valueItem->setToolTip(2, t_toolTip);
		}
		else
		{
			TTableMapEntryEx* t_pMapValue = reinterpret_cast<TTableMapEntryEx*>(t_ptrEntry.get());
			QTreeWidgetItem* t_mapItem = new QTreeWidgetItem(m_TW_value);
			t_mapItem->setData(0, eValueItemRole_type, eValueItemType_array);
			t_mapItem->setData(0, eValueItemRole_data, t_pMapValue->key.index);
			t_mapItem->setData(0, eValueItemRole_id, i);
			t_mapItem->setText(0, QString("0x7f%1%2").arg(_typeid, 2, 16, QChar('0')).arg(i, 4, 16, QChar('0')));
			t_mapItem->setText(1, m_Parser->keyString(t_pMapValue->key.index));
			t_mapItem->setToolTip(1, QString("0x%1").arg(t_pMapValue->key.index, 8, 16, QChar('0')));
			for (quint32 j = 0; j < t_pMapValue->count; ++j)
			{
				ResTable_map& t_tableMap = t_pMapValue->tablemap[j];
				QTreeWidgetItem* t_valueItem = new QTreeWidgetItem(t_mapItem);
				t_valueItem->setData(0, eValueItemRole_type, eValueItemType_arrayitem);
				t_valueItem->setData(0, eValueItemRole_data, t_tableMap.value.data);
				t_valueItem->setData(0, eValueItemRole_datatype, (uint32_t)t_tableMap.value.dataType);
				t_valueItem->setData(0, eValueItemRole_id, j);
				t_valueItem->setData(0, eValueItemRole_parentid, i);
				t_valueItem->setText(0, QString("0x%1").arg(t_tableMap.name.indent, 8, 16, QChar('0')));
				t_valueItem->setText(1, QString("0x%1").arg(t_tableMap.name.indent, 8, 16, QChar('0')));
				t_valueItem->setText(2, m_Parser->resValue2String(t_tableMap.value));
				t_valueItem->setToolTip(1, QString("0x%1").arg(t_tableMap.name.indent, 8, 16, QChar('0')));
				QString t_toolTip = QString("0x%1").arg(t_tableMap.value.data, 8, 16, QChar('0'));
				if (t_tableMap.value.dataType == Res_value::_DataType::TYPE_STRING)
					t_toolTip += "(refCount:" + QString::number(m_Parser->getReferenceCount(t_tableMap.value.data)) + ")";
				t_valueItem->setToolTip(2, t_toolTip);
			}
		}
	}
}
void QResArscEditor::onOpenReleased_Slot(void)
{
	QString t_FileName = QFileDialog::getOpenFileName(this, tr("Open ARSC File"), ".",
		tr("ARSC File (*.arsc)"), NULL, QFileDialog::DontConfirmOverwrite);
	if (t_FileName.isEmpty())
		return;
	m_LE_filePath->setText(t_FileName);
	m_Parser->readFile(t_FileName);
	refreshArscTree();
}
void QResArscEditor::onSaveReleased_Slot(void)
{
	QString t_FileName = QFileDialog::getSaveFileName(this, tr("Get OutPut ARSC File"), ".",
		tr("ARSC File (*.arsc)"), NULL, QFileDialog::DontConfirmOverwrite);
	if (t_FileName.isEmpty())
		return;
	m_LE_filePath->setText(t_FileName);
	if (!m_Parser->writeFile(t_FileName))
		QMessageBox::warning(this, tr("warning"), tr("The specified file cannot be written in !"));
}
void QResArscEditor::onTreeCurrentItemChanged_slot(QTreeWidgetItem* _current, QTreeWidgetItem* _previous)
{
	if (_current == NULL)
		return;
	if (_current->data(0, eTreeItemRole_type).toUInt() != eTreeItemType_spec)
		return;

	quint32 t_typeid = _current->data(0, eTreeItemRole_typeid).toUInt();
	quint32 t_specid = _current->data(0, eTreeItemRole_specid).toUInt();
	refreshResTableType(t_typeid, t_specid);
}
void QResArscEditor::onShowValueContextMenu_slot(const QPoint& _pos)
{
	if (m_TW_tree->currentItem() == NULL)
		return;
	if (m_TW_tree->currentItem()->data(0, eTreeItemRole_type).toUInt() != eTreeItemType_spec)
		return;
	//默认的那些值不能删
	quint32 t_specid = m_TW_tree->currentItem()->data(0, eTreeItemRole_specid).toUInt();

	delete m_valueMenu;
	m_valueMenu = new QMenu(m_TW_value);
	m_valueMenu->setObjectName(QString::fromUtf8("m_valueMenu"));
	m_valueMenu->addAction(m_AC_AddValue);
	QTreeWidgetItem* t_item = m_TW_value->currentItem();
	if (t_specid != 0 && t_item != NULL && t_item->data(0, eValueItemRole_type).toUInt() != eValueItemType_arrayitem)
		m_valueMenu->addAction(m_AC_DeleteValue);
	if (t_item != NULL && t_item->data(0, eValueItemRole_type).toUInt() != eValueItemType_array)
		m_valueMenu->addAction(m_AC_EditValue);
	m_valueMenu->popup(QCursor::pos());
}
void QResArscEditor::onShowTreeContextMenu_slot(const QPoint& _pos)
{
	if (m_TW_tree->currentItem() == NULL)
		return;
	QTreeWidgetItem* t_item = m_TW_tree->currentItem();
	QTreeWidgetItem* t_typeItem = t_item;
	if (t_item->data(0, eTreeItemRole_type).toUInt() != eTreeItemType_type)
		t_typeItem = t_item->parent();

	delete m_treeMenu;
	m_treeMenu = new QMenu(m_TW_tree);
	m_treeMenu->setObjectName(QString::fromUtf8("m_treeMenu"));

	quint32 t_typeid = t_typeItem->data(0, eTreeItemRole_typeid).toUInt();
	const QMap<uint, TTableTypeData>& t_typeDatas = m_Parser->tableTypeDatas();
	Q_ASSERT(t_typeDatas.contains(t_typeid));
	const TTableTypeData& t_tableTypeData = t_typeDatas[t_typeid];
	for (int i = 0; i < t_tableTypeData.typeSpec.configmask.size(); ++i)
	{
		if (t_tableTypeData.typeSpec.configmask[i] == ACONFIGURATION_LOCALE)
		{
			m_treeMenu->addAction(m_AC_AddLocale);
			break;
		}
	}

	if (t_item->data(0, eTreeItemRole_type).toUInt() == eTreeItemType_spec)
	{
		//m_treeMenu->addAction(m_AC_ExportLocale);
		//m_treeMenu->addAction(m_AC_ImportLocale);
	}
	m_treeMenu->popup(QCursor::pos());
}
void QResArscEditor::onAddValueTriggered_slot(void)
{
	if (m_TW_tree->currentItem() == NULL)
		return;
	if (m_TW_tree->currentItem()->data(0, eTreeItemRole_type).toUInt() != eTreeItemType_spec)
		return;
	quint32 t_typeid = m_TW_tree->currentItem()->data(0, eTreeItemRole_typeid).toUInt();
	quint32 t_specid = m_TW_tree->currentItem()->data(0, eTreeItemRole_specid).toUInt();

	const QMap<uint, TTableTypeData>& t_typeDatas = m_Parser->tableTypeDatas();
	Q_ASSERT(t_typeDatas.contains(t_typeid));
	const QVector<TTableTypeEx>& t_tableType = t_typeDatas[t_typeid].typeDatas;
	Q_ASSERT(t_specid < (quint32)t_tableType.size());

	const TTableTypeEx& t_defaultType = t_tableType[0];
	const TTableTypeEx& t_type = t_tableType[t_specid];

	const QVector<uint32_t>& t_configMasks = t_typeDatas[t_typeid].typeSpec.configmask;
	uint32_t t_configMask = getTableConfigMask(t_type.config);

	QMap<uint32_t, ResStringPool_ref> m_addType;
	for (int i = 0; i < t_type.entryValue.size(); ++i)
	{
		//这个字段在当前配置（比如语言本地化，比如分辨率），是不需要处理的
		if ((t_configMasks[i] & t_configMask) != t_configMask)
			continue;
		QSharedPointer<ResTable_entry> t_ptrEntry = t_type.entryValue[i];
		if (!t_ptrEntry.isNull())
			continue;

		QSharedPointer<ResTable_entry> t_ptrDefaultEntry = t_defaultType.entryValue[i];
		if (t_ptrDefaultEntry.isNull())
			continue;

		m_addType.insert(0x7F000000 + (t_typeid << 16) + i, t_ptrDefaultEntry.get()->key);
	}
	if (m_addType.size() == 0)
	{
		QMessageBox::information(this, tr("information"), tr("no items found to add !"));
		return;
	}
	m_appendDialog->m_CB_Name->clear();
	for (QMap<uint32_t, ResStringPool_ref>::iterator i = m_addType.begin(); i != m_addType.end(); ++i)
		m_appendDialog->m_CB_Name->addItem(m_Parser->keyString(i.value().index), i.key());
	if (m_appendDialog->exec() == QDialog::Accepted)
	{
		uint32_t t_id = m_appendDialog->m_CB_Name->currentData().toUInt();
		m_Parser->copyValue(t_typeid, t_specid, t_id & 0xFFFF);
		refreshResTableType(t_typeid, t_specid);
	}
}
void QResArscEditor::onDeleteValueTriggered_slot(void)
{
	if (m_TW_tree->currentItem() == NULL)
		return;
	if (m_TW_tree->currentItem()->data(0, eTreeItemRole_type).toUInt() != eTreeItemType_spec)
		return;
	quint32 t_typeid = m_TW_tree->currentItem()->data(0, eTreeItemRole_typeid).toUInt();
	quint32 t_specid = m_TW_tree->currentItem()->data(0, eTreeItemRole_specid).toUInt();
	QTreeWidgetItem* t_item = m_TW_value->currentItem();
	//数组的子项不能随便删除
	if (t_specid == 0 || t_item == NULL || t_item->data(0, eValueItemRole_type).toUInt() == eValueItemType_arrayitem)
		return;
	if (QMessageBox::question(this, tr("warning"), tr("Are you sure you want to delete this item?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
		return;

	m_Parser->deleteValue(t_typeid, t_specid, t_item->data(0, eValueItemRole_id).toUInt());
	refreshResTableType(t_typeid, t_specid);
}
void QResArscEditor::onEditValueTriggered_slot(void)
{
	if (m_TW_tree->currentItem() == NULL)
		return;
	QTreeWidgetItem* t_treeItem = m_TW_tree->currentItem();
	if (t_treeItem->data(0, eTreeItemRole_type).toUInt() != eTreeItemType_spec)
		return;
	quint32 t_typeid = t_treeItem->data(0, eTreeItemRole_typeid).toUInt();
	quint32 t_specid = t_treeItem->data(0, eTreeItemRole_specid).toUInt();
	QTreeWidgetItem* t_item = m_TW_value->currentItem();
	if (t_specid == 0 || t_item == NULL || t_item->data(0, eValueItemRole_type).toUInt() == eValueItemType_array)
		return;
	m_editDialog->m_LE_ID->setText(t_item->text(0));
	m_editDialog->m_LE_Name->setText(t_item->text(1));
	m_editDialog->setResArscParser(m_Parser);
	uint32_t t_dataType = t_item->data(0, eValueItemRole_datatype).toUInt();
	uint32_t t_data = t_item->data(0, eValueItemRole_data).toUInt();
	m_editDialog->setData(t_treeItem->data(0, eTreeItemRole_tableConfig).value<ResTable_config>(), t_dataType, t_data);
	if (m_editDialog->exec() != QDialog::Accepted)
		return;
	Res_value::_DataType t_newType = (Res_value::_DataType)m_editDialog->getType();
	if (t_newType == Res_value::_DataType::TYPE_STRING)
	{
		QString t_value = m_editDialog->getSData();
		bool t_force = false;
		if (t_dataType == (uint32_t)Res_value::_DataType::TYPE_STRING && m_Parser->getReferenceCount(t_data) > 1)
			if (QMessageBox::question(this, tr("warning"), tr("The old string number of citations > 1, chang all string?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
				t_force = true;
		if (t_item->data(0, eValueItemRole_type).toUInt() == eValueItemType_arrayitem)
			m_Parser->setValue(t_typeid, t_specid, t_item->data(0, eValueItemRole_parentid).toUInt(), t_item->data(0, eValueItemRole_id).toUInt(), t_value, t_force);
		else
			m_Parser->setValue(t_typeid, t_specid, t_item->data(0, eValueItemRole_id).toUInt(), t_value, t_force);
	}
	else
	{
		uint32_t t_value = m_editDialog->getData();
		if (t_item->data(0, eValueItemRole_type).toUInt() == eValueItemType_arrayitem)
			m_Parser->setValue(t_typeid, t_specid, t_item->data(0, eValueItemRole_parentid).toUInt(), t_item->data(0, eValueItemRole_id).toUInt(), t_newType, t_value);
		else
			m_Parser->setValue(t_typeid, t_specid, t_item->data(0, eValueItemRole_id).toUInt(), t_newType, t_value);
	}
	refreshResTableType(t_typeid, t_specid);
}
void QResArscEditor::onAddLocaleTriggered_slot(void)
{
	if (m_addLocaleDialog->exec() != QDialog::Accepted)
		return;

	QTreeWidgetItem* t_item = m_TW_tree->currentItem();
	QTreeWidgetItem* t_typeItem = t_item;
	if (t_item->data(0, eTreeItemRole_type).toUInt() != eTreeItemType_type)
		t_typeItem = t_item->parent();
	quint32 t_typeid = t_typeItem->data(0, eTreeItemRole_typeid).toUInt();

	ResTable_config t_config;
	memset(&t_config, 0, sizeof(t_config));
	t_config.size = sizeof(t_config);
	m_addLocaleDialog->getLocale(t_config);
	m_Parser->addLocale(t_typeid, t_config);
	refreshArscTree();
}
void QResArscEditor::onExportLocaleTriggered_slot(void)
{

}
void QResArscEditor::onImportLocaleTriggered_slot(void)
{

}
