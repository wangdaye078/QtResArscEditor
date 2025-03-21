#include "QResArscEditorUI.h"
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QSplitter>
#include <QTreeWidget>
#include <QStyleFactory>
#include <QHeaderView>
#include <QAction>
#include "QEditDialog.h"
#include "QAppendDialog.h"

QResArscEditorUI::QResArscEditorUI(QWidget* _parent)
	: QDialog(_parent)
{
	CreateControl();
	m_editDialog = new QEditDialog(this);
	m_appendDialog = new QAppendDialog(this);
	RetranslateUi();
}

QResArscEditorUI::~QResArscEditorUI()
{

}
void QResArscEditorUI::RetranslateUi(void)
{
	setWindowTitle(tr("QResArscEditor"));
	m_LB_filePath->setText(tr("filePath:"));

	QTreeWidgetItem* t_treeWidgetItem = m_TW_value->headerItem();
	t_treeWidgetItem->setText(2, tr("Value"));
	t_treeWidgetItem->setText(1, tr("Name"));
	t_treeWidgetItem->setText(0, tr("ID"));

	m_AC_AddValue->setText(tr("AddValue"));
	m_AC_DeleteValue->setText(tr("DeleteValue"));
	m_AC_EditValue->setText(tr("EditValue"));

	m_editDialog->RetranslateUi();
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

	t_mainLayout->addWidget(m_TB_save, 0, 3, 1, 1);

	QSplitter* t_splitter = new QSplitter(this);
	t_splitter->setObjectName(QString::fromUtf8("t_splitter"));
	t_splitter->setOrientation(Qt::Horizontal);

	m_TW_tree = new QTreeWidget(t_splitter);
	m_TW_tree->setIndentation(10);
	m_TW_tree->setObjectName(QString::fromUtf8("m_TW_tree"));
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

	t_mainLayout->addWidget(t_splitter, 1, 0, 1, 4);

	//------------------------------------------------
	t_splitter->setSizes(QList<int>() << 200 << 500);
	m_TW_value->setColumnWidth(0, 100);
	m_TW_value->setColumnWidth(1, 250);
	m_TW_tree->setItemDelegate(new ItemColorDelegate());
	m_TW_value->setItemDelegate(new ItemColorDelegate());
	m_TW_tree->setStyle(QStyleFactory::create("windows"));

	m_AC_AddValue = new QAction(this);
	m_AC_AddValue->setObjectName(QString::fromUtf8("m_AC_AddValue"));
	m_AC_DeleteValue = new QAction(this);
	m_AC_DeleteValue->setObjectName(QString::fromUtf8("m_AC_DeleteValue"));
	m_AC_EditValue = new QAction(this);
	m_AC_EditValue->setObjectName(QString::fromUtf8("m_AC_EditValue"));
	//------------------------------------------------
	connect(m_TB_open, SIGNAL(released()), this, SLOT(onOpenReleased_Slot()));
	connect(m_TB_save, SIGNAL(released()), this, SLOT(onSaveReleased_Slot()));
	connect(m_TW_tree, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(onTreeCurrentItemChanged_slot(QTreeWidgetItem*, QTreeWidgetItem*)));
	connect(m_TW_value, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(onShowValueContextMenu_slot(const QPoint&)));
	connect(m_AC_AddValue, SIGNAL(triggered()), this, SLOT(onAddValueTriggered_slot()));
	connect(m_AC_DeleteValue, SIGNAL(triggered()), this, SLOT(onDeleteValueTriggered_slot()));
	connect(m_AC_EditValue, SIGNAL(triggered()), this, SLOT(onEditValueTriggered_slot()));
}
