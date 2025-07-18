//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor\QResArscEditorUI.h
//	desc:		
//
//	created:	wangdaye 14:3:2025   20:54
//********************************************************************
#ifndef QResArscEditorUI_h__
#define QResArscEditorUI_h__
#include <QItemDelegate>
#include <QtWidgets/QDialog>

class QLabel;
class QLineEdit;
class QToolButton;
class QTreeWidget;
class QTreeWidgetItem;
class QEditDialog;
class QAppendDialog;
class QAddLocaleDialog;
class QAction;

class ItemColorDelegate : public QItemDelegate
{
	Q_OBJECT
public:
	ItemColorDelegate()
	{
	}
	void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
	{
		QStyleOptionViewItem  t_viewOption(_option);
		//高亮显示与普通显示时的前景色一致（即选中行和为选中时候的文字颜色一样）
		t_viewOption.palette.setColor(QPalette::HighlightedText, _index.data(Qt::ForegroundRole).value<QColor>());
		t_viewOption.palette.setColor(QPalette::Highlight, QColor(245, 228, 156));
		QItemDelegate::paint(_painter, t_viewOption, _index);
	}
	QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const
	{
		QSize t_size = QItemDelegate::sizeHint(_option, _index);
		t_size.setHeight(18);
		return t_size;
	}
};
class QResArscEditorUI : public QDialog
{
	Q_OBJECT

public:
	QResArscEditorUI(QWidget* _parent = nullptr);
	~QResArscEditorUI();
	void RetranslateUi(void);
private:
	void CreateControl(void);
private slots:
	virtual void onOpenReleased_Slot(void) = 0;
	virtual void onSaveReleased_Slot(void) = 0;
	virtual void onSaveAsReleased_Slot(void) = 0;
	virtual void onTreeCurrentItemChanged_slot(QTreeWidgetItem* _current, QTreeWidgetItem* _previous) = 0;
	virtual void onShowValueContextMenu_slot(const QPoint& _pos) = 0;
	virtual void onShowTreeContextMenu_slot(const QPoint& _pos) = 0;
	virtual void onAddValueTriggered_slot(void) = 0;
	virtual void onAddAllValueTriggered_slot(void) = 0;
	virtual void onDeleteValueTriggered_slot(void) = 0;
	virtual void onEditValueTriggered_slot(void) = 0;
	virtual void onAddLocaleTriggered_slot(void) = 0;
	virtual void onExportLocaleTriggered_slot(void) = 0;
	virtual void onImportLocaleTriggered_slot(void) = 0;
	virtual void onPrintPublicStringsTriggered_slot(void) = 0;
	void onExpandAllTriggered_slot(void);
protected:
	QLabel* m_LB_filePath;
	QLineEdit* m_LE_filePath;
	QToolButton* m_TB_open;
	QToolButton* m_TB_save;
	QToolButton* m_TB_saveas;
	QTreeWidget* m_TW_tree;
	QTreeWidget* m_TW_value;
	QEditDialog* m_editDialog;
	QAppendDialog* m_appendDialog;
	QAddLocaleDialog* m_addLocaleDialog;

	QAction* m_AC_AddValue;
	QAction* m_AC_AddAllValue;
	QAction* m_AC_DeleteValue;
	QAction* m_AC_EditValue;
	QAction* m_AC_ExpandAll;

	QAction* m_AC_AddLocale;
	QAction* m_AC_ExportLocale;
	QAction* m_AC_ImportLocale;
	QAction* m_AC_PrintPublicStrings;
};

#endif // QResArscEditorUI_h__
