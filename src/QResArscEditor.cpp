#include "QResArscEditor.h"
#include "src/QResArscParser.h"
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QToolButton>
#include <QTreeWidget>
#include <QMenu>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox> 
#include <QDomDocument>
#include <QDomElement>
#include <QXmlStreamWriter>
#include <QTextStream>
#include "basicDefine.h"
#include "QAppendDialog.h"
#include "QEditDialog.h"
#include "QAddLocaleDialog.h"

QResArscEditor::QResArscEditor(QWidget* _parent)
	: QResArscEditorUI(_parent), m_valueMenu(NULL), m_treeMenu(NULL), m_BasePath(".")
{
	m_Parser = new QResArscParser(this);
}

QResArscEditor::~QResArscEditor()
{
}
void QResArscEditor::refreshArscTree()
{
	m_TW_tree->clear();
	m_Parser->traversalAllTablePackage(std::bind(&QResArscEditor::onRefreshTablePackage, this, std::placeholders::_1, std::placeholders::_2));
}
void QResArscEditor::onRefreshTablePackage(const QString& _packageName, const TTablePackage& _tablePackage)
{
	QTreeWidgetItem* t_packageItem = new QTreeWidgetItem(m_TW_tree);
	t_packageItem->setText(0, _packageName);
	t_packageItem->setData(0, eTreeItemRole_type, eTreeItemType_package);
	t_packageItem->setData(0, eTreeItemRole_package, _packageName);
	_tablePackage.traversalData(std::bind(&QResArscEditor::onRefreshTablePackageData, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
	t_packageItem->setExpanded(true);
}
void QResArscEditor::onRefreshTablePackageData(const QString& _packageName, ETreeItemType _type, uint32_t _id1, uint32_t _id2, const QString& _name)
{
	QList<QTreeWidgetItem*> t_packageItems = m_TW_tree->findItems(_packageName, Qt::MatchFixedString);
	Q_ASSERT(t_packageItems.size() == 1);
	QTreeWidgetItem* t_Item = t_packageItems[0];
	if (_type == eTreeItemType_spec)
	{
		for (int i = 0; i < t_Item->childCount(); ++i)
		{
			if (t_Item->child(i)->data(0, eTreeItemRole_typeid) == _id1)
			{
				t_Item = t_Item->child(i);
				break;
			}
		}
	}

	QTreeWidgetItem* t_subItem = new QTreeWidgetItem(t_Item);
	t_subItem->setText(0, _name);
	t_subItem->setData(0, eTreeItemRole_type, _type);
	t_subItem->setData(0, eTreeItemRole_package, _packageName);
	t_subItem->setData(0, eTreeItemRole_typeid, _id1);
	t_subItem->setData(0, eTreeItemRole_specid, _id2);
}
void widgetItemSetData(QTreeWidgetItem* _item, EValueItemType _type, uint32_t _data, uint32_t _dataType, int _id, const QString& _text)
{
	_item->setData(0, eValueItemRole_type, _type);
	_item->setData(0, eValueItemRole_data, _data);
	_item->setData(0, eValueItemRole_datatype, _dataType);
	_item->setData(0, eValueItemRole_id, _id);
	_item->setText(0, _text);
}
void QResArscEditor::refreshResTableType(const TTablePackage& _tablePackage, quint32 _typeid, quint32 _specid)
{
	const TTableTypeEx& t_type = _tablePackage.getTableType(_typeid, _specid);
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
			widgetItemSetData(t_valueItem, eValueItemType_value, t_pValueEntry->value.data,
				(uint32_t)t_pValueEntry->value.dataType, i, QString("0x7f%1%2").arg(_typeid, 2, 16, QChar('0')).arg(i, 4, 16, QChar('0')));
			t_valueItem->setText(1, _tablePackage.getKeyString(t_pValueEntry->key.index));
			t_valueItem->setText(2, _tablePackage.resValue2String(t_pValueEntry->value));
			t_valueItem->setToolTip(1, QString("0x%1").arg(t_pValueEntry->key.index, 8, 16, QChar('0')));
			QString t_toolTip = QString("0x%1").arg(t_pValueEntry->value.data, 8, 16, QChar('0'));
			if (t_pValueEntry->value.dataType == Res_value::_DataType::TYPE_STRING)
				t_toolTip += "(refCount:" + QString::number(m_Parser->getReferenceCount(t_pValueEntry->value.data)) + ")";
			t_valueItem->setToolTip(2, t_toolTip);
		}
		else
		{
			TTableMapEntry* t_pMapValue = reinterpret_cast<TTableMapEntry*>(t_ptrEntry.get());
			QTreeWidgetItem* t_mapItem = new QTreeWidgetItem(m_TW_value);
			widgetItemSetData(t_mapItem, eValueItemType_array, t_pMapValue->key.index, i,
				(uint32_t)Res_value::_DataType::TYPE_NULL, QString("0x7f%1%2").arg(_typeid, 2, 16, QChar('0')).arg(i, 4, 16, QChar('0')));
			t_mapItem->setText(1, _tablePackage.getKeyString(t_pMapValue->key.index));
			t_mapItem->setToolTip(1, QString("0x%1").arg(t_pMapValue->key.index, 8, 16, QChar('0')));
			for (quint32 j = 0; j < t_pMapValue->count; ++j)
			{
				ResTable_map& t_tableMap = t_pMapValue->tablemap[j];
				QTreeWidgetItem* t_valueItem = new QTreeWidgetItem(t_mapItem);
				widgetItemSetData(t_valueItem, eValueItemType_arrayitem, t_tableMap.value.data,
					(uint32_t)t_tableMap.value.dataType, j, QString("0x%1").arg(t_tableMap.name.indent, 8, 16, QChar('0')));
				t_valueItem->setData(0, eValueItemRole_parentid, i);
				t_valueItem->setText(1, _tablePackage.getReferenceDestination(ResTable_config(), t_tableMap.name.indent));
				t_valueItem->setText(2, _tablePackage.resValue2String(t_tableMap.value));
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
	QString t_FileName = QFileDialog::getOpenFileName(this, tr("Open ARSC File"), m_BasePath,
		tr("ARSC File (*.arsc)"), NULL, QFileDialog::DontConfirmOverwrite);
	if (t_FileName.isEmpty())
		return;
	m_BasePath = QFileInfo(t_FileName).absolutePath();
	m_LE_filePath->setText(t_FileName);
	m_Parser->readFile(t_FileName);
	refreshArscTree();
}
void QResArscEditor::onSaveReleased_Slot(void)
{
	QString t_FileName = QFileDialog::getSaveFileName(this, tr("Get OutPut ARSC File"), m_BasePath,
		tr("ARSC File (*.arsc)"), NULL, QFileDialog::DontConfirmOverwrite);
	if (t_FileName.isEmpty())
		return;
	m_BasePath = QFileInfo(t_FileName).absolutePath();
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
	QString t_packageName = _current->data(0, eTreeItemRole_package).toString();
	quint32 t_typeid = _current->data(0, eTreeItemRole_typeid).toUInt();
	quint32 t_specid = _current->data(0, eTreeItemRole_specid).toUInt();
	const TTablePackage& t_tablePackage = m_Parser->tablePackage(t_packageName);
	refreshResTableType(t_tablePackage, t_typeid, t_specid);
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
	if (t_specid != 0)
		m_valueMenu->addAction(m_AC_AddValue);
	if (t_specid != 0)
		m_valueMenu->addAction(m_AC_AddAllValue);
	QTreeWidgetItem* t_item = m_TW_value->currentItem();
	if (t_specid != 0 && t_item != NULL && t_item->data(0, eValueItemRole_type).toUInt() != eValueItemType_arrayitem)
		m_valueMenu->addAction(m_AC_DeleteValue);
	if (t_item != NULL && t_item->data(0, eValueItemRole_type).toUInt() != eValueItemType_array)
		m_valueMenu->addAction(m_AC_EditValue);
	if (m_valueMenu->actions().size() > 0)
		m_valueMenu->addSeparator();
	m_valueMenu->addAction(m_AC_ExpandAll);
	m_valueMenu->addAction(m_AC_ExportLocale);
	m_valueMenu->addAction(m_AC_ImportLocale);
	m_valueMenu->popup(QCursor::pos());
}
void QResArscEditor::onShowTreeContextMenu_slot(const QPoint& _pos)
{
	if (m_TW_tree->currentItem() == NULL)
		return;
	QTreeWidgetItem* t_item = m_TW_tree->currentItem();
	if (t_item->data(0, eTreeItemRole_type).toUInt() == eTreeItemType_package)
		return;
	QTreeWidgetItem* t_typeItem = t_item;
	if (t_item->data(0, eTreeItemRole_type).toUInt() != eTreeItemType_type)
		t_typeItem = t_item->parent();

	delete m_treeMenu;
	m_treeMenu = new QMenu(m_TW_tree);
	m_treeMenu->setObjectName(QString::fromUtf8("m_treeMenu"));

	QString t_packageName = t_typeItem->data(0, eTreeItemRole_package).toString();
	quint32 t_typeid = t_typeItem->data(0, eTreeItemRole_typeid).toUInt();
	const TTablePackage& t_tablePackage = m_Parser->tablePackage(t_packageName);
	const QVector<uint32_t>& t_configMasks = t_tablePackage.getTableTypeMask(t_typeid);
	for (int i = 0; i < t_configMasks.size(); ++i)
	{
		if ((t_configMasks[i] & ACONFIGURATION_LOCALE) != 0)
		{
			m_treeMenu->addAction(m_AC_AddLocale);
			break;
		}
	}

	if (t_item->data(0, eTreeItemRole_type).toUInt() == eTreeItemType_spec)
	{
		m_treeMenu->addAction(m_AC_ExportLocale);
		m_treeMenu->addAction(m_AC_ImportLocale);
	}
	m_treeMenu->popup(QCursor::pos());
}
void QResArscEditor::onAddValueTriggered_slot(void)
{
	QTreeWidgetItem* t_treeItem = m_TW_tree->currentItem();
	if (t_treeItem == NULL)
		return;
	if (t_treeItem->data(0, eTreeItemRole_type).toUInt() != eTreeItemType_spec)
		return;
	QString t_packageName = t_treeItem->data(0, eTreeItemRole_package).toString();
	quint32 t_typeid = t_treeItem->data(0, eTreeItemRole_typeid).toUInt();
	quint32 t_specid = t_treeItem->data(0, eTreeItemRole_specid).toUInt();
	TTablePackage& t_tablePackage = m_Parser->tablePackage(t_packageName);

	const TTableTypeEx& t_defaultType = t_tablePackage.getTableType(t_typeid, 0);
	const TTableTypeEx& t_type = t_tablePackage.getTableType(t_typeid, t_specid);

	const QVector<uint32_t>& t_configMasks = t_tablePackage.getTableTypeMask(t_typeid);
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
		m_appendDialog->m_CB_Name->addItem(QString("(0x%1)%2").arg(i.key(), 8, 16, QChar('0')).arg(t_tablePackage.getKeyString(i.value().index)), i.key());
	if (m_appendDialog->exec() == QDialog::Accepted)
	{
		uint32_t t_id = m_appendDialog->m_CB_Name->currentData().toUInt();
		t_tablePackage.copyValue(t_typeid, t_specid, t_id & 0xFFFF);
		refreshResTableType(t_tablePackage, t_typeid, t_specid);
	}
}
void QResArscEditor::onAddAllValueTriggered_slot(void)
{
	QTreeWidgetItem* t_treeItem = m_TW_tree->currentItem();
	if (t_treeItem == NULL)
		return;
	if (t_treeItem->data(0, eTreeItemRole_type).toUInt() != eTreeItemType_spec)
		return;
	QString t_packageName = t_treeItem->data(0, eTreeItemRole_package).toString();
	quint32 t_typeid = t_treeItem->data(0, eTreeItemRole_typeid).toUInt();
	quint32 t_specid = t_treeItem->data(0, eTreeItemRole_specid).toUInt();
	TTablePackage& t_tablePackage = m_Parser->tablePackage(t_packageName);

	const TTableTypeEx& t_defaultType = t_tablePackage.getTableType(t_typeid, 0);
	const TTableTypeEx& t_type = t_tablePackage.getTableType(t_typeid, t_specid);

	const QVector<uint32_t>& t_configMasks = t_tablePackage.getTableTypeMask(t_typeid);;
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
		t_tablePackage.copyValue(t_typeid, t_specid, i);
	}
	refreshResTableType(t_tablePackage, t_typeid, t_specid);
}
void QResArscEditor::onDeleteValueTriggered_slot(void)
{
	if (m_TW_tree->currentItem() == NULL)
		return;
	if (m_TW_tree->currentItem()->data(0, eTreeItemRole_type).toUInt() != eTreeItemType_spec)
		return;
	QString t_packageName = m_TW_tree->currentItem()->data(0, eTreeItemRole_package).toString();
	quint32 t_typeid = m_TW_tree->currentItem()->data(0, eTreeItemRole_typeid).toUInt();
	quint32 t_specid = m_TW_tree->currentItem()->data(0, eTreeItemRole_specid).toUInt();
	TTablePackage& t_tablePackage = m_Parser->tablePackage(t_packageName);

	QTreeWidgetItem* t_item = m_TW_value->currentItem();
	//数组的子项不能随便删除
	if (t_specid == 0 || t_item == NULL || t_item->data(0, eValueItemRole_type).toUInt() == eValueItemType_arrayitem)
		return;
	if (QMessageBox::question(this, tr("warning"), tr("Are you sure you want to delete this item?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
		return;

	t_tablePackage.deleteValue(t_typeid, t_specid, t_item->data(0, eValueItemRole_id).toUInt(), std::bind(&QResArscParser::traversalAllValue, m_Parser, std::placeholders::_1));
	refreshResTableType(t_tablePackage, t_typeid, t_specid);
}
void QResArscEditor::onEditValueTriggered_slot(void)
{
	if (m_TW_tree->currentItem() == NULL)
		return;
	QTreeWidgetItem* t_treeItem = m_TW_tree->currentItem();
	if (t_treeItem->data(0, eTreeItemRole_type).toUInt() != eTreeItemType_spec)
		return;
	QString t_packageName = t_treeItem->data(0, eTreeItemRole_package).toString();
	quint32 t_typeid = t_treeItem->data(0, eTreeItemRole_typeid).toUInt();
	quint32 t_specid = t_treeItem->data(0, eTreeItemRole_specid).toUInt();
	TTablePackage& t_tablePackage = m_Parser->tablePackage(t_packageName);
	const TTableTypeEx& t_tableType = t_tablePackage.getTableType(t_typeid, t_specid);

	QTreeWidgetItem* t_item = m_TW_value->currentItem();
	if (t_item == NULL || t_item->data(0, eValueItemRole_type).toUInt() == eValueItemType_array)
		return;
	m_editDialog->m_LE_ID->setText(t_item->text(0));
	m_editDialog->m_LE_Name->setText(t_item->text(1));
	m_editDialog->setTablePackage(&t_tablePackage);
	uint32_t t_dataType = t_item->data(0, eValueItemRole_datatype).toUInt();
	uint32_t t_data = t_item->data(0, eValueItemRole_data).toUInt();

	m_editDialog->setData(t_tableType.config, t_dataType, t_data);
	if (m_editDialog->exec() != QDialog::Accepted)
		return;
	Res_value::_DataType t_newType = (Res_value::_DataType)m_editDialog->getType();
	const Res_value* t_newValue;
	if (t_newType == Res_value::_DataType::TYPE_STRING)
	{
		QString t_value = m_editDialog->getSData();
		bool t_force = false;
		if (t_dataType == (uint32_t)Res_value::_DataType::TYPE_STRING && m_Parser->getReferenceCount(t_data) > 1)
			if (QMessageBox::question(this, tr("warning"), tr("The old string number of citations > 1, chang all string?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
				t_force = true;
		if (t_item->data(0, eValueItemRole_type).toUInt() == eValueItemType_arrayitem)
			t_newValue = m_Parser->setValue(t_tablePackage, t_typeid, t_specid, t_item->data(0, eValueItemRole_parentid).toUInt(), t_item->data(0, eValueItemRole_id).toUInt(), t_value, t_force);
		else
			t_newValue = m_Parser->setValue(t_tablePackage, t_typeid, t_specid, t_item->data(0, eValueItemRole_id).toUInt(), t_value, t_force);
	}
	else
	{
		uint32_t t_value = m_editDialog->getData();
		if (t_item->data(0, eValueItemRole_type).toUInt() == eValueItemType_arrayitem)
			t_newValue = m_Parser->setValue(t_tablePackage, t_typeid, t_specid, t_item->data(0, eValueItemRole_parentid).toUInt(), t_item->data(0, eValueItemRole_id).toUInt(), t_newType, t_value);
		else
			t_newValue = m_Parser->setValue(t_tablePackage, t_typeid, t_specid, t_item->data(0, eValueItemRole_id).toUInt(), t_newType, t_value);
	}
	t_item->setData(0, eValueItemRole_datatype, (uint32_t)t_newValue->dataType);
	t_item->setText(2, t_tablePackage.resValue2String(*t_newValue));
}
void QResArscEditor::onAddLocaleTriggered_slot(void)
{
	if (m_addLocaleDialog->exec() != QDialog::Accepted)
		return;

	QTreeWidgetItem* t_typeItem = m_TW_tree->currentItem();
	if (t_typeItem->data(0, eTreeItemRole_type).toUInt() == eTreeItemType_spec)
		t_typeItem = t_typeItem->parent();
	Q_ASSERT(t_typeItem->data(0, eTreeItemRole_type).toUInt() == eTreeItemType_type);

	QString t_packageName = t_typeItem->data(0, eTreeItemRole_package).toString();
	TTablePackage& t_tablePackage = m_Parser->tablePackage(t_packageName);
	quint32 t_typeid = t_typeItem->data(0, eTreeItemRole_typeid).toUInt();

	ResTable_config t_config;
	memset(&t_config, 0, sizeof(t_config));
	t_config.size = sizeof(t_config);
	m_addLocaleDialog->getLocale(t_config);

	for (int i = 0; i < t_typeItem->childCount(); ++i)
	{
		QTreeWidgetItem* t_Item = t_typeItem->child(i);
		quint32 t_specid = t_Item->data(0, eTreeItemRole_specid).toUInt();
		const TTableTypeEx& t_tableType = t_tablePackage.getTableType(t_typeid, t_specid);
		if (t_tableType.config == t_config)
		{
			QMessageBox::warning(this, tr("warning"), tr("This localized configuration already exists !"));
			return;
		}
	}

	t_tablePackage.addLocale(t_typeid, t_config, std::bind(&QResArscEditor::onRefreshTablePackageData, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
}
void QResArscEditor::onExportLocaleTriggered_slot(void)
{
	QString t_FileName = QFileDialog::getSaveFileName(this, tr("Get OutPut xml File"), m_BasePath,
		tr("xml File (*.xml)"), NULL, QFileDialog::DontConfirmOverwrite);
	if (t_FileName.isEmpty())
		return;
	m_BasePath = QFileInfo(t_FileName).absolutePath();
	if (m_TW_tree->currentItem() == NULL)
		return;
	QTreeWidgetItem* t_treeItem = m_TW_tree->currentItem();
	if (t_treeItem->data(0, eTreeItemRole_type).toUInt() != eTreeItemType_spec)
		return;

	QString t_packageName = t_treeItem->data(0, eTreeItemRole_package).toString();
	const TTablePackage& t_tablePackage = m_Parser->tablePackage(t_packageName);
	quint32 t_typeid = t_treeItem->data(0, eTreeItemRole_typeid).toUInt();
	quint32 t_specid = t_treeItem->data(0, eTreeItemRole_specid).toUInt();
	const TTableTypeEx& t_tableType = t_tablePackage.getTableType(t_typeid, t_specid);

	QFile t_WriteFile(t_FileName);
	if (!t_WriteFile.open(QFile::WriteOnly | QFile::Text))
	{
		QMessageBox::warning(this, tr("warning"), tr("file open failed !"));
		return;
	}

	QXmlStreamWriter t_xmlWriter(&t_WriteFile);
	t_xmlWriter.setAutoFormatting(true);
	t_xmlWriter.writeStartDocument();
	t_xmlWriter.writeStartElement("resources");
	for (int i = 0; i < t_tableType.entryValue.size(); ++i)
	{
		QSharedPointer<ResTable_entry> t_ptrEntry = t_tableType.entryValue[i];
		if (t_ptrEntry.isNull())
			continue;
		if ((*t_ptrEntry).size == sizeof(ResTable_entry))
		{
			TTableValueEntry* t_pValueEntry = reinterpret_cast<TTableValueEntry*>(t_ptrEntry.get());
			t_xmlWriter.writeStartElement("string");
			t_xmlWriter.writeAttribute("id", QString("0x%1").arg(0x7f000000 + (t_typeid << 16) + i, 8, 16, QChar('0')));
			t_xmlWriter.writeAttribute("name", t_tablePackage.getKeyString(t_pValueEntry->key.index));
			t_xmlWriter.writeAttribute("type", QString::number((uint32_t)t_pValueEntry->value.dataType));
			QString t_text = t_tablePackage.resValue2String(t_pValueEntry->value);
			if (t_text.indexOf("<") >= 0 || t_text.indexOf("&") >= 0 || t_text.indexOf("\"") >= 0)
				t_xmlWriter.writeCDATA(t_text);
			else
				t_xmlWriter.writeCharacters(t_text);
			t_xmlWriter.writeEndElement();
		}
		else
		{
			TTableMapEntry* t_pMapValue = reinterpret_cast<TTableMapEntry*>(t_ptrEntry.get());
			t_xmlWriter.writeStartElement("string-array");
			t_xmlWriter.writeAttribute("id", QString("0x%1").arg(0x7f000000 + (t_typeid << 16) + i, 8, 16, QChar('0')));
			t_xmlWriter.writeAttribute("name", t_tablePackage.getKeyString(t_pMapValue->key.index));
			for (quint32 j = 0; j < t_pMapValue->count; ++j)
			{
				ResTable_map& t_tableMap = t_pMapValue->tablemap[j];
				t_xmlWriter.writeStartElement("item");
				t_xmlWriter.writeAttribute("id", QString::number(j));
				t_xmlWriter.writeAttribute("name", t_tablePackage.getReferenceDestination(ResTable_config(), t_tableMap.name.indent));
				t_xmlWriter.writeAttribute("type", QString::number((uint32_t)t_tableMap.value.dataType));
				QString t_text = t_tablePackage.resValue2String(t_tableMap.value);
				if (t_text.indexOf("<") >= 0 || t_text.indexOf("&") >= 0 || t_text.indexOf("\"") >= 0)
					t_xmlWriter.writeCDATA(t_text);
				else
					t_xmlWriter.writeCharacters(t_text);
				t_xmlWriter.writeEndElement();
			}
			t_xmlWriter.writeEndElement();
		}
	}
	t_xmlWriter.writeEndElement();
	t_WriteFile.close();
	QMessageBox::information(this, tr("information"), tr("Export was successful !"));
}
static uint32_t QStringToUint(const QString& _str)
{
	if (_str.toLower().mid(0, 2) == "0x")
		return _str.mid(2).toUInt(NULL, 16);
	else
		return _str.toUInt();
}
void QResArscEditor::onImportLocaleTriggered_slot(void)
{
	QString t_FileName = QFileDialog::getOpenFileName(this, tr("Open xml File"), m_BasePath,
		tr("xml File (*.xml)"), NULL, QFileDialog::DontConfirmOverwrite);
	if (t_FileName.isEmpty())
		return;
	m_BasePath = QFileInfo(t_FileName).absolutePath();
	QFile t_ReadFile(t_FileName);
	t_ReadFile.open(QFile::ReadOnly);
	QDomDocument t_domTree;
	if (!t_domTree.setContent(&t_ReadFile))
	{
		t_ReadFile.close();
		QMessageBox::warning(this, tr("warning"), tr("The XML format of this file is incorrect !"));
		return;
	}

	QTreeWidgetItem* t_treeItem = m_TW_tree->currentItem();
	if (t_treeItem == NULL || t_treeItem->data(0, eTreeItemRole_type).toUInt() != eTreeItemType_spec)
		return;
	struct TValue
	{
		Res_value::_DataType type;
		QString data;
		TValue(Res_value::_DataType _type = Res_value::_DataType::TYPE_NULL, const QString& _data = QString()) :
			type(_type), data(_data) {
		};
	};
	//先将翻译文件读入map中
	QMap<QString, TValue> t_StringMap;
	QMap<QString, QVector<TValue>> t_arrayMap;
	QDomElement t_root = t_domTree.documentElement();
	for (QDomElement t_childDom = t_root.firstChildElement(); !t_childDom.isNull(); t_childDom = t_childDom.nextSiblingElement())
	{
		QString t_tagName = t_childDom.tagName();
		QString t_name = t_childDom.attribute("name");
		if (t_tagName == "string")
			t_StringMap.insert(t_name, TValue((Res_value::_DataType)t_childDom.attribute("type").toUInt(), t_childDom.text()));
		else if (t_tagName == "string-array")
			for (QDomElement t_valueDom = t_childDom.firstChildElement("item"); !t_valueDom.isNull(); t_valueDom = t_valueDom.nextSiblingElement("item"))
				t_arrayMap[t_name].append(TValue((Res_value::_DataType)t_valueDom.attribute("type").toUInt(), t_valueDom.text()));
	}
	t_ReadFile.close();
	//然后根据值的名字来查找翻译，这样即使后续版本值的ID发生了变化（ID是编译器自动产生的，不同版本，同一个值的ID不能保证不变），
	//只要值的名字不变(正常这个是写入代码，不会变)，就不会影响自动翻译。
	QString t_packageName = t_treeItem->data(0, eTreeItemRole_package).toString();
	TTablePackage& t_tablePackage = m_Parser->tablePackage(t_packageName);
	uint32_t t_typeid = t_treeItem->data(0, eTreeItemRole_typeid).toUInt();
	uint32_t t_specid = t_treeItem->data(0, eTreeItemRole_specid).toUInt();
	const TTableTypeEx& t_type = t_tablePackage.getTableType(t_typeid, t_specid);
	m_TW_value->clear();
	for (int i = 0; i < t_type.entryValue.size(); ++i)
	{
		QSharedPointer<ResTable_entry> t_ptrEntry = t_type.entryValue[i];
		if (t_ptrEntry.isNull())
			continue;
		if ((*t_ptrEntry).size == sizeof(ResTable_entry))
		{
			TTableValueEntry* t_pValueEntry = reinterpret_cast<TTableValueEntry*>(t_ptrEntry.get());
			QString t_name = t_tablePackage.getKeyString(t_pValueEntry->key.index);
			if (!t_StringMap.contains(t_name))
				continue;
			TValue& t_value = t_StringMap[t_name];
			if (t_value.type == Res_value::_DataType::TYPE_STRING)
				m_Parser->setValue(t_tablePackage, t_typeid, t_specid, uint32_t(i), t_value.data, false);
			else
				m_Parser->setValue(t_tablePackage, t_typeid, t_specid, uint32_t(i), t_value.type, QEditDialog::qstringToData(t_value.type, t_value.data));
		}
		else
		{
			TTableMapEntry* t_pMapValue = reinterpret_cast<TTableMapEntry*>(t_ptrEntry.get());
			QString t_name = t_tablePackage.getKeyString(t_pMapValue->key.index);
			if (!t_arrayMap.contains(t_name) || t_arrayMap[t_name].size() != t_pMapValue->tablemap.size())
				continue;
			QVector<TValue> t_values = t_arrayMap[t_name];
			for (int j = 0; j < t_pMapValue->tablemap.size(); ++j)
			{
				TValue& t_value = t_values[j];
				if (t_value.type == Res_value::_DataType::TYPE_STRING)
					m_Parser->setValue(t_tablePackage, t_typeid, t_specid, uint32_t(i), uint32_t(j), t_value.data, false);
				else
					m_Parser->setValue(t_tablePackage, t_typeid, t_specid, uint32_t(i), uint32_t(j), t_value.type, QEditDialog::qstringToData(t_value.type, t_value.data));
			}
		}
	}
	refreshResTableType(t_tablePackage, t_typeid, t_specid);
	QMessageBox::information(this, tr("information"), tr("Import was successful !"));
}
