//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor\src\QEditDialog.h
//	desc:		
//
//	created:	wangdaye 18:3:2025   19:15
//********************************************************************
#ifndef QEditDialog_h__
#define QEditDialog_h__
#include <QDialog>
#include <QMap>
#include "ResArscStruct.h"
class QLabel;
class QLineEdit;
class QComboBox;
class QDialogButtonBox;
class QStackedLayout;
class QStackedWidget;
class QTextEdit;
class QPlainTextEdit;
class QTextBrowser;
class QCheckBox;
class QResArscParser;
class QTablePackage;

class QEditDialog : public QDialog
{
	Q_OBJECT

public:
	QEditDialog(QWidget* _parent = nullptr);
	~QEditDialog();
	void RetranslateUi(void);
	void setTablePackage(QTablePackage* _package);
	void setData(const ResTable_config& _config, uint32_t _type, uint32_t _data, const QString& _sdata);
	uint32_t getType(void) const;
	uint32_t getData(void) const;
	QString getSData(void) const;
	static uint32_t qstringToData(Res_value::_DataType _dataType, const QString& _str);
private:
	void CreateControl(void);
private slots:
	void onTypeCurrentIndexChanged_slot(int _index);
	void onStackedCurrentChanged_slot(int _index);
	void onColorTextChanged_slot(const QString& _text);
	void onReferenceTextChanged_slot(const QString& _text);
	void onShowRichTextStateChanged_slot(int);
public:
	QLabel* m_LB_ID;
	QLineEdit* m_LE_ID;
	QLabel* m_LB_Name;
	QLineEdit* m_LE_Name;
	QLabel* m_LB_Type;
	QComboBox* m_CB_Type;
	QLabel* m_LB_Value;
	QDialogButtonBox* m_buttonBox;
	QStackedLayout* m_stackedLayout;
	//----------------------
	QStackedWidget* m_SW_String;
	QPlainTextEdit* m_TE_String;
	QTextBrowser* m_TB_String;
	QCheckBox* m_CB_ShowRichText;
	QLineEdit* m_LE_Color;
	QLabel* m_LB_Color;
	QLineEdit* m_LE_Digital;
	QLineEdit* m_LE_Hex;
	QComboBox* m_CB_Boolen;
	QLineEdit* m_LE_Reference;
	QTextEdit* m_TE_Reference;
	//----------------------
	QMap<uint32_t, int> m_Type2Page;
	QTablePackage* m_tablePackage;
	ResTable_config m_config;
	uint32_t m_type;
	uint32_t m_data;
	QString m_sdata;
};

#endif // QEditDialog_h__
