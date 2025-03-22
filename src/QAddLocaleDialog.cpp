#include "QAddLocaleDialog.h"
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFile>

QAddLocaleDialog::QAddLocaleDialog(QWidget* parent)
	: QDialog(parent)
{
	CreateControl();
	RetranslateUi();
	initLocaleData();
}
QAddLocaleDialog::~QAddLocaleDialog()
{
}
void QAddLocaleDialog::RetranslateUi(void)
{
	setWindowTitle(tr("add locale"));
	m_LB_language->setText(tr("language:"));
	m_LB_country->setText(tr("country:"));
}
void QAddLocaleDialog::CreateControl(void)
{
	resize(223, 93);
	QGridLayout* t_mainLayout = new QGridLayout(this);
	t_mainLayout->setObjectName(QString::fromUtf8("t_mainLayout"));
	m_LB_language = new QLabel(this);
	m_LB_language->setObjectName(QString::fromUtf8("m_LB_language"));

	t_mainLayout->addWidget(m_LB_language, 0, 0, 1, 1);

	m_CB_language = new QComboBox(this);
	m_CB_language->setObjectName(QString::fromUtf8("m_CB_language"));

	t_mainLayout->addWidget(m_CB_language, 0, 1, 1, 1);

	m_LB_country = new QLabel(this);
	m_LB_country->setObjectName(QString::fromUtf8("m_LB_country"));

	t_mainLayout->addWidget(m_LB_country, 1, 0, 1, 1);

	m_CB_country = new QComboBox(this);
	m_CB_country->setObjectName(QString::fromUtf8("m_CB_country"));

	t_mainLayout->addWidget(m_CB_country, 1, 1, 1, 1);

	QDialogButtonBox* t_buttonBox = new QDialogButtonBox(this);
	t_buttonBox->setObjectName(QString::fromUtf8("t_buttonBox"));
	t_buttonBox->setOrientation(Qt::Horizontal);
	t_buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);

	t_mainLayout->addWidget(t_buttonBox, 2, 0, 1, 2);

	t_mainLayout->setColumnStretch(1, 1);

	QObject::connect(t_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	QObject::connect(t_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
	connect(m_CB_language, SIGNAL(currentIndexChanged(int)), this, SLOT(onLanguageCurrentIndexChanged_slot(int)));
}
void QAddLocaleDialog::initLocaleData(void)
{
	QFile t_ReadFile(":/res/res/locale.txt");
	if (!t_ReadFile.exists()) return;

	if (!t_ReadFile.open(QFile::ReadOnly))
		return;
	while (!t_ReadFile.atEnd())
	{
		QByteArray t_line = t_ReadFile.readLine().trimmed();
		int t_pos;
		if ((t_pos = t_line.indexOf("-")) > 0)
			m_locale[t_line.mid(0, t_pos)].append(t_line.mid(t_pos + 1));
		else
			m_locale[t_line].append("");
	}
	t_ReadFile.close();

	for (QMap<QString, QVector<QString>>::iterator i = m_locale.begin(); i != m_locale.end(); ++i)
	{
		m_CB_language->addItem(i.key());
	}
}
void QAddLocaleDialog::onLanguageCurrentIndexChanged_slot(int _index)
{
	QString t_language = m_CB_language->currentText();
	Q_ASSERT(m_locale.contains(t_language));
	QVector<QString>& t_countrys = m_locale[t_language];
	m_CB_country->clear();
	for (int i = 0; i < t_countrys.size(); ++i)
		m_CB_country->addItem(t_countrys[i]);
}
void QAddLocaleDialog::getLocale(ResTable_config& _config)
{
	QString t_language = m_CB_language->currentText();
	QByteArray t_arrLanguage = t_language.toUtf8();
	packLanguageOrRegion(t_arrLanguage.constData(), t_arrLanguage.size(), _config.language, 0x61);
	if (!m_CB_country->currentText().isEmpty())
	{
		QString t_country = m_CB_country->currentText();
		int t_pos;
		if ((t_pos = t_country.indexOf("-")) > 0)
		{
			QByteArray t_arrCounty = t_country.mid(0, t_pos).toUtf8();
			QByteArray t_localeScript = t_country.mid(t_pos + 1).toUtf8();
			packLanguageOrRegion(t_arrCounty.constData(), t_arrCounty.size(), _config.country, 0x30);
			memcpy(_config.localeScript, t_localeScript.data(), t_localeScript.size());
		}
		else
		{
			QByteArray t_arrCounty = t_country.toUtf8();
			packLanguageOrRegion(t_arrCounty.constData(), t_arrCounty.size(), _config.country, 0x30);
		}
	}
}
