#include "QAddLocaleDialog.h"
#include "QAppendDialog.h"
#include "QEditDialog.h"
#include "QResArscEditorUI.h"
#include "src/common/QUtf8.h"
#include <QAction>
#include <QCoreApplication>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QSplitter>
#include <QStyleFactory>
#include <QToolButton>
#include <QTreeWidget>

QResArscEditorUI::QResArscEditorUI(QWidget* _parent)
	: QDialog(_parent)
{
	CreateControl();
	m_editDialog = new QEditDialog(this);
	m_appendDialog = new QAppendDialog(this);
	m_addLocaleDialog = new QAddLocaleDialog(this);
	RetranslateUi();
}

QResArscEditorUI::~QResArscEditorUI()
{

}
void QResArscEditorUI::RetranslateUi(void)
{
	setWindowTitle(tr("QResArscEditor ") + QCoreApplication::applicationVersion());
	m_LB_filePath->setText(tr("filePath:"));

	QTreeWidgetItem* t_treeWidgetItem = m_TW_value->headerItem();
	t_treeWidgetItem->setText(2, tr("Value"));
	t_treeWidgetItem->setText(1, tr("Name"));
	t_treeWidgetItem->setText(0, tr("ID"));

	m_AC_AddValue->setText(tr("AddValue"));
	m_AC_AddAllValue->setText(tr("AddAllValue"));
	m_AC_DeleteValue->setText(tr("DeleteValue"));
	m_AC_EditValue->setText(tr("EditValue"));
	m_AC_ExpandAll->setText(tr("ExpandAll"));
	m_AC_AddAttribute->setText(tr("AddAttribute"));
	m_AC_DeleteAttribute->setText(tr("DeleteAttribute"));
	m_AC_AddLocale->setText(tr("AddLocale"));
	m_AC_ExportLocale->setText(tr("ExportLocale"));
	m_AC_ImportLocale->setText(tr("ImportLocale"));
	m_AC_AppendSubElement->setText(tr("AppendSubElement"));
	m_AC_DeleteElement->setText(tr("DeleteElement"));
	m_AC_ElementMoveUp->setText(tr("ElementMoveUp"));
	m_AC_ElementMoveDown->setText(tr("ElementMoveDown"));
	m_AC_ExportXml->setText(tr("ExportXml"));
	m_AC_PrintPublicStrings->setText(tr("PrintPublicStrings"));
	m_AC_ExpandTree->setText(tr("ExpandAll"));
	m_AC_AllowUcs4->setText(tr("AllowUcs4"));

	m_TB_open->setToolTip(tr("Open ARSC File"));
	m_TB_save->setToolTip(tr("Save ARSC File"));
	m_TB_saveas->setToolTip(tr("Save ARSC File As"));

	m_editDialog->RetranslateUi();
	m_appendDialog->RetranslateUi();
	m_addLocaleDialog->RetranslateUi();
}
void QResArscEditorUI::CreateControl(void)
{
	setObjectName(QString::fromUtf8("QResArscEditorUIClass"));
	resize(740, 503);
	setFont(QFont(QString::fromUtf8("\345\256\213\344\275\223")));

	QGridLayout* t_mainLayout = new QGridLayout(this);
	t_mainLayout->setSpacing(6);
	t_mainLayout->setContentsMargins(11, 11, 11, 11);
	t_mainLayout->setObjectName(QString::fromUtf8("t_mainLayout"));
	m_LB_filePath = new QLabel(this);
	m_LB_filePath->setObjectName(QString::fromUtf8("m_LB_filePath"));

	t_mainLayout->addWidget(m_LB_filePath, 0, 0, 1, 1);

	m_LE_filePath = new QLineEdit(this);
	m_LE_filePath->setObjectName(QString::fromUtf8("m_LE_filePath"));
	m_LE_filePath->setReadOnly(true);

	t_mainLayout->addWidget(m_LE_filePath, 0, 1, 1, 1);

	m_TB_open = new QToolButton(this);
	m_TB_open->setObjectName(QString::fromUtf8("m_TB_open"));
	m_TB_open->setIcon(QIcon(":/icon/ico/open.png"));

	t_mainLayout->addWidget(m_TB_open, 0, 2, 1, 1);

	m_TB_save = new QToolButton(this);
	m_TB_save->setObjectName(QString::fromUtf8("m_TB_save"));
	m_TB_save->setIcon(QIcon(":/icon/ico/save.png"));
	m_TB_save->setEnabled(false);

	t_mainLayout->addWidget(m_TB_save, 0, 3, 1, 1);

	m_TB_saveas = new QToolButton(this);
	m_TB_saveas->setObjectName(QString::fromUtf8("m_TB_saveas"));
	m_TB_saveas->setIcon(QIcon(":/icon/ico/saveas.png"));
	m_TB_saveas->setEnabled(false);

	t_mainLayout->addWidget(m_TB_saveas, 0, 4, 1, 1);

	QSplitter* t_splitter = new QSplitter(this);
	t_splitter->setObjectName(QString::fromUtf8("t_splitter"));
	t_splitter->setOrientation(Qt::Horizontal);

	m_TW_tree = new QTreeWidget(t_splitter);
	m_TW_tree->setIndentation(10);
	m_TW_tree->setObjectName(QString::fromUtf8("m_TW_tree"));
	m_TW_tree->setContextMenuPolicy(Qt::CustomContextMenu);
	m_TW_tree->header()->setVisible(false);

	t_splitter->addWidget(m_TW_tree);
	m_TW_value = new QTreeWidget(t_splitter);
	m_TW_value->setIndentation(13);
	QTreeWidgetItem* __qtreewidgetitem1 = new QTreeWidgetItem();
	m_TW_value->setHeaderItem(__qtreewidgetitem1);
	m_TW_value->setObjectName(QString::fromUtf8("m_TW_value"));
	m_TW_value->setContextMenuPolicy(Qt::CustomContextMenu);
	m_TW_value->setColumnCount(3);
	t_splitter->addWidget(m_TW_value);

	t_mainLayout->addWidget(t_splitter, 1, 0, 1, 5);

	//------------------------------------------------
	t_splitter->setSizes(QList<int>() << 200 << 500);
	m_TW_value->setColumnWidth(0, 100);
	m_TW_value->setColumnWidth(1, 250);
	m_TW_tree->setItemDelegate(new ItemColorDelegate());
	m_TW_value->setItemDelegate(new ItemColorDelegate());
	m_TW_tree->setStyle(QStyleFactory::create("windows"));

	m_AC_AddValue = new QAction(this);
	m_AC_AddValue->setObjectName(QString::fromUtf8("m_AC_AddValue"));
	m_AC_AddAllValue = new QAction(this);
	m_AC_AddAllValue->setObjectName(QString::fromUtf8("m_AC_AddAllValue"));
	m_AC_DeleteValue = new QAction(this);
	m_AC_DeleteValue->setObjectName(QString::fromUtf8("m_AC_DeleteValue"));
	m_AC_EditValue = new QAction(this);
	m_AC_EditValue->setObjectName(QString::fromUtf8("m_AC_EditValue"));
	m_AC_ExpandAll = new QAction(this);
	m_AC_ExpandAll->setObjectName(QString::fromUtf8("m_AC_ExpandAll"));
	m_AC_AddAttribute = new QAction(this);
	m_AC_AddAttribute->setObjectName(QString::fromUtf8("m_AC_AddAttribute"));
	m_AC_DeleteAttribute = new QAction(this);
	m_AC_DeleteAttribute->setObjectName(QString::fromUtf8("m_AC_DeleteAttribute"));
	m_AC_AddLocale = new QAction(this);
	m_AC_AddLocale->setObjectName(QString::fromUtf8("m_AC_AddLocale"));
	m_AC_ExportLocale = new QAction(this);
	m_AC_ExportLocale->setObjectName(QString::fromUtf8("m_AC_ExportLocale"));
	m_AC_ImportLocale = new QAction(this);
	m_AC_ImportLocale->setObjectName(QString::fromUtf8("m_AC_ImportLocale"));
	m_AC_AppendSubElement = new QAction(this);
	m_AC_AppendSubElement->setObjectName(QString::fromUtf8("m_AC_AppendElement"));
	m_AC_DeleteElement = new QAction(this);
	m_AC_DeleteElement->setObjectName(QString::fromUtf8("m_AC_DeleteElement"));
	m_AC_ElementMoveUp = new QAction(this);
	m_AC_ElementMoveUp->setObjectName(QString::fromUtf8("m_AC_ElementMoveUp"));
	m_AC_ElementMoveDown = new QAction(this);
	m_AC_ElementMoveDown->setObjectName(QString::fromUtf8("m_AC_ElementMoveDown"));
	m_AC_ExportXml = new QAction(this);
	m_AC_ExportXml->setObjectName(QString::fromUtf8("m_AC_ExportXml"));
	m_AC_PrintPublicStrings = new QAction(this);
	m_AC_PrintPublicStrings->setObjectName(QString::fromUtf8("m_AC_PrintPublicStrings"));
	m_AC_ExpandTree = new QAction(this);
	m_AC_ExpandTree->setObjectName(QString::fromUtf8("m_AC_ExpandTree"));
	m_AC_AllowUcs4 = new QAction(this);
	m_AC_AllowUcs4->setObjectName(QString::fromUtf8("m_AC_AllowUcs4"));
	m_AC_AllowUcs4->setCheckable(true);
	m_AC_AllowUcs4->setChecked(allow_ucs4);
	//------------------------------------------------
	connect(m_TB_open, SIGNAL(released()), this, SLOT(onOpenReleased_Slot()));
	connect(m_TB_save, SIGNAL(released()), this, SLOT(onSaveReleased_Slot()));
	connect(m_TB_saveas, SIGNAL(released()), this, SLOT(onSaveAsReleased_Slot()));
	connect(m_TW_tree, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(onTreeCurrentItemChanged_slot(QTreeWidgetItem*, QTreeWidgetItem*)));
	connect(m_TW_value, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(onShowValueContextMenu_slot(const QPoint&)));
	connect(m_TW_tree, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(onShowTreeContextMenu_slot(const QPoint&)));
	connect(m_AC_AddValue, SIGNAL(triggered()), this, SLOT(onAddValueTriggered_slot()));
	connect(m_AC_AddAllValue, SIGNAL(triggered()), this, SLOT(onAddAllValueTriggered_slot()));
	connect(m_AC_DeleteValue, SIGNAL(triggered()), this, SLOT(onDeleteValueTriggered_slot()));
	connect(m_AC_EditValue, SIGNAL(triggered()), this, SLOT(onEditValueTriggered_slot()));
	connect(m_AC_ExpandAll, SIGNAL(triggered()), this, SLOT(onExpandAllTriggered_slot()));
	connect(m_AC_AddAttribute, SIGNAL(triggered()), this, SLOT(onAddAttributeTriggered_slot()));
	connect(m_AC_DeleteAttribute, SIGNAL(triggered()), this, SLOT(onDeleteAttributeTriggered_slot()));
	connect(m_AC_AddLocale, SIGNAL(triggered()), this, SLOT(onAddLocaleTriggered_slot()));
	connect(m_AC_ExportLocale, SIGNAL(triggered()), this, SLOT(onExportLocaleTriggered_slot()));
	connect(m_AC_ImportLocale, SIGNAL(triggered()), this, SLOT(onImportLocaleTriggered_slot()));
	connect(m_AC_AppendSubElement, SIGNAL(triggered()), this, SLOT(onAppendSubElementTriggered_slot()));
	connect(m_AC_DeleteElement, SIGNAL(triggered()), this, SLOT(onDeleteElementTriggered_slot()));
	connect(m_AC_ElementMoveUp, &QAction::triggered, [=]() {onElementMoveTriggered_slot(-1); });
	connect(m_AC_ElementMoveDown, &QAction::triggered, [=]() {onElementMoveTriggered_slot(1); });
	connect(m_AC_ExportXml, SIGNAL(triggered()), this, SLOT(onExportXmlTriggered_slot()));
	connect(m_AC_PrintPublicStrings, SIGNAL(triggered()), this, SLOT(onPrintPublicStringsTriggered_slot()));
	connect(m_AC_ExpandTree, SIGNAL(triggered()), this, SLOT(onExpandTreeTriggered_slot()));
	connect(m_AC_AllowUcs4, SIGNAL(triggered(bool)), this, SLOT(onAllowUcs4Triggered_slot(bool)));
}
void QResArscEditorUI::onExpandAllTriggered_slot(void)
{
	m_TW_value->expandAll();
}
void QResArscEditorUI::onExpandTreeTriggered_slot(void)
{
	m_TW_tree->expandAll();
}
void QResArscEditorUI::onAllowUcs4Triggered_slot(bool _checked)
{
	allow_ucs4 = _checked;
}
