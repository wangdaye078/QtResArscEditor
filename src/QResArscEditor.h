//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor2\QResArscEditor.h
//	desc:		
//
//	created:	wangdaye 22:6:2025   12:46
//********************************************************************
#ifndef QResArscEditor_h__
#define QResArscEditor_h__
#include "QResArscEditorUI.h"
#include "QTablePackage.h"

class QResArscParser;
class QMenu;

enum ETreeItemRole
{
	eTreeItemRole_type = Qt::UserRole,
	eTreeItemRole_package,
	eTreeItemRole_typeid,
	eTreeItemRole_value,
};


class QResArscEditor : public QResArscEditorUI
{
	Q_OBJECT

public:
	QResArscEditor(QWidget* _parent = nullptr);
	~QResArscEditor();
private:
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
	void onPrintPublicStringsTriggered_slot(void);
private:
	void onRefreshTablePackage(const QString& _packageName, const PTablePackage& _package);
	void onRefreshTablePackageData(const QString& _packageName, ETreeItemType _type, uint32_t _typeId, const QString& _name, const QVariant& _v);
	QTreeWidgetItem* onRefreshSpecificData(const PTablePackage& _package, uint32_t _typeId, QTreeWidgetItem* _parent, uint32_t _idx, EValueItemType _type, const QVariant& _v);
private:
	QResArscParser* m_parser;
	QMenu* m_valueMenu;
	QMenu* m_treeMenu;
	QString m_basePath;
};

#endif // QResArscEditor_h__
