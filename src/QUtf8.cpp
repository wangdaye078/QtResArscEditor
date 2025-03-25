#include "QUtf8.h"
#include "utf8.h"
#include <QVector>
QString utf8_to_QString(const char* _pBuff, quint32 _len)
{
	int t_needlen = (int)u8decodelen(_pBuff) + 1;
	QVector<ucs4_t> t_ucsBuff(t_needlen, 0);
	int t_decodelen = (int)u8decode(_pBuff, t_ucsBuff.data(), t_needlen, NULL);
	QVector<ushort> t_utf16(t_decodelen + 1, 0);
	for (int i = 0; i < t_decodelen; ++i)
	{
		Q_ASSERT(t_ucsBuff[i] < 0xFFFF);
		t_utf16[i] = (ushort)t_ucsBuff[i];
	}

	return QString::fromUtf16(t_utf16.data(), t_decodelen);;
}
QByteArray QString_to_utf8(const QString& _str)
{
	QVector<ucs4_t> t_ucsBuff2(_str.length() + 1, 0);
	for (int i = 0; i < _str.length(); ++i)
		t_ucsBuff2[i] = *(_str.utf16() + i);

	int t_needlen2 = (int)u8encodelen(t_ucsBuff2.data()) + 1;
	QByteArray t_arr(t_needlen2, char('\0'));
	int t_encodelen = (int)u8encode(t_ucsBuff2.data(), t_arr.data(), t_needlen2, NULL);
	t_arr.resize(t_arr.size() - 1);
	return t_arr;
}