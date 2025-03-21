#include "QPublicFinal.h"
#include <QDomDocument>
#include <QFile>
QPublicFinal::QPublicFinal(QObject* parent)
	: QObject(parent)
{
	initFinalData();
}
QPublicFinal::~QPublicFinal()
{
}
QString QPublicFinal::getDataName(uint32_t _data)
{
	if (m_finalMap.contains(_data))
		return m_finalMap[_data];
	return "unknow";
}
void QPublicFinal::initFinalData(void)
{
	QFile t_ReadFile(":/res/res/public-final.xml");
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
	QDomElement t_publicDom = t_root.firstChildElement("public");
	while (!t_publicDom.isNull())
	{
		QString t_str = "@android:" + t_publicDom.attribute("type") + "/" + t_publicDom.attribute("name");
		QString t_sid = t_publicDom.attribute("id");
		uint32_t t_id = 0;
		if (t_sid[0] == 0x30 && (t_sid[1] == 0x58 || t_sid[1] == 0x78))
			t_id = t_sid.mid(2).toUInt(NULL, 16);
		else
			t_id = t_sid.toUInt();
		m_finalMap.insert(t_id, t_str);
		t_publicDom = t_publicDom.nextSiblingElement("public");
	}
	t_ReadFile.close();
}
