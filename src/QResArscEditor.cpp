#include "common/basicDefine.h"
#include "QAddLocaleDialog.h"
#include "QAndroidAttribute.h"
#include "QAppendDialog.h"
#include "QEditDialog.h"
#include "QManifestParser.h"
#include "QResArscEditor.h"
#include "QResArscParser.h"
#include "QTreeWidgetItem_ArscValue.h"
#include "SimpleRichText.h"
#include <QComboBox>
#include <QDebug>
#include <QDomDocument>
#include <QDomElement>
#include <QFileDialog>
#include <QInputDialog> 
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QToolButton>
#include <QTreeWidget>
#include <QXmlStreamWriter>
QResArscEditor::QResArscEditor(QWidget* _parent)
	: QResArscEditorUI(_parent), m_parser(NULL), m_valueMenu(NULL), m_treeMenu(NULL), m_basePath(".")
{
	g_publicFinal = new QPublicFinal(this);
	g_androidAttribute = new QAndroidAttribute(this);
}

QResArscEditor::~QResArscEditor()
{

}
void QResArscEditor::onOpenReleased_Slot(void)
{
	if (m_parser != NULL)
		delete m_parser;
	QString t_selectedFilter;
	QString t_FileName = QFileDialog::getOpenFileName(this, tr("Open ARSC File"), m_basePath,
		tr("ARSC File (*.arsc *.apk);;xml bin File (*.xml *.apk)"),
		&t_selectedFilter, QFileDialog::DontConfirmOverwrite);
	if (t_FileName.isEmpty())
		return;
	if (t_selectedFilter == tr("ARSC File (*.arsc *.apk)"))
		m_parser = new QResArscParser(this);
	else if (t_selectedFilter == tr("xml bin File (*.xml *.apk)"))
		m_parser = new QManifestParser(this);
	else
		Q_ASSERT(false);
	qDebug() << "Open File:" << t_FileName;
	m_basePath = QFileInfo(t_FileName).absolutePath();
	m_LE_filePath->setText(t_FileName);
	bool t_readOk = m_parser->readFile(t_FileName);

	m_TW_tree->clear();

	if (t_readOk)
	{
		if (m_parser->getParserType() == RES_TYPE::RES_TABLE_TYPE)
			reinterpret_cast<QResArscParser*>(m_parser)->setTraversalAllTablePackageFunc(std::bind(&QResArscEditor::onRefreshTablePackage, this, std::placeholders::_1, std::placeholders::_2));
		else if (m_parser->getParserType() == RES_TYPE::RES_XML_TYPE)
			reinterpret_cast<QManifestParser*>(m_parser)->setTraversalAllXmlElementFunc(std::bind(&QResArscEditor::onRefreshXmlElement, this, std::placeholders::_1, std::placeholders::_2));
		m_parser->traversalSubItems();
		m_TB_save->setEnabled(true);
		m_TB_saveas->setEnabled(true);
		QMessageBox::information(this, tr("information"), tr("File read completed !"));
	}
	else
		QMessageBox::warning(this, tr("warning"), tr("The specified file read error !"));
}
void QResArscEditor::onSaveReleased_Slot(void)
{
	qDebug() << "Save File:" << m_LE_filePath->text();
	if (!m_parser->writeFile(m_LE_filePath->text()))
		QMessageBox::warning(this, tr("warning"), tr("The specified file cannot be written in !"));
	else
	{
		QMessageBox::information(this, tr("information"), tr("File write completed !"));
		if (m_LE_filePath->text().endsWith(".apk", Qt::CaseInsensitive)) //如果是APK文件，提示需要重新签名
			QMessageBox::information(this, tr("information"), tr("This APK file needs to be re-signed before it can be installed !"));
	}
}
void QResArscEditor::onSaveAsReleased_Slot(void)
{
	QString t_filter;
	QString t_title;
	if (m_parser->getParserType() == RES_TYPE::RES_TABLE_TYPE)
	{
		t_filter = m_LE_filePath->text().endsWith(".apk", Qt::CaseInsensitive) ? tr("ARSC File (*.arsc *.apk)") : tr("ARSC File (*.arsc)");
		t_title = tr("OutPut ARSC File");
	}
	else if (m_parser->getParserType() == RES_TYPE::RES_XML_TYPE)
	{
		t_filter = m_LE_filePath->text().endsWith(".apk", Qt::CaseInsensitive) ? tr("Manifest File (*.xml *.apk)") : tr("Manifest File (*.xml)");
		t_title = tr("OutPut Manifest File");
	}

	QString t_FileName = QFileDialog::getSaveFileName(this, t_title, m_basePath,
		t_filter, NULL, QFileDialog::DontConfirmOverwrite);
	if (t_FileName.isEmpty())
		return;

	qDebug() << "Save File:" << t_FileName;
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
	if (m_parser->getParserType() == RES_TYPE::RES_TABLE_TYPE)
		onTreeCurrentItemChanged_slot_TablePackage(_current);
	else if (m_parser->getParserType() == RES_TYPE::RES_XML_TYPE)
		onTreeCurrentItemChanged_slot_XmlTree(_current);
}
void QResArscEditor::onTreeCurrentItemChanged_slot_TablePackage(QTreeWidgetItem* _current)
{
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
void QResArscEditor::onTreeCurrentItemChanged_slot_XmlTree(QTreeWidgetItem* _current)
{
	m_TW_value->clear();
	PResXmlElement t_pXmlElement = _current->data(0, eTreeItemRole_value).value<PResXmlElement>();
	const QResXmlTree* t_xmlTree = t_pXmlElement->getXmlTree();
	const QVector<PResValue>& t_attributes = t_pXmlElement->getAttributes();
	for (int i = 0; i < t_attributes.size(); ++i)
	{
		TXmlAttrEntry* t_attr = reinterpret_cast<TXmlAttrEntry*>(t_attributes[i].get());
		QTreeWidgetItem* t_attrItem = new QTreeWidgetItem_ArscValue(m_TW_value);
		t_attrItem->setData(0, eValueItemRole_restype, (uint32_t)RES_TYPE::RES_XML_TYPE);
		t_attrItem->setText(0, QString("0x%1").arg(i, 8, 16, QChar('0')));
		QString t_name = m_parser->getStringPool()->getGuidRef(t_attr->attribute.name.index)->string;
		if (t_attr->attribute.ns.index != ResStringPool_ref::END)
			t_name = t_xmlTree->getNameSpacePrefix(t_attr->snameSpace->string, NULL)->string + ":" + t_name;
		t_attrItem->setText(1, t_name);
		t_attrItem->setToolTip(1, QString("0x%1").arg(t_attr->attribute.name.index, 8, 16, QChar('0')));
		t_attrItem->setData(0, eValueItemRole_type, eValueItemType_value);
		t_attrItem->setData(0, eValueItemRole_datatype, (uint32_t)t_attr->attribute.typedValue.dataType);
		t_attrItem->setData(0, eValueItemRole_data, t_attr->attribute.typedValue.data);
		t_attrItem->setData(0, eValueItemRole_id, i);
		t_attrItem->setData(0, eValueItemRole_entry, QVariant::fromValue(t_attributes[i]));

		PArscRichString t_sValue;
		if (t_attr->attribute.typedValue.dataType == Res_value::_DataType::TYPE_STRING)
			t_sValue = m_parser->getStringPool()->getGuidRef(t_attr->attribute.typedValue.data);
		t_attrItem->setText(2, resValue2String(t_name, t_attr->attribute.typedValue, t_sValue));
	}
}
void QResArscEditor::onShowValueContextMenu_slot(const QPoint& _pos)
{
	if (m_TW_tree->currentItem() == NULL)
		return;
	if (m_parser->getParserType() == RES_TYPE::RES_TABLE_TYPE)
		onShowValueContextMenu_slot_TablePackage(_pos);
	else if (m_parser->getParserType() == RES_TYPE::RES_XML_TYPE)
		onShowValueContextMenu_slot_XmlTree(_pos);
}
void QResArscEditor::onShowValueContextMenu_slot_TablePackage(const QPoint& _pos)
{
	QTreeWidgetItem* t_specItem = m_TW_tree->currentItem();
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
void QResArscEditor::onShowValueContextMenu_slot_XmlTree(const QPoint& _pos)
{
	delete m_valueMenu;
	m_valueMenu = new QMenu(m_TW_value);
	m_valueMenu->setObjectName(QString::fromUtf8("m_valueMenu"));
	m_valueMenu->addAction(m_AC_EditValue);
	m_valueMenu->addAction(m_AC_AddAttribute);
	m_valueMenu->addAction(m_AC_DeleteAttribute);
	m_valueMenu->popup(QCursor::pos());
}
void QResArscEditor::onShowTreeContextMenu_slot(const QPoint& _pos)
{
	if (m_TW_tree->currentItem() == NULL)
		return;
	if (m_parser->getParserType() == RES_TYPE::RES_TABLE_TYPE)
		onShowTreeContextMenu_slot_TablePackage(_pos);
	else if (m_parser->getParserType() == RES_TYPE::RES_XML_TYPE)
		onShowTreeContextMenu_slot_XmlTree(_pos);
}
void QResArscEditor::onShowTreeContextMenu_slot_TablePackage(const QPoint& _pos)
{
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
	m_treeMenu->addAction(m_AC_ExpandTree);
	m_treeMenu->addAction(m_AC_AllowUcs4);
	m_treeMenu->popup(QCursor::pos());
}
void QResArscEditor::onShowTreeContextMenu_slot_XmlTree(const QPoint& _pos)
{
	delete m_treeMenu;
	m_treeMenu = new QMenu(m_TW_tree);
	m_treeMenu->setObjectName(QString::fromUtf8("m_treeMenu"));
	m_treeMenu->addAction(m_AC_AppendSubElement);
	QTreeWidgetItem* t_item = m_TW_tree->currentItem();
	QTreeWidgetItem* t_parentItem = t_item->parent();
	if (t_parentItem != NULL)
	{
		m_treeMenu->addAction(m_AC_DeleteElement);
		if (t_parentItem->indexOfChild(t_item) != 0)
			m_treeMenu->addAction(m_AC_ElementMoveUp);
		if (t_parentItem->indexOfChild(t_item) != t_parentItem->childCount() - 1)
			m_treeMenu->addAction(m_AC_ElementMoveDown);
	}
	m_treeMenu->addAction(m_AC_ExportXml);
	m_treeMenu->addAction(m_AC_PrintPublicStrings);
	m_treeMenu->addAction(m_AC_ExpandTree);
	m_treeMenu->addAction(m_AC_AllowUcs4);
	m_treeMenu->popup(QCursor::pos());
}
QTreeWidgetItem* onTraversalDefaultSpecificData(const QSet<uint32_t>& _maskIdx, QMap<uint32_t, uint32_t>* _keyIndexs, QTreeWidgetItem* _parent, uint32_t _idx, EValueItemType _type, const QVariant& _v)
{
	if (_type == eValueItemType_arrayend || _type == eValueItemType_arrayitem)
		return NULL;
	if (!_maskIdx.contains(_idx))
		return NULL;
	IResValue* t_Entry = _v.value<PResValue>().get();
	_keyIndexs->insert(_idx, t_Entry->getKeyIndex());
	return NULL;
}
QTreeWidgetItem* onTraversalSpecificData(const QSet<uint32_t>& _maskIdx, QMap<uint32_t, uint32_t>* _keyIndexs, QTreeWidgetItem* _parent, uint32_t _idx, EValueItemType _type, const QVariant& _v)
{
	if (_type == eValueItemType_arrayend || _type == eValueItemType_arrayitem)
		return NULL;
	if (!_maskIdx.contains(_idx))
		return NULL;
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
	//把所有默认的条目都加入进去
	_defaultSpecData->traversalData(std::bind(&onTraversalDefaultSpecificData, t_maskIdx, &t_keyIndexs, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	//再把已有的条目删掉
	_specData->traversalData(std::bind(&onTraversalSpecificData, t_maskIdx, &t_keyIndexs, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	//剩下的就是缺失的条目
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
	QTreeWidgetItem* t_currentTreeItem = m_TW_tree->currentItem();
	if (t_currentTreeItem == NULL)
		return;
	if (t_currentTreeItem->data(0, eTreeItemRole_type).toUInt() != eTreeItemType_spec &&
		t_currentTreeItem->data(0, eTreeItemRole_type).toUInt() != etreeItemType_xmlElement)
		return;
	QTreeWidgetItem* t_valueItem = m_TW_value->currentItem();
	if (t_valueItem == NULL || t_valueItem->data(0, eValueItemRole_type).toUInt() == eValueItemType_array)
		return;

	if (m_parser->getParserType() == RES_TYPE::RES_TABLE_TYPE)
		editValueInit_TablePackage(t_currentTreeItem, t_valueItem);
	else if (m_parser->getParserType() == RES_TYPE::RES_XML_TYPE)
		editValueInit_XmlTree(t_currentTreeItem, t_valueItem);

	PResValue t_pEntry = t_valueItem->data(0, eValueItemRole_entry).value<PResValue>();
	uint32_t t_dataType = t_valueItem->data(0, eValueItemRole_datatype).toUInt();
	uint32_t t_data = t_valueItem->data(0, eValueItemRole_data).toUInt();
	QString t_originalString = t_valueItem->text(2);

	m_editDialog->m_LE_ID->setText(t_valueItem->text(0));
	m_editDialog->m_LE_Name->setText(t_valueItem->text(1));
	m_editDialog->setData(t_dataType, t_data, t_originalString);

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
		PArscRichString t_rich(decodeRichText(m_parser->getStringPool(), t_newString.replace(QString("\\n"), QChar(0x0A))));
		if (t_force)
		{
			m_parser->getStringPool()->removeRichString(t_originalRich);
			if (m_parser->getStringPool()->getRefCount(t_rich) > 0)
			{
				t_rich = m_parser->getStringPool()->getRichString(t_rich);
				std::set<PArscRichString*> t_use_ref = t_originalRich.use_ref();
				for (std::set<PArscRichString*>::iterator i = t_use_ref.begin(); i != t_use_ref.end(); ++i)
					**i = t_rich;
			}
			else
			{
				t_originalRich->string = t_rich->string;
				t_originalRich->styles = t_rich->styles;
				m_parser->getStringPool()->insertRichString(t_originalRich);
			}
			//其实到这，因为都是直接修改的PArscRichString的值，已经不需要t_pEntry->setValue(t_value);了，但是这时候t_pEntry->data是不准确的，但是对于
			//dataType == Res_value::_DataType::TYPE_STRING的情况，我们都是以内部的PArscRichString为准，并不以data为重
		}
		else
		{
			t_rich = m_parser->getStringPool()->getRichString(t_rich);
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
	onTreeCurrentItemChanged_slot(t_currentTreeItem, NULL);
}
void QResArscEditor::editValueInit_TablePackage(QTreeWidgetItem* _treeItem, QTreeWidgetItem* _valueItem)
{
	QTreeWidgetItem* t_typeItem = _treeItem->parent();
	QTreeWidgetItem* t_packageItem = t_typeItem->parent();

	PTablePackage t_tablePackage = t_packageItem->data(0, eTreeItemRole_value).value<PTablePackage>();
	m_editDialog->setTablePackage(t_tablePackage.get());
	m_editDialog->setKeyStringPool(NULL);
}
void QResArscEditor::editValueInit_XmlTree(QTreeWidgetItem* _treeItem, QTreeWidgetItem* _valueItem)
{
	m_editDialog->setTablePackage(NULL);
	m_editDialog->setKeyStringPool(m_parser->getStringPool());
}
void QResArscEditor::onAddAttributeTriggered_slot(void)
{
	QString t_attrName = QInputDialog::getText(this, tr("Add Attribute"), tr("Attribute Name:"));
	if (t_attrName.isEmpty())
		return;
	QTreeWidgetItem* t_currentTreeItem = m_TW_tree->currentItem();
	PResXmlElement t_pElement = t_currentTreeItem->data(0, eTreeItemRole_value).value<PResXmlElement>();
	const QResXmlTree* t_xmlTree = t_pElement->getXmlTree();
	TXmlAttrEntry* t_attrEntry = new TXmlAttrEntry(m_parser->getStringPool());
	t_attrEntry->attribute.ns.index = ResStringPool_ref::END;

	TArscRichString* t_richAttrName = new TArscRichString();
	t_richAttrName->string = t_attrName;
	int t_delimiterIdx = t_attrName.indexOf(":");
	if (t_delimiterIdx > 0)
	{
		t_richAttrName->string = t_attrName.mid(t_delimiterIdx + 1);
		QString t_nsPrefix = t_attrName.mid(0, t_delimiterIdx);
		bool t_ok;
		t_attrEntry->attribute.ns.index = t_xmlTree->getNameSpaceUrl(t_nsPrefix, &t_ok)->guid;
		if (!t_ok)
		{
			QMessageBox::warning(this, tr("warning"), tr("The specified namespace prefix does not exist !"));
			return;
		}
		if (t_nsPrefix != "android")
		{
			QMessageBox::warning(this, tr("warning"), tr("The specified namespace prefix does not support adding !"));
			return;
		}
		t_richAttrName->resId = g_publicFinal->getDataId("@android:attr/" + t_richAttrName->string);
	}
	PArscRichString t_attrNameRef = m_parser->getStringPool()->getRichString(PArscRichString(t_richAttrName));

	t_attrEntry->attribute.name.index = t_attrNameRef->guid;
	t_attrEntry->attribute.rawValue.index = ResStringPool_ref::END;
	t_attrEntry->attribute.typedValue.dataType = Res_value::_DataType::TYPE_NULL;
	t_attrEntry->snameSpace = m_parser->getStringPool()->getGuidRef(t_attrEntry->attribute.ns.index);
	t_attrEntry->sname = t_attrNameRef;
	t_pElement->addAttribute(PResValue(t_attrEntry));
	onTreeCurrentItemChanged_slot_XmlTree(t_currentTreeItem);
}
void QResArscEditor::onDeleteAttributeTriggered_slot(void)
{
	QTreeWidgetItem* t_currentTreeItem = m_TW_tree->currentItem();
	PResXmlElement t_pElement = t_currentTreeItem->data(0, eTreeItemRole_value).value<PResXmlElement>();
	QTreeWidgetItem* t_valueItem = m_TW_value->currentItem();
	int t_index = t_valueItem->data(0, eValueItemRole_id).toInt();
	t_pElement->removeAttribute(t_index);
	onTreeCurrentItemChanged_slot_XmlTree(t_currentTreeItem);
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
QTreeWidgetItem* onWriteSpecificData(QXmlStreamWriter* _xmlWriter, const PTablePackage& _package, uint32_t _typeId, QTreeWidgetItem* _parent, uint32_t _idx, EValueItemType _type, const QVariant& _v)
{
	PResValue t_pResValue = _v.value<PResValue>();
	QString t_entryName = _package->getKeyString(QString(), false, t_pResValue->getKeyIndex());
	switch (_type)
	{
	case eValueItemType_value:
		{
			TTableValueEntry* t_pValueEntry = reinterpret_cast<TTableValueEntry*>(_v.value<PResValue>().get());
			//因为本功能主要就是为汉化翻译等做的，所以只导出字符串类型的值，其他类型的值没什么意义，尤其是引用类型的值，导出后再导入就乱套了
			if (t_pValueEntry->value.dataType != Res_value::_DataType::TYPE_STRING)
				return NULL;
			_xmlWriter->writeStartElement("value");
			_xmlWriter->writeAttribute("id", QString("0x%1").arg(0x7f000000 + (_typeId << 16) + _idx, 8, 16, QChar('0')));
			_xmlWriter->writeAttribute("name", t_entryName);
			_xmlWriter->writeAttribute("type", QString::number((uint32_t)t_pValueEntry->value.dataType));
			QString t_text = resValue2String(t_entryName, t_pValueEntry->value, t_pValueEntry->svalue);
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
			_xmlWriter->writeAttribute("name", t_entryName);
			if (t_pMapValue->entry.parent.ident != 0)
				_xmlWriter->writeAttribute("parent", _package->getKeyString("@", true, t_pMapValue->entry.parent.ident));
		}
		break;
	case eValueItemType_arrayitem:
		{
			ResTable_pairs* t_pPairs = reinterpret_cast<ResTable_pairs*>(_v.value<PResValue>().get());
			if (t_pPairs->value.dataType != Res_value::_DataType::TYPE_STRING)
				return NULL;
			_xmlWriter->writeStartElement("item");
			_xmlWriter->writeAttribute("id", QString::number(_idx));
			_xmlWriter->writeAttribute("name", t_entryName);
			_xmlWriter->writeAttribute("type", QString::number((uint32_t)t_pPairs->value.dataType));
			QString t_text = resValue2String(t_entryName, t_pPairs->value, t_pPairs->svalue);
			if (t_text.indexOf("<") >= 0 || t_text.indexOf("&") >= 0 || t_text.indexOf("\"") >= 0)
				_xmlWriter->writeCDATA(t_text);
			else
				_xmlWriter->writeCharacters(t_text);
			_xmlWriter->writeEndElement();
		}
		break;
	case eValueItemType_arrayend:
		_xmlWriter->writeEndElement();
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
	t_specData->traversalData(std::bind(&onWriteSpecificData, &t_xmlWriter, t_tablePackage, t_typeId, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	t_xmlWriter.writeEndElement();
	t_xmlWriter.writeEndDocument();
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
using TValueMap = QMap<QString, TTmpValue>;
using TArrayMap = QMap<QString, TValueMap>;

QTreeWidgetItem* onImportSpecificData(QStringPool* _stringPool, const TValueMap& _valueMap, const TArrayMap& _arrayMap, const PTablePackage& _package, uint32_t _typeId, QTreeWidgetItem* _parent, uint32_t _idx, EValueItemType _type, const QVariant& _v)
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
				PArscRichString t_rich(decodeRichText(_stringPool, t_tmpValue.data.replace(QString("\\n"), QChar(0x0A))));
				t_rich = _stringPool->getRichString(t_rich);
				t_value.data = t_rich->guid;
			}
			else
				t_value.data = QEditDialog::qstringToData(t_name, t_tmpValue.type, t_tmpValue.data);
			t_pEntry->setValue(t_value);
		}
		break;
	case eValueItemType_arrayend:
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
	t_specData->traversalData(std::bind(&onImportSpecificData, m_parser->getStringPool(), t_valueMap, t_arrayMap,
		t_tablePackage, t_typeId, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	m_TW_value->clear();
	t_specData->traversalData(std::bind(&QResArscEditor::onRefreshSpecificData, this, t_tablePackage, t_typeId, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	QMessageBox::information(this, tr("information"), tr("Import was successful !"));
}
void QResArscEditor::onAppendSubElementTriggered_slot(void)
{
	QString t_elementName = QInputDialog::getText(this, tr("Add SubElement"), tr("Element Name:"));
	if (t_elementName.isEmpty())
		return;

	TArscRichString* t_richElementName = new TArscRichString();
	t_richElementName->string = t_elementName;
	PArscRichString t_elementNameRef = m_parser->getStringPool()->getRichString(PArscRichString(t_richElementName));
	QTreeWidgetItem* t_parentTreeItem = m_TW_tree->currentItem();
	PResXmlElement t_parentElement = t_parentTreeItem->data(0, eTreeItemRole_value).value<PResXmlElement>();
	PResXmlElement t_newElement = t_parentElement->appendSubElement(t_elementNameRef);
	onRefreshXmlElement(t_parentTreeItem, QVariant::fromValue(PResXmlElement(t_newElement)));
	t_parentTreeItem->setExpanded(true);
}
void QResArscEditor::onDeleteElementTriggered_slot(void)
{
	QTreeWidgetItem* t_elementItem = m_TW_tree->currentItem();
	QTreeWidgetItem* t_parentItem = t_elementItem->parent();
	int t_index = t_parentItem->indexOfChild(t_elementItem);
	Q_ASSERT(t_index >= 0);
	PResXmlElement t_pXmlElement = t_parentItem->data(0, eTreeItemRole_value).value<PResXmlElement>();
	t_pXmlElement->removeSubElement(t_index);
	t_parentItem->removeChild(t_elementItem);
	delete t_elementItem;
}
void QResArscEditor::onElementMoveTriggered_slot(int _inc)
{
	QTreeWidgetItem* t_elementItem = m_TW_tree->currentItem();
	QTreeWidgetItem* t_parentItem = t_elementItem->parent();
	int t_index = t_parentItem->indexOfChild(t_elementItem);
	PResXmlElement t_pXmlElement = t_parentItem->data(0, eTreeItemRole_value).value<PResXmlElement>();
	t_pXmlElement->moveSubElement(t_index, t_index + 1);

	t_parentItem->removeChild(t_elementItem);
	t_parentItem->insertChild(t_index + _inc, t_elementItem);
}
void QResArscEditor::onExportXmlTriggered_slot(void)
{
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

	QXmlStreamWriter t_xmlWriter(&t_WriteFile);
	t_xmlWriter.setAutoFormatting(true);
	t_xmlWriter.writeStartDocument();
	reinterpret_cast<QManifestParser*>(m_parser)->exportXml(t_xmlWriter);
	t_WriteFile.close();
	QMessageBox::information(this, tr("information"), tr("Export was successful !"));
}
void QResArscEditor::onPrintPublicStringsTriggered_slot(void)
{
	m_parser->getStringPool()->printRefCount();
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
	if (_type == eValueItemType_arrayend)
		return NULL;
	QTreeWidgetItem* t_TreeWidgetItem = (_parent == NULL) ? new QTreeWidgetItem_ArscValue(m_TW_value) : new QTreeWidgetItem_ArscValue(_parent);
	t_TreeWidgetItem->setData(0, eValueItemRole_restype, (uint32_t)RES_TYPE::RES_TABLE_TYPE);
	t_TreeWidgetItem->setData(0, eValueItemRole_package, QVariant::fromValue(_package));
	IResValue* t_Entry = _v.value<PResValue>().get();
	QString t_name = _package->getKeyString(QString(), false, t_Entry->getKeyIndex());
	t_TreeWidgetItem->setText(1, t_name);
	t_TreeWidgetItem->setToolTip(1, QString("0x%1").arg(t_Entry->getKeyIndex(), 8, 16, QChar('0')));

	switch (_type)
	{
	case eValueItemType_value:
		{
			TTableValueEntry* t_pValueEntry = reinterpret_cast<TTableValueEntry*>(_v.value<PResValue>().get());
			widgetItemSetData(t_TreeWidgetItem, _type, t_pValueEntry->value.data, (uint32_t)t_pValueEntry->value.dataType,
				_idx, QString("0x7f%1%2").arg(_typeId, 2, 16, QChar('0')).arg(_idx, 4, 16, QChar('0')), _v);
			t_TreeWidgetItem->setText(2, resValue2String(t_name, t_pValueEntry->value, t_pValueEntry->svalue));
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
			t_TreeWidgetItem->setText(2, resValue2String(t_name, t_pPairs->value, t_pPairs->svalue));
		}
		break;
	}

	return t_TreeWidgetItem;
}
QTreeWidgetItem* QResArscEditor::onRefreshXmlElement(QTreeWidgetItem* _parent, const QVariant& _v)
{
	QTreeWidgetItem* t_ElementItem = (_parent == NULL) ? new QTreeWidgetItem(m_TW_tree) : new QTreeWidgetItem(_parent);
	PResXmlElement t_pXmlElement = _v.value<PResXmlElement>();
	t_ElementItem->setText(0, m_parser->getStringPool()->getGuidRef(t_pXmlElement->getAttrExt().name.index)->string);
	t_ElementItem->setData(0, eTreeItemRole_type, etreeItemType_xmlElement);
	t_ElementItem->setData(0, eTreeItemRole_value, _v);
	return t_ElementItem;
}


