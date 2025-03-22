//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor\src\QAddLocaleDialog.h
//	desc:		
//
//	created:	wangdaye 21:3:2025   21:19
//********************************************************************
#ifndef QAddLocaleDialog_h__
#define QAddLocaleDialog_h__
#include <QDialog>
#include <QMap>
#include "ResArscStruct.h"

class QLabel;
class QComboBox;

class QAddLocaleDialog : public QDialog
{
	Q_OBJECT

public:
	QAddLocaleDialog(QWidget* parent = nullptr);
	~QAddLocaleDialog();
	void RetranslateUi(void);
	void getLocale(ResTable_config& _config);
private:
	void CreateControl(void);
	void initLocaleData(void);
private slots:
	void onLanguageCurrentIndexChanged_slot(int _index);
private:
	QLabel* m_LB_language;
	QComboBox* m_CB_language;
	QLabel* m_LB_country;
	QComboBox* m_CB_country;
	QMap<QString, QVector<QString>> m_locale;
};

#endif // QAddLocaleDialog_h__
