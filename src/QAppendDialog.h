//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor\src\QAppendDialog.h
//	desc:		
//
//	created:	wangdaye 19:3:2025   12:35
//********************************************************************
#ifndef QAppendDialog_h__
#define QAppendDialog_h__
#include <QDialog>

class QLabel;
class QLineEdit;
class QComboBox;
class QDialogButtonBox;

class QAppendDialog : public QDialog
{
	Q_OBJECT
public:
	QAppendDialog(QWidget* _parent = nullptr);
	~QAppendDialog();
	void RetranslateUi(void);
private:
	void CreateControl(void);
private slots:
	void onNameCurrentIndexChanged_slot(int _index);
public:
	QLabel* m_LB_ID;
	QLineEdit* m_LE_ID;
	QLabel* m_LB_Name;
	QComboBox* m_CB_Name;
	QDialogButtonBox* m_buttonBox;
};

#endif // QAppendDialog_h__
