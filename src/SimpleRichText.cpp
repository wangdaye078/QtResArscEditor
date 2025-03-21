#include "SimpleRichText.h"
#include <QRegExp>
#include <QStringList>
#include <windows.h>
#include <codecvt>
#include <iterator>
#include "utf8.h"

void encodeRichText(QString& _input, const TStringPoolSpans& _spanEx, const TStringPool& _stringPool)
{
	QMultiMap_modify<quint32, QString> t_tags;
	for (int i = 0; i < _spanEx.spans.size(); ++i)
	{
		const ResStringPool_span& t_span = _spanEx.spans[i];
		QString t_style = _stringPool.strings[t_span.name.index];
		//为了在相同KEY的情况下，要插在最后一个，所以需要用upperBound找所有相同KEY后面那一个位置
		QMap<quint32, QString>::const_iterator t_iter = t_tags.upperBound(t_span.firstChar * 2);
		if (t_iter != t_tags.end())
			t_tags.insert_modify(t_iter, t_span.firstChar * 2, "<" + t_style + ">");
		else
			t_tags.insert(t_span.firstChar * 2, "<" + t_style + ">");
		//在相同KEY的情况下，需要插最前面，所以需要用upperBound找所有相同KEY的第一个
		t_iter = t_tags.lowerBound(t_span.lastChar * 2 + 1);
		if (t_iter != t_tags.end())
			t_tags.insert_modify(t_iter, t_span.lastChar * 2 + 1, "</" + t_style + ">");
		else
			t_tags.insert(t_span.lastChar * 2 + 1, "</" + t_style + ">");
	}
	for (QMultiMap<quint32, QString>::iterator i = t_tags.end(); i != t_tags.begin(); --i)
	{
		_input.insert(((i - 1).key() + 1) / 2, (i - 1).value());
	}
}
QRegExp g_rBeing("<([a-zA-Z]+)>");
QRegExp g_rend("<(/[a-zA-Z]+)>");
void decodeRichText(QString& _input, TStringPoolSpans& _spanEx, const TStringPool& _stringPool, int _endPos)
{
	if (_endPos == -1)
		_endPos = _input.length();

	int t_bPos;
	while ((t_bPos = g_rBeing.indexIn(_input)) != -1 && t_bPos < _endPos)
	{
		_spanEx.spans.append(ResStringPool_span());
		ResStringPool_span& t_span = _spanEx.spans.last();
		TRichString t_tag(g_rBeing.capturedTexts()[1], TStringPoolSpans());
		Q_ASSERT(_stringPool.strIndexs.contains(t_tag));
		t_span.name.index = _stringPool.strIndexs[t_tag];
		t_span.firstChar = t_bPos;

		int t_matchedLength = g_rBeing.matchedLength();
		_input.remove(t_bPos, t_matchedLength);
		_endPos -= t_matchedLength;

		QString t_endtag = "</" + t_tag.str + ">";
		int t_ePos = _input.indexOf(t_endtag);
		int t_subbpos = 0;
		while ((t_subbpos = g_rBeing.indexIn(_input)) != -1 && t_subbpos < t_ePos)		//中间还有
		{
			decodeRichText(_input, _spanEx, _stringPool, t_ePos);
			t_ePos = _input.indexOf(t_endtag);
		}
		_input.remove(t_ePos, t_endtag.length());
		_endPos -= t_matchedLength;
		t_span.lastChar = t_ePos - 1;
	}
}
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