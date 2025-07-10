//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor2\src\QTreeWidgetItem_ArscValue.h
//	desc:		
//
//	created:	wangdaye 1:7:2025   15:47
//********************************************************************
#ifndef QTreeWidgetItem_ArscValue_h__
#define QTreeWidgetItem_ArscValue_h__

#include <QTreeWidgetItem>

class QTreeWidgetItem_ArscValue : public QTreeWidgetItem
{
public:
	QTreeWidgetItem_ArscValue(QTreeWidget* _parent);
	QTreeWidgetItem_ArscValue(QTreeWidgetItem* _parent);
	~QTreeWidgetItem_ArscValue();
	virtual QVariant data(int _column, int _role) const;
};

#endif // QTreeWidgetItem_ArscValue_h__
