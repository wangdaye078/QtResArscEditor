//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor\src\QResArscEditor.h
//	desc:		
//
//	created:	wangdaye 16:3:2025   11:38
//********************************************************************
#ifndef QResArscEditor_h__
#define QResArscEditor_h__

#include "QResArscEditorUI.h"
class QResArscParser;
enum ETreeItemType
{
	eTreeItemType_package,
	eTreeItemType_type,
	eTreeItemType_spec
};
enum ETreeItemRole
{
	eTreeItemRole_type = Qt::UserRole,
	eTreeItemRole_package,
	eTreeItemRole_typeid,
	eTreeItemRole_specid,
	eTreeItemRole_tableConfig
};
enum EValueItemType
{
	eValueItemType_array,
	eValueItemType_arrayitem,
	eValueItemType_value
};
enum EValueItemRole
{
	eValueItemRole_type = Qt::UserRole,		//是什么节点，数据，数组还是数组元素
	eValueItemRole_data,
	eValueItemRole_datatype,
	eValueItemRole_id,
	eValueItemRole_parentid,
};

class QMenu;
class QResArscEditor : public QResArscEditorUI
{
	Q_OBJECT

public:
	QResArscEditor(QWidget* _parent = nullptr);
	~QResArscEditor();
private:
	void refreshArscTree();
	void refreshResTableType(quint32 _typeid, quint32 _specid);

	void onOpenReleased_Slot(void);
	void onSaveReleased_Slot(void);
	void onTreeCurrentItemChanged_slot(QTreeWidgetItem* _current, QTreeWidgetItem* _previous);
	void onShowValueContextMenu_slot(const QPoint& _pos);
	void onShowTreeContextMenu_slot(const QPoint& _pos);
	void onAddValueTriggered_slot(void);
	void onAddAllValueTriggered_slot(void);
	void onDeleteValueTriggered_slot(void);
	void onEditValueTriggered_slot(void);
	void onAddLocaleTriggered_slot(void);
	void onExportLocaleTriggered_slot(void);
	void onImportLocaleTriggered_slot(void);
private:
	QResArscParser* m_Parser;
	QMenu* m_valueMenu;
	QMenu* m_treeMenu;
	QString m_BasePath;
};

#endif // QResArscEditor_h__
