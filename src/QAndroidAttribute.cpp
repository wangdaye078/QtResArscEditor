#include "QAndroidAttribute.h"
#include <QDebug>
#include <QDomDocument>
#include <QFile>
#include <QRegularExpression>

void TAttributeFlags::appendFlag(const QDomNode& _node)
{
	QDomNamedNodeMap t_attrMap = _node.attributes();
	if (t_attrMap.isEmpty())
		return;
	TAttributeFlag t_flag;
	t_flag.Name = t_attrMap.namedItem("name").nodeValue();
	QString t_svalue = t_attrMap.namedItem("value").nodeValue();
	if (t_svalue.startsWith("0x"))
	{
		t_flag.bit = true;
		t_flag.value = t_svalue.toUInt(nullptr, 16);
	}
	else
	{
		t_flag.bit = false;
		t_flag.value = (uint32_t)t_svalue.toInt(nullptr, 10);
	}
	flags.insert(t_flag, 0);
	flagValueMap.insert(t_flag.Name, t_flag.value);
}
uint32_t TAttributeFlags::string2value(const QString& _sname, const QString& _svalue)
{
	uint32_t t_value = 0;
	QStringList t_sflags = _svalue.split("|", Qt::SkipEmptyParts);
	for (QStringList::iterator i = t_sflags.begin(); i != t_sflags.end(); ++i)
	{
		if (flagValueMap.contains(*i))
			t_value += flagValueMap[*i];
		else
			qDebug() << "Attribute:" << _sname << " unknow attr string:" << *i;
	}
	return t_value;
}
QString TAttributeFlags::value2string(const QString& _sname, uint32_t _value)
{
	QString t_svalue;
	//从大到小来匹配标志位
	for (QMap<TAttributeFlag, int>::iterator i = flags.begin(); i != flags.end(); ++i)
	{
		const TAttributeFlag& t_flag = i.key();
		if (t_flag.bit && (_value & t_flag.value) == t_flag.value)
		{
			if (t_flag.value == 0 && t_svalue.length() > 0)
				break;
			t_svalue += t_flag.Name + "|";
			_value ^= t_flag.value;
			if (_value == 0)
				break;
		}
	}
	if (_value != 0)
	{
		for (QMap<TAttributeFlag, int>::iterator i = flags.begin(); i != flags.end(); ++i)
		{
			const TAttributeFlag& t_flag = i.key();
			if (!t_flag.bit && _value == t_flag.value)
			{
				t_svalue += t_flag.Name + "|";
				_value = 0;
				break;
			}
		}
	}
	qDebug() << "Attribute:" << _sname << " unknow attr value:" << _value;
	return t_svalue.mid(0, t_svalue.length() - 1);
}
QAndroidAttribute::QAndroidAttribute(QObject* _parent)
	:QObject(_parent)
{
	initAttributes(":/res/res/attrs.xml");
	initAttributes(":/res/res/attrs_manifest.xml");
}
QAndroidAttribute::~QAndroidAttribute()
{

}
void QAndroidAttribute::initAttributes(const QString& _path)
{
	QFile t_ReadFile(_path);
	if (!t_ReadFile.exists()) return;

	if (!t_ReadFile.open(QFile::ReadOnly))
		return;
	QDomDocument t_domTree;
	if (!t_domTree.setContent(&t_ReadFile))
	{
		t_ReadFile.close();
		return;
	}
	QDomElement t_root = t_domTree.documentElement();
	QDomNode t_subNode = t_root.firstChild();
	while (!t_subNode.isNull())
	{
		analysisNode(t_subNode);
		t_subNode = t_subNode.nextSibling();
	}
	t_ReadFile.close();
}
void QAndroidAttribute::analysisNode(const QDomNode& _node)
{
	QString t_nodeName = _node.nodeName();
	if (t_nodeName != "attr")
	{
		QDomNode t_subNode = _node.firstChild();
		while (!t_subNode.isNull())
		{
			analysisNode(t_subNode);
			t_subNode = t_subNode.nextSibling();
		}
		return;
	}
	if (!_node.hasChildNodes())
		return;
	QDomNamedNodeMap t_attrMap = _node.attributes();
	QString t_name = t_attrMap.namedItem("name").nodeValue();
	Q_ASSERT(!m_attrs.contains("android:" + t_name));
	TAttributeFlags t_flags;
	QDomNode t_flagNode = _node.firstChild();
	while (!t_flagNode.isNull())
	{
		t_flags.appendFlag(t_flagNode);
		t_flagNode = t_flagNode.nextSibling();
	}
	if (t_flags.flags.size() > 0)
		m_attrs.insert("android:" + t_name, t_flags);
}
uint32_t QAndroidAttribute::string2value(const QString& _sname, const QString& _svalue)
{
	bool t_ok = true;
	uint32_t t_value;
	static QRegularExpression t_numberRegExp("^[0-9a-fA-FxX]{1,10}$");
	//只有非数值的字符串才去分析
	if (m_attrs.contains(_sname) && !t_numberRegExp.match(_svalue).hasMatch())
		t_value = m_attrs[_sname].string2value(_sname, _svalue);
	else if (_svalue.startsWith("0x", Qt::CaseInsensitive))
		t_value = _svalue.toUInt(&t_ok, 16);
	else
		t_value = _svalue.toUInt(&t_ok, 10);
	if (!t_ok)
		qDebug() << "Attribute:" << _sname << " unknow attr string:" << _svalue;
	return t_value;
}
QString QAndroidAttribute::value2string(const QString& _sname, uint32_t _value, bool _hex)
{
	if (m_attrs.contains(_sname))
		return m_attrs[_sname].value2string(_sname, _value);
	else if (_hex)
		return QString("0x%1").arg(_value, 0, 16);
	else
		return QString::number(_value);
}
bool QAndroidAttribute::contains(const QString& _sname)
{
	return m_attrs.contains(_sname);
}
QAndroidAttribute* g_androidAttribute;