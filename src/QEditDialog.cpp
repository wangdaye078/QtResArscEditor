#include "QAndroidAttribute.h"
#include "QEditDialog.h"
#include "QPublicFinal.h"
#include "QResArscParser.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QRegExp>
#include <QRgba64>
#include <QStackedLayout>
#include <QStackedWidget>
#include <QTextBrowser>
#include <QTextEdit>
QEditDialog::QEditDialog(QWidget* _parent)
	: QDialog(_parent)
{
	CreateControl();
	RetranslateUi();
}

QEditDialog::~QEditDialog()
{
}
void QEditDialog::RetranslateUi(void)
{
	setWindowTitle(tr("EditDialog"));
	m_LB_ID->setText(tr("ID:"));
	m_LB_Name->setText(tr("Name:"));
	m_LB_Type->setText(tr("Type:"));
	m_LB_Value->setText(tr("Value:"));
	m_CB_ShowRichText->setText(tr("ShowRichText"));
	m_LE_Color->setPlaceholderText(tr("0x02468ACE"));
	m_LE_Digital->setPlaceholderText(tr("decimal numerical value"));
	m_LE_Hex->setPlaceholderText(tr("0x02468ACE"));
	m_LE_Reference->setPlaceholderText(tr("0x7F010001"));
}
void QEditDialog::CreateControl(void)
{
	setObjectName(QString::fromUtf8("QEditDialog"));
	resize(500, 320);
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

	m_LE_Name = new QLineEdit(this);
	m_LE_Name->setObjectName(QString::fromUtf8("m_LE_Name"));
	m_LE_Name->setReadOnly(true);

	t_mainLayout->addWidget(m_LE_Name, 1, 1, 1, 1);

	m_LB_Type = new QLabel(this);
	m_LB_Type->setObjectName(QString::fromUtf8("m_LB_Type"));

	t_mainLayout->addWidget(m_LB_Type, 2, 0, 1, 1);

	m_CB_Type = new QComboBox(this);
	m_CB_Type->setObjectName(QString::fromUtf8("m_CB_Type"));

	t_mainLayout->addWidget(m_CB_Type, 2, 1, 1, 1);

	m_LB_Value = new QLabel(this);
	m_LB_Value->setObjectName(QString::fromUtf8("m_LB_Value"));
	m_LB_Value->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignTop);

	t_mainLayout->addWidget(m_LB_Value, 3, 0, 1, 1);

	m_stackedLayout = new QStackedLayout();
	m_stackedLayout->setObjectName(QString::fromUtf8("m_stackedLayout"));

	t_mainLayout->addLayout(m_stackedLayout, 3, 1, 1, 1);

	m_buttonBox = new QDialogButtonBox(this);
	m_buttonBox->setObjectName(QString::fromUtf8("m_buttonBox"));
	m_buttonBox->setOrientation(Qt::Horizontal);
	m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);

	t_mainLayout->addWidget(m_buttonBox, 4, 0, 1, 2);

	//----------------0
	QWidget* t_W_String = new QWidget(this);
	t_W_String->setObjectName(QString::fromUtf8("t_W_String"));
	QGridLayout* t_gLayout_String = new QGridLayout(t_W_String);
	t_gLayout_String->setObjectName(QString::fromUtf8("t_gLayout_String"));
	t_gLayout_String->setContentsMargins(0, 0, 0, 0);
	t_gLayout_String->setSpacing(0);

	m_SW_String = new QStackedWidget(t_W_String);
	m_SW_String->setObjectName(QString::fromUtf8("t_SW_String"));
	m_TE_String = new QPlainTextEdit();
	m_TE_String->setObjectName(QString::fromUtf8("m_TE_String"));
	m_SW_String->addWidget(m_TE_String);

	m_TB_String = new QTextBrowser();
	m_TB_String->setObjectName(QString::fromUtf8("m_TB_String"));
	m_TB_String->setLineWrapMode(QTextEdit::NoWrap);
	m_SW_String->addWidget(m_TB_String);

	t_gLayout_String->addWidget(m_SW_String, 0, 0, 1, 2);

	QSpacerItem* t_hSpacer_String = new QSpacerItem(367, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

	t_gLayout_String->addItem(t_hSpacer_String, 1, 0, 1, 1);

	m_CB_ShowRichText = new QCheckBox(t_W_String);
	m_CB_ShowRichText->setObjectName(QString::fromUtf8("m_CB_ShowRichText"));

	t_gLayout_String->addWidget(m_CB_ShowRichText, 1, 1, 1, 1);
	m_stackedLayout->addWidget(t_W_String);
	//----------------1
	QWidget* t_W_Color = new QWidget(this);
	t_W_Color->setObjectName(QString::fromUtf8("t_W_Color"));
	QGridLayout* t_gLayout_Color = new QGridLayout(t_W_Color);
	t_gLayout_Color->setObjectName(QString::fromUtf8("t_gridLayout_Color"));
	t_gLayout_Color->setContentsMargins(0, 0, 0, 0);
	m_LE_Color = new QLineEdit(t_W_Color);
	m_LE_Color->setObjectName(QString::fromUtf8("m_LE_Color"));

	t_gLayout_Color->addWidget(m_LE_Color, 0, 0, 1, 2);

	QSpacerItem* t_hSpacer_Color = new QSpacerItem(213, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

	t_gLayout_Color->addItem(t_hSpacer_Color, 1, 0, 1, 1);

	m_LB_Color = new QLabel(t_W_Color);
	m_LB_Color->setObjectName(QString::fromUtf8("m_LB_Color"));
	m_LB_Color->setMinimumSize(QSize(32, 32));
	m_LB_Color->setMaximumSize(QSize(32, 32));

	t_gLayout_Color->addWidget(m_LB_Color, 1, 1, 1, 1);

	QSpacerItem* t_vSpacer_Color = new QSpacerItem(20, 51, QSizePolicy::Minimum, QSizePolicy::Expanding);

	t_gLayout_Color->addItem(t_vSpacer_Color, 2, 0, 1, 1);
	m_stackedLayout->addWidget(t_W_Color);
	//----------------2
	QWidget* t_W_Digital = new QWidget(this);
	t_W_Digital->setObjectName(QString::fromUtf8("t_W_Digital"));
	QVBoxLayout* t_vLayout_Digital = new QVBoxLayout(t_W_Digital);
	t_vLayout_Digital->setObjectName(QString::fromUtf8("t_vLayout_Digital"));
	t_vLayout_Digital->setContentsMargins(0, 0, 0, 0);
	m_LE_Digital = new QLineEdit(t_W_Digital);
	m_LE_Digital->setObjectName(QString::fromUtf8("m_LE_Digital"));

	t_vLayout_Digital->addWidget(m_LE_Digital);

	QSpacerItem* t_vSpacer_Digital = new QSpacerItem(20, 89, QSizePolicy::Minimum, QSizePolicy::Expanding);

	t_vLayout_Digital->addItem(t_vSpacer_Digital);
	m_stackedLayout->addWidget(t_W_Digital);
	//----------------3
	QWidget* t_W_Hex = new QWidget(this);
	t_W_Hex->setObjectName(QString::fromUtf8("t_W_Hex"));
	QVBoxLayout* t_vLayout_Hex = new QVBoxLayout(t_W_Hex);
	t_vLayout_Hex->setObjectName(QString::fromUtf8("t_vLayout_Hex"));
	t_vLayout_Hex->setContentsMargins(0, 0, 0, 0);
	m_LE_Hex = new QLineEdit(t_W_Hex);
	m_LE_Hex->setObjectName(QString::fromUtf8("m_LE_Hex"));

	t_vLayout_Hex->addWidget(m_LE_Hex);

	QSpacerItem* t_vSpacer_Hex = new QSpacerItem(20, 89, QSizePolicy::Minimum, QSizePolicy::Expanding);

	t_vLayout_Hex->addItem(t_vSpacer_Hex);
	m_stackedLayout->addWidget(t_W_Hex);
	//----------------4
	QWidget* t_W_Boolen = new QWidget(this);
	t_W_Boolen->setObjectName(QString::fromUtf8("t_W_Boolen"));
	QVBoxLayout* t_vLayout_Boolen = new QVBoxLayout(t_W_Boolen);
	t_vLayout_Boolen->setObjectName(QString::fromUtf8("t_vLayout_Boolen"));
	t_vLayout_Boolen->setContentsMargins(0, 0, 0, 0);
	m_CB_Boolen = new QComboBox(t_W_Boolen);
	m_CB_Boolen->setObjectName(QString::fromUtf8("m_CB_Boolen"));

	t_vLayout_Boolen->addWidget(m_CB_Boolen);

	QSpacerItem* t_vSpacer_Boolen = new QSpacerItem(20, 89, QSizePolicy::Minimum, QSizePolicy::Expanding);

	t_vLayout_Boolen->addItem(t_vSpacer_Boolen);
	m_stackedLayout->addWidget(t_W_Boolen);
	//----------------5
	QWidget* t_W_Reference = new QWidget(this);
	t_W_Reference->setObjectName(QString::fromUtf8("t_W_Reference"));

	QVBoxLayout* t_vLayout_Reference = new QVBoxLayout(t_W_Reference);
	t_vLayout_Reference->setObjectName(QString::fromUtf8("t_vLayout_Reference"));
	t_vLayout_Reference->setContentsMargins(0, 0, 0, 0);
	m_LE_Reference = new QLineEdit(t_W_Reference);
	m_LE_Reference->setObjectName(QString::fromUtf8("m_LE_Reference"));

	t_vLayout_Reference->addWidget(m_LE_Reference);

	m_TE_Reference = new QTextEdit(t_W_Reference);
	m_TE_Reference->setObjectName(QString::fromUtf8("m_TE_Reference"));
	m_TE_Reference->setReadOnly(true);
	m_TE_Reference->setLineWrapMode(QTextEdit::NoWrap);

	t_vLayout_Reference->addWidget(m_TE_Reference);
	m_stackedLayout->addWidget(t_W_Reference);
	//----------------
	QObject::connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	QObject::connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
	connect(m_CB_Type, SIGNAL(currentIndexChanged(int)), this, SLOT(onTypeCurrentIndexChanged_slot(int)));
	connect(m_stackedLayout, SIGNAL(currentChanged(int)), this, SLOT(onStackedCurrentChanged_slot(int)));
	connect(m_LE_Color, SIGNAL(textChanged(const QString&)), this, SLOT(onColorTextChanged_slot(const QString&)));
	connect(m_LE_Reference, SIGNAL(textChanged(const QString&)), this, SLOT(onReferenceTextChanged_slot(const QString&)));
	connect(m_CB_ShowRichText, SIGNAL(stateChanged(int)), this, SLOT(onShowRichTextStateChanged_slot(int)));
	//--------------------------------------------------------
	m_CB_Type->addItem("(1)Reference", (uint32_t)Res_value::_DataType::TYPE_REFERENCE);			//0x01
	m_CB_Type->addItem("(2)Attribute", (uint32_t)Res_value::_DataType::TYPE_ATTRIBUTE);			//0x02
	m_CB_Type->addItem("(3)String", (uint32_t)Res_value::_DataType::TYPE_STRING);				//0x03
	m_CB_Type->addItem("(4)Float", (uint32_t)Res_value::_DataType::TYPE_FLOAT);					//0x04
	m_CB_Type->addItem("(5)Dimension", (uint32_t)Res_value::_DataType::TYPE_DIMENSION);			//0x05
	m_CB_Type->addItem("(6)Fraction", (uint32_t)Res_value::_DataType::TYPE_FRACTION);			//0x06
	m_CB_Type->addItem("(7)Dynamic_Reference", (uint32_t)Res_value::_DataType::TYPE_DYNAMIC_REFERENCE);	//0x07
	m_CB_Type->addItem("(8)Dynamic_Attribute", (uint32_t)Res_value::_DataType::TYPE_DYNAMIC_ATTRIBUTE);	//0x08
	m_CB_Type->addItem("(16)Int_Dec", (uint32_t)Res_value::_DataType::TYPE_INT_DEC);			//0x10
	m_CB_Type->addItem("(17)Int_Hex", (uint32_t)Res_value::_DataType::TYPE_INT_HEX);			//0x11
	m_CB_Type->addItem("(18)Boolean", (uint32_t)Res_value::_DataType::TYPE_INT_BOOLEAN);		//0x12
	m_CB_Type->addItem("(28)Color_ARGB8", (uint32_t)Res_value::_DataType::TYPE_INT_COLOR_ARGB8);//0x1c
	m_CB_Type->addItem("(29)Color_RGB8", (uint32_t)Res_value::_DataType::TYPE_INT_COLOR_RGB8);	//0x1d
	m_CB_Type->addItem("(30)Color_ARGB4", (uint32_t)Res_value::_DataType::TYPE_INT_COLOR_ARGB4);//0x1e
	m_CB_Type->addItem("(31)Color_RGB4", (uint32_t)Res_value::_DataType::TYPE_INT_COLOR_RGB4);	//0x1f

	m_CB_Boolen->addItem("false", 0);
	m_CB_Boolen->addItem("true", 1);
	//--------------------------------------------------------
	m_Type2Page.insert((uint32_t)Res_value::_DataType::TYPE_REFERENCE, 5);
	m_Type2Page.insert((uint32_t)Res_value::_DataType::TYPE_ATTRIBUTE, 5);
	m_Type2Page.insert((uint32_t)Res_value::_DataType::TYPE_STRING, 0);
	m_Type2Page.insert((uint32_t)Res_value::_DataType::TYPE_FLOAT, 2);
	m_Type2Page.insert((uint32_t)Res_value::_DataType::TYPE_DIMENSION, 2);
	m_Type2Page.insert((uint32_t)Res_value::_DataType::TYPE_FRACTION, 2);
	m_Type2Page.insert((uint32_t)Res_value::_DataType::TYPE_DYNAMIC_REFERENCE, 5);
	m_Type2Page.insert((uint32_t)Res_value::_DataType::TYPE_DYNAMIC_ATTRIBUTE, 5);
	m_Type2Page.insert((uint32_t)Res_value::_DataType::TYPE_INT_DEC, 2);
	m_Type2Page.insert((uint32_t)Res_value::_DataType::TYPE_INT_HEX, 3);
	m_Type2Page.insert((uint32_t)Res_value::_DataType::TYPE_INT_BOOLEAN, 4);
	m_Type2Page.insert((uint32_t)Res_value::_DataType::TYPE_INT_COLOR_ARGB8, 1);
	m_Type2Page.insert((uint32_t)Res_value::_DataType::TYPE_INT_COLOR_RGB8, 1);
	m_Type2Page.insert((uint32_t)Res_value::_DataType::TYPE_INT_COLOR_ARGB4, 1);
	m_Type2Page.insert((uint32_t)Res_value::_DataType::TYPE_INT_COLOR_RGB4, 1);
}
void QEditDialog::setTablePackage(QTablePackage* _package)
{
	m_tablePackage = _package;
}
void QEditDialog::setKeyStringPool(QStringPool* _keyStringPool)
{
	m_keyStringPool = _keyStringPool;
}
void QEditDialog::setData(uint32_t _type, uint32_t _data, const QString& _sdata)
{
	m_data = _data;
	m_sdata = _sdata;
	int t_newIdx = m_CB_Type->findData(_type);
	if (t_newIdx == m_CB_Type->currentIndex())
		onTypeCurrentIndexChanged_slot(t_newIdx);
	else
		m_CB_Type->setCurrentIndex(t_newIdx);
}
void QEditDialog::onTypeCurrentIndexChanged_slot(int _index)
{
	if (_index < 0)
		return;
	uint32_t t_type = m_CB_Type->currentData().toUInt();
	int t_page = m_Type2Page[t_type];
	if (m_stackedLayout->currentIndex() != t_page)
		m_stackedLayout->setCurrentIndex(m_Type2Page[t_type]);
	else
		onStackedCurrentChanged_slot(t_page);
}
void QEditDialog::onStackedCurrentChanged_slot(int _index)
{
	Res_value t_value;
	t_value.data = m_data;
	t_value.dataType = Res_value::_DataType(m_CB_Type->currentData().toUInt());
	switch (_index)
	{
	case 0:
		m_TE_String->setPlainText(m_sdata);
		m_CB_ShowRichText->setChecked(false);
		break;
	case 1:
		m_LE_Color->setText(resValue2String(m_LE_Name->text(), t_value, PArscRichString()));
		break;
	case 2:
		m_LE_Digital->setText(resValue2String(m_LE_Name->text(), t_value, PArscRichString()));
		break;
	case 3:
		m_LE_Hex->setText(resValue2String(m_LE_Name->text(), t_value, PArscRichString()));
		break;
	case 4:
		m_CB_Boolen->setCurrentIndex(m_data == 0 ? 0 : 1);
		break;
	case 5:
		m_LE_Reference->setText(resValue2String(m_LE_Name->text(), t_value, PArscRichString()));
		break;
	}
}
QRegExp g_hexRegExp(".?0[xX]([0-9a-fA-F]{8})");
static uint32_t getHexTextData(const QString& _str, bool* _ok)
{
	int t_index = g_hexRegExp.indexIn(_str);
	if (t_index < 0)
	{
		if (_ok != NULL)
			*_ok = false;
		return 0;
	}
	if (_ok != NULL)
		*_ok = true;
	return g_hexRegExp.capturedTexts()[1].toUInt(NULL, 16);
}
void QEditDialog::onColorTextChanged_slot(const QString& _text)
{
	bool t_ok;
	uint32_t t_data = getHexTextData(_text, &t_ok);
	if (!t_ok)
		return;

	QPixmap t_pixmap(32, 32);
	t_pixmap.fill(QColor(QRgba64::fromArgb32(t_data)));
	m_LB_Color->setPixmap(t_pixmap);
}
void QEditDialog::onReferenceTextChanged_slot(const QString& _text)
{
	bool t_ok;
	uint32_t t_data = getHexTextData(_text, &t_ok);
	if (!t_ok)
		return;
	QString t_prefix = "@";
	bool t_addType = true;
	if (_text[0] == '?')
	{
		t_prefix = "?";
		t_addType = false;
	}
	QString t_keyString;
	if (m_tablePackage != NULL)
		t_keyString = m_tablePackage->getKeyString(t_prefix, t_addType, t_data);
	else if ((t_data >> 16) == 0)
		t_keyString = m_keyStringPool->getGuidRef(t_data)->string;
	else if ((t_data >> 24) == 1 && (t_data & 0xFF0000) != 0)
		t_keyString = g_publicFinal->getDataName(t_data)->string;
	else if ((t_data >> 24) == 1 && (t_data & 0xFF0000) == 0)
		t_keyString = QString("0x%1").arg(t_data, 8, 16, QChar('0'));
	else
		t_keyString = QString("0x%1").arg(t_data, 8, 16, QChar('0'));

	m_TE_Reference->setText(t_keyString);
}
void QEditDialog::onShowRichTextStateChanged_slot(int)
{
	QString t_text = m_TE_String->toPlainText();
	if (t_text.indexOf("</") >= 0)
		t_text.replace(QString("\\n"), QString("<br>"));
	else
		t_text.replace(QString("\\n"), QChar(0x0A));
	m_TB_String->setText(t_text);

	m_SW_String->setCurrentIndex(m_CB_ShowRichText->isChecked() ? 1 : 0);
}
uint32_t QEditDialog::getType(void) const
{
	return m_CB_Type->currentData().toUInt();
}
uint32_t QEditDialog::qstringToData(const QString& _name, Res_value::_DataType _dataType, const QString& _str)
{
	switch (_dataType)
	{
	case Res_value::_DataType::TYPE_STRING:
		Q_ASSERT(false);
		return 0;
	case Res_value::_DataType::TYPE_INT_COLOR_ARGB8:
	case Res_value::_DataType::TYPE_INT_COLOR_RGB8:
	case Res_value::_DataType::TYPE_INT_COLOR_ARGB4:
	case Res_value::_DataType::TYPE_INT_COLOR_RGB4:
		return getHexTextData(_str, NULL);
	case Res_value::_DataType::TYPE_INT_DEC:
		return g_androidAttribute->string2value(_name, _str);//_str.toUInt(NULL, 10);
	case Res_value::_DataType::TYPE_FLOAT:
		{
			float t_f = _str.toFloat();
			return *reinterpret_cast<uint32_t*>(&t_f);
		}
	case Res_value::_DataType::TYPE_DIMENSION:
		{
			QRegularExpressionMatch t_match = g_DFRegExp.match(_str);
			if (t_match.hasMatch())
				return getDimensionFractionData(t_match.captured(1).toFloat(), 1, t_match.captured(2), DIMENSION_UNIT_STRS, 6);
			return 0;
		}
	case Res_value::_DataType::TYPE_FRACTION:
		{
			QRegularExpressionMatch t_match = g_DFRegExp.match(_str);
			if (t_match.hasMatch())
				return getDimensionFractionData(t_match.captured(1).toFloat(), 100, t_match.captured(2), FRACTION_UNIT_STRS, 2);
			return 0;
		}
		return 0;
	case Res_value::_DataType::TYPE_ATTRIBUTE:
	case Res_value::_DataType::TYPE_DYNAMIC_ATTRIBUTE:
		return getHexTextData(_str, NULL);
	case Res_value::_DataType::TYPE_INT_HEX:
		return g_androidAttribute->string2value(_name, _str);//getHexTextData(_str, NULL);
	case Res_value::_DataType::TYPE_INT_BOOLEAN:
		return _str.toLower() == QString("true") ? uint32_t(-1) : 0;
	case Res_value::_DataType::TYPE_REFERENCE:
	case Res_value::_DataType::TYPE_DYNAMIC_REFERENCE:
		return getHexTextData(_str, NULL);
	default:
		return 0;
	}
}
uint32_t QEditDialog::getData(void) const
{
	Res_value::_DataType t_dataType = Res_value::_DataType(m_CB_Type->currentData().toUInt());

	switch (t_dataType)
	{
	case Res_value::_DataType::TYPE_STRING:
		Q_ASSERT(false);
		return 0;
	case Res_value::_DataType::TYPE_INT_COLOR_ARGB8:
	case Res_value::_DataType::TYPE_INT_COLOR_RGB8:
	case Res_value::_DataType::TYPE_INT_COLOR_ARGB4:
	case Res_value::_DataType::TYPE_INT_COLOR_RGB4:
		return qstringToData(m_LE_Name->text(), t_dataType, m_LE_Color->text());
	case Res_value::_DataType::TYPE_INT_DEC:
	case Res_value::_DataType::TYPE_FLOAT:
	case Res_value::_DataType::TYPE_DIMENSION:
	case Res_value::_DataType::TYPE_FRACTION:
		return qstringToData(m_LE_Name->text(), t_dataType, m_LE_Digital->text());
	case Res_value::_DataType::TYPE_ATTRIBUTE:
	case Res_value::_DataType::TYPE_DYNAMIC_ATTRIBUTE:
	case Res_value::_DataType::TYPE_INT_HEX:
		return qstringToData(m_LE_Name->text(), t_dataType, m_LE_Hex->text());
	case Res_value::_DataType::TYPE_INT_BOOLEAN:
		return qstringToData(m_LE_Name->text(), t_dataType, m_CB_Boolen->currentText());
	case Res_value::_DataType::TYPE_REFERENCE:
	case Res_value::_DataType::TYPE_DYNAMIC_REFERENCE:
		return qstringToData(m_LE_Name->text(), t_dataType, m_LE_Reference->text());
	default:
		return 0;
	}
}
QString QEditDialog::getSData(void) const
{
	return m_TE_String->toPlainText();
}
