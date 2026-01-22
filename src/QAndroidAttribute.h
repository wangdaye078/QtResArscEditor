//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor2\QAndroidAttribute.h
//	desc:		
//
//	created:	wangdaye 15:1:2026   13:19
//********************************************************************
#ifndef QAndroidAttribute_h__
#define QAndroidAttribute_h__
#include <QMap>
#include <QObject>
#include <QString>

class QDomNode;

struct TAttributeFlag
{
	QString Name;
	uint32_t value;
	bool bit;		//按位匹配？
	bool operator < (const TAttributeFlag& _r) const
	{
		if (value != _r.value)
			return value > _r.value;	//值大的排前面
		return Name < _r.Name;
	}
};
struct TAttributeFlags
{
	QMap<TAttributeFlag, int> flags;
	QMap<QString, int> flagValueMap;
	void appendFlag(const QDomNode& _node);
	uint32_t string2value(const QString& _sname, const QString& _svalue);
	QString value2string(const QString& _sname, uint32_t _value);
};
class QAndroidAttribute : public QObject
{
	Q_OBJECT
public:
	QAndroidAttribute(QObject* _parent);
	~QAndroidAttribute();
	uint32_t string2value(const QString& _sname, const QString& _svalue);
	QString value2string(const QString& _sname, uint32_t _value, bool _hex);
	bool contains(const QString& _sname);
private:
	void initAttributes(const QString& _path);
	void analysisNode(const QDomNode& _node);
private:
	QMap<QString, TAttributeFlags> m_attrs;
};
extern QAndroidAttribute* g_androidAttribute;

#endif // QAndroidAttribute_h__
