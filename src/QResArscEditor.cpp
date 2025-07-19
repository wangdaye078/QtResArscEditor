#include "QResArscEditor.h"
#include <QComboBox>
#include <QDomDocument>
#include <QDomElement>
#include <QFileDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QTreeWidget>
#include <QXmlStreamWriter>

#include "common/basicDefine.h"
#include "QAddLocaleDialog.h"
#include "QAppendDialog.h"
#include "QEditDialog.h"
#include "QResArscParser.h"
#include "QTreeWidgetItem_ArscValue.h"
#include "SimpleRichText.h"
QResArscEditor::QResArscEditor(QWidget* _parent)
	: QResArscEditorUI(_parent), m_valueMenu(NULL), m_treeMenu(NULL), m_basePath(".")
{
	m_parser = new QResArscParser(this);
}

QResArscEditor::~QResArscEditor()
{
}
void QResArscEditor::onOpenReleased_Slot(void)
{
	QString t_FileName = QFileDialog::getOpenFileName(this, tr("Open ARSC File"), m_basePath,
		tr("ARSC File (*.arsc);;APK File (*.apk)"), NULL, QFileDialog::DontConfirmOverwrite);
	if (t_FileName.isEmpty())
		return;
	m_basePath = QFileInfo(t_FileName).absolutePath();
	m_LE_filePath->setText(t_FileName);
	m_parser->readFile(t_FileName);

	m_TW_tree->clear();
	m_parser->traversalAllTablePackage(std::bind(&QResArscEditor::onRefreshTablePackage, this, std::placeholders::_1, std::placeholders::_2));
}
void QResArscEditor::onSaveReleased_Slot(void)
{
	if (!m_parser->writeFile(m_LE_filePath->text()))
		QMessageBox::warning(this, tr("warning"), tr("The specified file cannot be written in !"));
	else
		QMessageBox::information(this, tr("information"), tr("File write completed !"));
}
void QResArscEditor::onSaveAsReleased_Slot(void)
{
	QString t_filter = m_LE_filePath->text().endsWith(".apk", Qt::CaseInsensitive) ? tr("ARSC File (*.arsc);;APK File (*.apk)") : tr("ARSC File (*.arsc)");

	QString t_FileName = QFileDialog::getSaveFileName(this, tr("OutPut ARSC File"), m_basePath,
		t_filter, NULL, QFileDialog::DontConfirmOverwrite);
	if (t_FileName.isEmpty())
		return;

	if (t_FileName.endsWith(".apk", Qt::CaseInsensitive) && !QFile::copy(m_LE_filePath->text(), t_FileName))
	{
		QMessageBox::warning(this, tr("warning"), tr("The specified file cannot be written in !"));
		return;
	}

	m_basePath = QFileInfo(t_FileName).absolutePath();
	m_LE_filePath->setText(t_FileName);
	if (!m_parser->writeFile(t_FileName))
		QMessageBox::warning(this, tr("warning"), tr("The specified file cannot be written in !"));
	else
		QMessageBox::information(this, tr("information"), tr("File write completed !"));
}
void QResArscEditor::onTreeCurrentItemChanged_slot(QTreeWidgetItem* _current, QTreeWidgetItem* _previous)
{
	if (_current == NULL)
		return;
	if (_current->data(0, eTreeItemRole_type).toUInt() != eTreeItemType_spec)
		return;
	m_TW_value->clear();

	QTreeWidgetItem* t_typeItem = _current->parent();
	Q_ASSERT(t_typeItem->data(0, eTreeItemRole_type).toUInt() == eTreeItemType_type);
	uint32_t t_typeId = t_typeItem->data(0, eTreeItemRole_typeid).toUInt();
	QTreeWidgetItem* t_packageItem = t_typeItem->parent();
	Q_ASSERT(t_packageItem->data(0, eTreeItemRole_type).toUInt() == eTreeItemType_package);
	PTablePackage t_pTablePackage = t_packageItem->data(0, eTreeItemRole_value).value<PTablePackage>();
	PSpecificData t_specData = _current->data(0, eTreeItemRole_value).value<PSpecificData>();
	t_specData->traversalData(std::bind(&QResArscEditor::onRefreshSpecificData, this, t_pTablePackage, t_typeId, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}
void QResArscEditor::onShowValueContextMenu_slot(const QPoint& _pos)
{
	QTreeWidgetItem* t_specItem = m_TW_tree->currentItem();
	if (t_specItem == NULL)
		return;
	if (t_specItem->data(0, eTreeItemRole_type).toUInt() != eTreeItemType_spec)
		return;
	//如果是0的话，基本就是默认适配，不能随便增删
	int t_specIndex = t_specItem->parent()->indexOfChild(t_specItem);
	delete m_valueMenu;
	m_valueMenu = new QMenu(m_TW_value);
	m_valueMenu->setObjectName(QString::fromUtf8("m_valueMenu"));
	//默认的那些值不能删
	if (t_specIndex != 0)
		m_valueMenu->addAction(m_AC_AddValue);
	if (t_specIndex != 0)
		m_valueMenu->addAction(m_AC_AddAllValue);
	QTreeWidgetItem* t_valueItem = m_TW_value->currentItem();
	//数组中的单项不能删
	if (t_specIndex != 0 && t_valueItem != NULL && t_valueItem->data(0, eValueItemRole_type).toUInt() != eValueItemType_arrayitem)
		m_valueMenu->addAction(m_AC_DeleteValue);
	//数组本身不能编辑，只能编辑子项
	if (/*t_specIndex != 0 && */t_valueItem != NULL && t_valueItem->data(0, eValueItemRole_type).toUInt() != eValueItemType_array)
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

	PTableType t_tableType = t_typeItem->data(0, eTreeItemRole_value).value<PTableType>();
	const QVector<uint32_t>& t_configMasks = t_tableType->getConfigMask();
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
	m_treeMenu->addAction(m_AC_PrintPublicStrings);
	m_treeMenu->popup(QCursor::pos());
}
QTreeWidgetItem* onTraversalDefaultSpecificData(const QSet<uint32_t>& _maskIdx, QMap<uint32_t, uint32_t>* _keyIndexs, QTreeWidgetItem* _parent, uint32_t _idx, EValueItemType _type, const QVariant& _v)
{
	if (!_maskIdx.contains(_idx))
		return NULL;
	if (_type == eValueItemType_arrayitem)
		return NULL;
	IResValue* t_Entry = _v.value<PResValue>().get();
	_keyIndexs->insert(_idx, t_Entry->getKeyIndex());
	return NULL;
}
QTreeWidgetItem* onTraversalSpecificData(const QSet<uint32_t>& _maskIdx, QMap<uint32_t, uint32_t>* _keyIndexs, QTreeWidgetItem* _parent, uint32_t _idx, EValueItemType _type, const QVariant& _v)
{
	if (!_maskIdx.contains(_idx))
		return NULL;
	if (_type == eValueItemType_arrayitem)
		return NULL;
	//ResTable_value_RW* t_Entry = _v.value<PResTable_value_RW>().get();
	_keyIndexs->remove(_idx);
	return NULL;
}
QMap<uint32_t, uint32_t> findMissingItems(const PTableType& _tableType, const PSpecificData& _specData, const PSpecificData& _defaultSpecData)
{
	const QVector<uint32_t>& t_configMasks = _tableType->getConfigMask();
	uint32_t t_configMask = getTableConfigMask(_specData->getTType().config);
	QSet<uint32_t> t_maskIdx;
	for (int i = 0; i < t_configMasks.size(); ++i)
	{
		if ((t_configMasks[i] & t_configMask) == t_configMask)
			t_maskIdx.insert(i);
	}
	QMap<uint32_t, uint32_t> t_keyIndexs;
	_defaultSpecData->traversalData(std::bind(&onTraversalDefaultSpecificData, t_maskIdx, &t_keyIndexs, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	_specData->traversalData(std::bind(&onTraversalSpecificData, t_maskIdx, &t_keyIndexs, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	return t_keyIndexs;
}
void QResArscEditor::onAddValueTriggered_slot(void)
{
	QTreeWidgetItem* t_specItem = m_TW_tree->currentItem();
	if (t_specItem == NULL)
		return;
	if (t_specItem->data(0, eTreeItemRole_type).toUInt() != eTreeItemType_spec)
		return;
	QTreeWidgetItem* t_typeItem = t_specItem->parent();
	QTreeWidgetItem* t_packageItem = t_typeItem->parent();
	QTreeWidgetItem* t_defaultSpecItem = t_typeItem->child(0);

	uint32_t t_typeId = t_typeItem->data(0, eTreeItemRole_typeid).toUInt();
	PTableType t_tableType = t_typeItem->data(0, eTreeItemRole_value).value<PTableType>();
	PSpecificData t_specData = t_specItem->data(0, eTreeItemRole_value).value<PSpecificData>();
	PSpecificData t_defaultSpecData = t_defaultSpecItem->data(0, eTreeItemRole_value).value<PSpecificData>();
	PTablePackage t_tablePackage = t_packageItem->data(0, eTreeItemRole_value).value<PTablePackage>();

	QMap<uint32_t, uint32_t> t_keyIndexs = findMissingItems(t_tableType, t_specData, t_defaultSpecData);

	if (t_keyIndexs.size() == 0)
	{
		QMessageBox::information(this, tr("information"), tr("no items found to add !"));
		return;
	}
	m_appendDialog->m_CB_Name->clear();
	for (QMap<uint32_t, uint32_t>::iterator i = t_keyIndexs.begin(); i != t_keyIndexs.end(); ++i)
	{
		uint32_t t_ID = 0x7F000000 + (t_typeId << 16) + i.key();
		m_appendDialog->m_CB_Name->addItem(QString("(0x%1)%2").arg(t_ID, 8, 16, QChar('0')).arg(t_tablePackage->getKeyString(QString(), false, i.value())), t_ID);
	}
	if (m_appendDialog->exec() == QDialog::Accepted)
	{
		uint32_t t_idx = m_appendDialog->m_CB_Name->currentData().toUInt() & 0xFFFF;
		t_specData->copyValue(*t_defaultSpecData.get(), t_idx);
		m_TW_value->clear();
		t_specData->traversalData(std::bind(&QResArscEditor::onRefreshSpecificData, this, t_tablePackage, t_typeId, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	}
}
void QResArscEditor::onAddAllValueTriggered_slot(void)
{
	QTreeWidgetItem* t_specItem = m_TW_tree->currentItem();
	if (t_specItem == NULL)
		return;
	if (t_specItem->data(0, eTreeItemRole_type).toUInt() != eTreeItemType_spec)
		return;
	QTreeWidgetItem* t_typeItem = t_specItem->parent();
	QTreeWidgetItem* t_packageItem = t_typeItem->parent();
	QTreeWidgetItem* t_defaultSpecItem = t_typeItem->child(0);

	uint32_t t_typeId = t_typeItem->data(0, eTreeItemRole_typeid).toUInt();
	PTableType t_tableType = t_typeItem->data(0, eTreeItemRole_value).value<PTableType>();
	PSpecificData t_specData = t_specItem->data(0, eTreeItemRole_value).value<PSpecificData>();
	PSpecificData t_defaultSpecData = t_defaultSpecItem->data(0, eTreeItemRole_value).value<PSpecificData>();
	PTablePackage t_tablePackage = t_packageItem->data(0, eTreeItemRole_value).value<PTablePackage>();

	QMap<uint32_t, uint32_t> t_keyIndexs = findMissingItems(t_tableType, t_specData, t_defaultSpecData);

	if (t_keyIndexs.size() == 0)
	{
		QMessageBox::information(this, tr("information"), tr("no items found to add !"));
		return;
	}
	for (QMap<uint32_t, uint32_t>::iterator i = t_keyIndexs.begin(); i != t_keyIndexs.end(); ++i)
	{
		t_specData->copyValue(*t_defaultSpecData.get(), i.key());
	}
	m_TW_value->clear();
	t_specData->traversalData(std::bind(&QResArscEditor::onRefreshSpecificData, this, t_tablePackage, t_typeId, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}
void QResArscEditor::onDeleteValueTriggered_slot(void)
{
	QTreeWidgetItem* t_specItem = m_TW_tree->currentItem();
	if (t_specItem == NULL)
		return;
	if (t_specItem->data(0, eTreeItemRole_type).toUInt() != eTreeItemType_spec)
		return;
	QTreeWidgetItem* t_typeItem = t_specItem->parent();
	QTreeWidgetItem* t_packageItem = t_typeItem->parent();

	//如果是0的话，基本就是默认适配，不能随便增删
	int t_specIndex = t_typeItem->indexOfChild(t_specItem);
	QTreeWidgetItem* t_valueItem = m_TW_value->currentItem();
	//数组的子项不能随便删除
	if (t_specIndex == 0 || t_valueItem == NULL || t_valueItem->data(0, eValueItemRole_type).toUInt() == eValueItemType_arrayitem)
		return;
	if (QMessageBox::question(this, tr("warning"), tr("Are you sure you want to delete this item?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
		return;

	PSpecificData t_specData = t_specItem->data(0, eTreeItemRole_value).value<PSpecificData>();
	PTablePackage t_tablePackage = t_packageItem->data(0, eTreeItemRole_value).value<PTablePackage>();
	uint32_t t_typeId = t_typeItem->data(0, eTreeItemRole_typeid).toUInt();

	uint32_t t_id = t_valueItem->data(0, eValueItemRole_id).toUInt();
	t_specData->deleteValue(t_id);

	m_TW_value->clear();
	t_specData->traversalData(std::bind(&QResArscEditor::onRefreshSpecificData, this, t_tablePackage, t_typeId, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}
void QResArscEditor::onEditValueTriggered_slot(void)
{
	QTreeWidgetItem* t_specItem = m_TW_tree->currentItem();
	if (t_specItem == NULL)
		return;
	if (t_specItem->data(0, eTreeItemRole_type).toUInt() != eTreeItemType_spec)
		return;
	QTreeWidgetItem* t_typeItem = t_specItem->parent();
	QTreeWidgetItem* t_packageItem = t_typeItem->parent();

	QTreeWidgetItem* t_valueItem = m_TW_value->currentItem();
	if (t_valueItem == NULL || t_valueItem->data(0, eValueItemRole_type).toUInt() == eValueItemType_array)
		return;

	PSpecificData t_specData = t_specItem->data(0, eTreeItemRole_value).value<PSpecificData>();
	PTablePackage t_tablePackage = t_packageItem->data(0, eTreeItemRole_value).value<PTablePackage>();
	uint32_t t_typeId = t_typeItem->data(0, eTreeItemRole_typeid).toUInt();
	PResValue t_pEntry = t_valueItem->data(0, eValueItemRole_entry).value<PResValue>();
	uint32_t t_dataType = t_valueItem->data(0, eValueItemRole_datatype).toUInt();
	uint32_t t_data = t_valueItem->data(0, eValueItemRole_data).toUInt();
	QString t_originalString = t_valueItem->text(2);

	m_editDialog->m_LE_ID->setText(t_valueItem->text(0));
	m_editDialog->m_LE_Name->setText(t_valueItem->text(1));
	m_editDialog->setTablePackage(t_tablePackage.get());

	m_editDialog->setData(t_specData->getTType().config, t_dataType, t_data, t_originalString);
	if (m_editDialog->exec() != QDialog::Accepted)
		return;
	Res_value t_value;
	t_value.dataType = (Res_value::_DataType)m_editDialog->getType();

	if (((uint32_t)t_value.dataType == t_dataType) &&
		(((t_dataType != (uint32_t)Res_value::_DataType::TYPE_STRING) && (t_data == m_editDialog->getData())) ||
			((t_dataType == (uint32_t)Res_value::_DataType::TYPE_STRING) && (t_originalString == m_editDialog->getSData()))))
	{
		QMessageBox::information(this, tr("warning"), tr("No changes were found !"), QMessageBox::Close, QMessageBox::Close);
		return;
	}

	if (t_value.dataType == Res_value::_DataType::TYPE_STRING)
	{
		QString t_newString = m_editDialog->getSData();
		PArscRichString t_originalRich = t_pEntry->getValue(NULL);
		//如果强制全部修改相同字符串的引用，那办法就是从原来的QStringPool中取出PArscRichString，然后修改成新内容后重新插入，那所有使用这个PArscRichString的内容就都改变了。
		//但是如果新的字符串和原来的某个一样，那就会存在两个内容一样的字符串，所以这时候只能把所有旧字符串的引用都修改。为了找出来所有引用，没办法只能搞了自己的shared_ptr。
		bool t_force = false;
		if (t_dataType == (uint32_t)Res_value::_DataType::TYPE_STRING && t_originalRich.use_count() > (1 + 1 + LEAST_REF_COUNT))	//t_originalRich用1次，t_pValue里也用一次
			if (QMessageBox::question(this, tr("warning"), tr("The old string number of citations > 1, chang all string?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
				t_force = true;
		PArscRichString t_rich(decodeRichText(t_newString.replace(QString("\\n"), QChar(0x0A))));
		if (t_force)
		{
			g_publicStrPool->removeRichString(t_originalRich);
			if (g_publicStrPool->getRefCount(t_rich) > 0)
			{
				t_rich = g_publicStrPool->getRichString(t_rich);
				std::set<PArscRichString*> t_use_ref = t_originalRich.use_ref();
				for (std::set<PArscRichString*>::iterator i = t_use_ref.begin(); i != t_use_ref.end(); ++i)
					**i = t_rich;
			}
			else
			{
				t_originalRich->string = t_rich->string;
				t_originalRich->styles = t_rich->styles;
				g_publicStrPool->insertRichString(t_originalRich);
			}
			//其实到这，因为都是直接修改的PArscRichString的值，已经不需要t_pEntry->setValue(t_value);了，但是这时候t_pEntry->data是不准确的，但是对于
			//dataType == Res_value::_DataType::TYPE_STRING的情况，我们都是以内部的PArscRichString为准，并不以data为重
		}
		else
		{
			t_rich = g_publicStrPool->getRichString(t_rich);
			t_value.data = t_rich->guid;
			t_pEntry->setValue(t_value);
		}
	}
	else
	{
		t_value.data = m_editDialog->getData();
		t_pEntry->setValue(t_value);
	}
	m_TW_value->clear();
	t_specData->traversalData(std::bind(&QResArscEditor::onRefreshSpecificData, this, t_tablePackage, t_typeId, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}
void QResArscEditor::onAddLocaleTriggered_slot(void)
{
	if (m_addLocaleDialog->exec() != QDialog::Accepted)
		return;

	QTreeWidgetItem* t_typeItem = m_TW_tree->currentItem();
	if (t_typeItem->data(0, eTreeItemRole_type).toUInt() == eTreeItemType_spec)
		t_typeItem = t_typeItem->parent();
	Q_ASSERT(t_typeItem->data(0, eTreeItemRole_type).toUInt() == eTreeItemType_type);

	uint32_t t_typeId = t_typeItem->data(0, eTreeItemRole_typeid).toUInt();
	QTreeWidgetItem* t_defaultSpecItem = t_typeItem->child(0);
	QTreeWidgetItem* t_packageItem = t_typeItem->parent();
	PTableType t_tableType = t_typeItem->data(0, eTreeItemRole_value).value<PTableType>();
	PTablePackage t_tablePackage = t_packageItem->data(0, eTreeItemRole_value).value<PTablePackage>();

	ResTable_config t_config;
	memset(&t_config, 0, sizeof(t_config));
	t_config.size = sizeof(t_config);
	m_addLocaleDialog->getLocale(t_config);

	QString t_localName = tableConfig2String(t_tablePackage->getTypeString(t_typeId - 1), t_config);
	for (QTreeWidgetItemIterator i(t_typeItem); *i != NULL; ++i)
	{
		if ((*i)->text(0) == t_localName)
		{
			QMessageBox::warning(this, tr("warning"), tr("This localized configuration already exists !"));
			return;
		}
	}

	PSpecificData t_specData = t_tableType->addLocale(t_config);
	QString t_packageName = WCHARToQString(t_tablePackage->packageInfo().name);
	QString t_typeName = t_tablePackage->getTypeString(t_typeId - 1);
	onRefreshTablePackageData(t_packageName, eTreeItemType_spec, t_typeId, tableConfig2String(t_typeName, t_specData->getType().config), QVariant::fromValue(t_specData));

	PSpecificData t_defaultSpecData = t_defaultSpecItem->data(0, eTreeItemRole_value).value<PSpecificData>();

	QMap<uint32_t, uint32_t> t_keyIndexs = findMissingItems(t_tableType, t_specData, t_defaultSpecData);
	for (QMap<uint32_t, uint32_t>::iterator i = t_keyIndexs.begin(); i != t_keyIndexs.end(); ++i)
	{
		t_specData->copyValue(*t_defaultSpecData.get(), i.key());
	}
}
bool g_inArray = false;
QTreeWidgetItem* onWriteSpecificData(QXmlStreamWriter* _xmlWriter, const PTablePackage& _package, uint32_t _typeId, QTreeWidgetItem* _parent, uint32_t _idx, EValueItemType _type, const QVariant& _v)
{
	if (g_inArray && _type != eValueItemType_arrayitem)
	{
		g_inArray = false;
		_xmlWriter->writeEndElement();
	}
	switch (_type)
	{
	case eValueItemType_value:
		{
			TTableValueEntry* t_pValueEntry = reinterpret_cast<TTableValueEntry*>(_v.value<PResValue>().get());
			_xmlWriter->writeStartElement("value");
			_xmlWriter->writeAttribute("id", QString("0x%1").arg(0x7f000000 + (_typeId << 16) + _idx, 8, 16, QChar('0')));
			_xmlWriter->writeAttribute("name", _package->getKeyString(QString(), false, t_pValueEntry->getKeyIndex()));
			_xmlWriter->writeAttribute("type", QString::number((uint32_t)t_pValueEntry->value.dataType));
			QString t_text = resValue2String(t_pValueEntry->value, t_pValueEntry->svalue);
			if (t_text.indexOf("<") >= 0 || t_text.indexOf("&") >= 0 || t_text.indexOf("\"") >= 0)
				_xmlWriter->writeCDATA(t_text);
			else
				_xmlWriter->writeCharacters(t_text);
			_xmlWriter->writeEndElement();
		}
		break;
	case eValueItemType_array:
		{
			TTableMapEntry* t_pMapValue = reinterpret_cast<TTableMapEntry*>(_v.value<PResValue>().get());
			_xmlWriter->writeStartElement("array");
			_xmlWriter->writeAttribute("id", QString("0x%1").arg(0x7f000000 + (_typeId << 16) + _idx, 8, 16, QChar('0')));
			_xmlWriter->writeAttribute("name", _package->getKeyString(QString(), false, t_pMapValue->getKeyIndex()));
			if (t_pMapValue->entry.parent.ident != 0)
				_xmlWriter->writeAttribute("parent", _package->getKeyString("@", true, t_pMapValue->entry.parent.ident));
			g_inArray = true;
		}
		break;
	case eValueItemType_arrayitem:
		{
			ResTable_pairs* t_pPairs = reinterpret_cast<ResTable_pairs*>(_v.value<PResValue>().get());
			_xmlWriter->writeStartElement("item");
			_xmlWriter->writeAttribute("id", QString::number(_idx));
			_xmlWriter->writeAttribute("name", _package->getKeyString(QString(), false, t_pPairs->getKeyIndex()));
			_xmlWriter->writeAttribute("type", QString::number((uint32_t)t_pPairs->value.dataType));
			QString t_text = resValue2String(t_pPairs->value, t_pPairs->svalue);
			if (t_text.indexOf("<") >= 0 || t_text.indexOf("&") >= 0 || t_text.indexOf("\"") >= 0)
				_xmlWriter->writeCDATA(t_text);
			else
				_xmlWriter->writeCharacters(t_text);
			_xmlWriter->writeEndElement();
		}
		break;
	}
	return NULL;
}
void QResArscEditor::onExportLocaleTriggered_slot(void)
{
	QTreeWidgetItem* t_specItem = m_TW_tree->currentItem();
	if (t_specItem == NULL)
		return;
	if (t_specItem->data(0, eTreeItemRole_type).toUInt() != eTreeItemType_spec)
		return;

	QString t_FileName = QFileDialog::getSaveFileName(this, tr("Get OutPut xml File"), m_basePath,
		tr("xml File (*.xml)"), NULL, QFileDialog::DontConfirmOverwrite);
	if (t_FileName.isEmpty())
		return;
	m_basePath = QFileInfo(t_FileName).absolutePath();

	QFile t_WriteFile(t_FileName);
	if (!t_WriteFile.open(QFile::WriteOnly | QFile::Text))
	{
		QMessageBox::warning(this, tr("warning"), tr("file open failed !"));
		return;
	}
	QTreeWidgetItem* t_typeItem = t_specItem->parent();
	QTreeWidgetItem* t_packageItem = t_typeItem->parent();
	PSpecificData t_specData = t_specItem->data(0, eTreeItemRole_value).value<PSpecificData>();
	PTablePackage t_tablePackage = t_packageItem->data(0, eTreeItemRole_value).value<PTablePackage>();
	uint32_t t_typeId = t_typeItem->data(0, eTreeItemRole_typeid).toUInt();

	QXmlStreamWriter t_xmlWriter(&t_WriteFile);
	t_xmlWriter.setAutoFormatting(true);
	t_xmlWriter.writeStartDocument();
	t_xmlWriter.writeStartElement("resources");
	g_inArray = false;
	t_specData->traversalData(std::bind(&onWriteSpecificData, &t_xmlWriter, t_tablePackage, t_typeId, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	if (g_inArray)
		t_xmlWriter.writeEndElement();
	t_xmlWriter.writeEndElement();
	t_WriteFile.close();
	QMessageBox::information(this, tr("information"), tr("Export was successful !"));
}
struct TTmpValue
{
	Res_value::_DataType type;
	QString data;
	TTmpValue(Res_value::_DataType _type = Res_value::_DataType::TYPE_NULL, const QString& _data = QString()) :
		type(_type), data(_data) {
	};
};
typedef QMap<QString, TTmpValue> TValueMap;
typedef QMap<QString, TValueMap> TArrayMap;

QTreeWidgetItem* onImportSpecificData(const TValueMap& _valueMap, const TArrayMap& _arrayMap, const PTablePackage& _package, uint32_t _typeId, QTreeWidgetItem* _parent, uint32_t _idx, EValueItemType _type, const QVariant& _v)
{
	static QString t_arrayName;
	IResValue* t_pEntry = _v.value<PResValue>().get();
	QString t_name = _package->getKeyString(QString(), false, t_pEntry->getKeyIndex());
	const TValueMap* t_valueMap = &_valueMap;
	switch (_type)
	{
	case eValueItemType_array:
		t_arrayName = t_name;
		break;
	case eValueItemType_arrayitem:
		if (!_arrayMap.contains(t_arrayName))
			return NULL;
		t_valueMap = &(_arrayMap.find(t_arrayName).value());
	case eValueItemType_value:
		{
			if (!t_valueMap->contains(t_name))
				return NULL;
			TTmpValue t_tmpValue = (*t_valueMap)[t_name];
			Res_value t_value;
			t_value.dataType = (Res_value::_DataType)t_tmpValue.type;
			if (t_value.dataType == Res_value::_DataType::TYPE_STRING)
			{
				PArscRichString t_rich(decodeRichText(t_tmpValue.data.replace(QString("\\n"), QChar(0x0A))));
				t_rich = g_publicStrPool->getRichString(t_rich);
				t_value.data = t_rich->guid;
			}
			else
				t_value.data = QEditDialog::qstringToData(t_tmpValue.type, t_tmpValue.data);
			t_pEntry->setValue(t_value);
		}
		break;
	}
	return NULL;
}
void QResArscEditor::onImportLocaleTriggered_slot(void)
{
	QTreeWidgetItem* t_specItem = m_TW_tree->currentItem();
	if (t_specItem == NULL || t_specItem->data(0, eTreeItemRole_type).toUInt() != eTreeItemType_spec)
		return;
	QTreeWidgetItem* t_typeItem = t_specItem->parent();
	QTreeWidgetItem* t_packageItem = t_typeItem->parent();
	PSpecificData t_specData = t_specItem->data(0, eTreeItemRole_value).value<PSpecificData>();
	PTablePackage t_tablePackage = t_packageItem->data(0, eTreeItemRole_value).value<PTablePackage>();
	uint32_t t_typeId = t_typeItem->data(0, eTreeItemRole_typeid).toUInt();


	QString t_FileName = QFileDialog::getOpenFileName(this, tr("Open xml File"), m_basePath,
		tr("xml File (*.xml)"), NULL, QFileDialog::DontConfirmOverwrite);
	if (t_FileName.isEmpty())
		return;
	m_basePath = QFileInfo(t_FileName).absolutePath();
	QFile t_ReadFile(t_FileName);
	t_ReadFile.open(QFile::ReadOnly);
	QDomDocument t_domTree;
	if (!t_domTree.setContent(&t_ReadFile))
	{
		t_ReadFile.close();
		QMessageBox::warning(this, tr("warning"), tr("The XML format of this file is incorrect !"));
		return;
	}
	TValueMap t_valueMap;
	TArrayMap t_arrayMap;
	QDomElement t_root = t_domTree.documentElement();
	for (QDomElement t_childDom = t_root.firstChildElement(); !t_childDom.isNull(); t_childDom = t_childDom.nextSiblingElement())
	{
		QString t_tagName = t_childDom.tagName();
		QString t_name = t_childDom.attribute("name");
		if (t_tagName == "value")
			t_valueMap.insert(t_name, TTmpValue((Res_value::_DataType)t_childDom.attribute("type").toUInt(), t_childDom.text()));
		else if (t_tagName == "array")
			for (QDomElement t_valueDom = t_childDom.firstChildElement("item"); !t_valueDom.isNull(); t_valueDom = t_valueDom.nextSiblingElement("item"))
				t_arrayMap[t_name].insert(t_valueDom.attribute("name"), TTmpValue((Res_value::_DataType)t_valueDom.attribute("type").toUInt(), t_valueDom.text()));
	}
	t_ReadFile.close();
	t_specData->traversalData(std::bind(&onImportSpecificData, t_valueMap, t_arrayMap, t_tablePackage, t_typeId, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	m_TW_value->clear();
	t_specData->traversalData(std::bind(&QResArscEditor::onRefreshSpecificData, this, t_tablePackage, t_typeId, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	QMessageBox::information(this, tr("information"), tr("Import was successful !"));
}
void QResArscEditor::onPrintPublicStringsTriggered_slot(void)
{
	g_publicStrPool->printRefCount();
}
void QResArscEditor::onRefreshTablePackage(const QString& _packageName, const PTablePackage& _package)
{
	QTreeWidgetItem* t_packageItem = new QTreeWidgetItem(m_TW_tree);
	t_packageItem->setText(0, _packageName);
	t_packageItem->setData(0, eTreeItemRole_type, eTreeItemType_package);
	t_packageItem->setData(0, eTreeItemRole_package, _packageName);
	t_packageItem->setData(0, eTreeItemRole_value, QVariant::fromValue(_package));
	_package->traversalData(std::bind(&QResArscEditor::onRefreshTablePackageData, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
	t_packageItem->setExpanded(true);
}
void QResArscEditor::onRefreshTablePackageData(const QString& _packageName, ETreeItemType _type, uint32_t _typeId, const QString& _name, const QVariant& _v)
{
	QList<QTreeWidgetItem*> t_packageItems = m_TW_tree->findItems(_packageName, Qt::MatchFixedString);
	Q_ASSERT(t_packageItems.size() == 1);
	QTreeWidgetItem* t_Item = t_packageItems[0];

	if (_type == eTreeItemType_spec)
	{
		for (int i = 0; i < t_Item->childCount(); ++i)
		{
			if (t_Item->child(i)->data(0, eTreeItemRole_typeid) == _typeId)
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
	t_subItem->setData(0, eTreeItemRole_typeid, _typeId);
	t_subItem->setData(0, eTreeItemRole_value, _v);
}
void widgetItemSetData(QTreeWidgetItem* _item, EValueItemType _type, uint32_t _data, uint32_t _dataType, int _id, const QString& _text, const QVariant& _v)
{
	_item->setData(0, eValueItemRole_type, _type);
	_item->setData(0, eValueItemRole_datatype, _dataType);
	_item->setData(0, eValueItemRole_data, _data);
	_item->setData(0, eValueItemRole_id, _id);
	_item->setData(0, eValueItemRole_entry, _v);
	_item->setText(0, _text);
}
QTreeWidgetItem* QResArscEditor::onRefreshSpecificData(const PTablePackage& _package, uint32_t _typeId, QTreeWidgetItem* _parent, uint32_t _idx, EValueItemType _type, const QVariant& _v)
{
	QTreeWidgetItem* t_TreeWidgetItem = (_parent == NULL) ? new QTreeWidgetItem_ArscValue(m_TW_value) : new QTreeWidgetItem_ArscValue(_parent);
	t_TreeWidgetItem->setData(0, eValueItemRole_package, QVariant::fromValue(_package));
	IResValue* t_Entry = _v.value<PResValue>().get();
	t_TreeWidgetItem->setText(1, _package->getKeyString(QString(), false, t_Entry->getKeyIndex()));
	t_TreeWidgetItem->setToolTip(1, QString("0x%1").arg(t_Entry->getKeyIndex(), 8, 16, QChar('0')));

	switch (_type)
	{
	case eValueItemType_value:
		{
			TTableValueEntry* t_pValueEntry = reinterpret_cast<TTableValueEntry*>(_v.value<PResValue>().get());
			widgetItemSetData(t_TreeWidgetItem, _type, t_pValueEntry->value.data, (uint32_t)t_pValueEntry->value.dataType,
				_idx, QString("0x7f%1%2").arg(_typeId, 2, 16, QChar('0')).arg(_idx, 4, 16, QChar('0')), _v);
			t_TreeWidgetItem->setText(2, resValue2String(t_pValueEntry->value, t_pValueEntry->svalue));
		}
		break;
	case eValueItemType_array:
		{
			TTableMapEntry* t_pMapEntry = reinterpret_cast<TTableMapEntry*>(_v.value<PResValue>().get());
			widgetItemSetData(t_TreeWidgetItem, _type, t_pMapEntry->entry.parent.ident, (uint32_t)Res_value::_DataType::TYPE_NULL,
				_idx, QString("0x7f%1%2").arg(_typeId, 2, 16, QChar('0')).arg(_idx, 4, 16, QChar('0')), _v);
			if (t_pMapEntry->entry.parent.ident != 0)
				t_TreeWidgetItem->setText(2, QString("@0x%1").arg(t_pMapEntry->entry.parent.ident, 8, 16, QChar('0')));
		}
		break;
	case eValueItemType_arrayitem:
		{
			ResTable_pairs* t_pPairs = reinterpret_cast<ResTable_pairs*>(_v.value<PResValue>().get());
			widgetItemSetData(t_TreeWidgetItem, _type, t_pPairs->value.data, (uint32_t)t_pPairs->value.dataType,
				_idx, QString("0x%1").arg(t_pPairs->key.ident, 8, 16, QChar('0')), _v);
			t_TreeWidgetItem->setText(2, resValue2String(t_pPairs->value, t_pPairs->svalue));
		}
		break;
	}

	return t_TreeWidgetItem;
}


