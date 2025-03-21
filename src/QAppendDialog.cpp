#include "QAppendDialog.h"
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>

QAppendDialog::QAppendDialog(QWidget* _parent)
	: QDialog(_parent)
{
	CreateControl();
	RetranslateUi();
}
QAppendDialog::~QAppendDialog()
{
}
void QAppendDialog::RetranslateUi(void)
{
	setWindowTitle(tr("Append"));
	m_LB_ID->setText(tr("ID:"));
	m_LB_Name->setText(tr("Name:"));
}
void QAppendDialog::CreateControl(void)
{
	setObjectName(QString::fromUtf8("QAppendDialog"));
	resize(282, 93);
	QGridLayout* t_mainLayout = new QGridLayout(this);
	t_mainLayout->setObjectName(QString::fromUtf8("t_mainLayout"));
	m_LB_ID = new QLabel(this);
	m_LB_ID->setObjectName(QString::fromUtf8("m_LB_ID"));

	t_mainLayout->addWidget(m_LB_ID, 0, 0, 1, 1);

	m_LE_ID = new QLineEdit(this);
	m_LE_ID->setObjectName(QString::fromUtf8("m_LE_ID"));
	m_LE_ID->setReadOnly(true);

	t_mainLayout->addWidget(m_LE_ID, 0, 1, 1, 1);

	m_LB_Name = new QLabel(this);
	m_LB_Name->setObjectName(QString::fromUtf8("m_LB_Name"));

	t_mainLayout->addWidget(m_LB_Name, 1, 0, 1, 1);

	m_CB_Name = new QComboBox(this);
	m_CB_Name->setObjectName(QString::fromUtf8("m_CB_Name"));

	t_mainLayout->addWidget(m_CB_Name, 1, 1, 1, 1);

	m_buttonBox = new QDialogButtonBox(this);
	m_buttonBox->setObjectName(QString::fromUtf8("m_buttonBox"));
	m_buttonBox->setOrientation(Qt::Horizontal);
	m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);

	t_mainLayout->addWidget(m_buttonBox, 2, 0, 1, 2);

	connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
	connect(m_CB_Name, SIGNAL(currentIndexChanged(int)), this, SLOT(onNameCurrentIndexChanged_slot(int)));
}
void QAppendDialog::onNameCurrentIndexChanged_slot(int _index)
{
	if (_index < 0)
		return;
	m_LE_ID->setText(QString("0x%1").arg(m_CB_Name->currentData().toUInt(), 8, 16, QChar('0')));
}
