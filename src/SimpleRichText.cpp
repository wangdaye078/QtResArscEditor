#include "SimpleRichText.h"
#include "QStringPool.h"
#include <QRegExp>
#include <iterator>

QString encodeRichText(const TArscRichString* _input)
{
	QMultiMap_modify<quint32, QString> t_tags;
	for (int i = 0; i < _input->styles.size(); ++i)
	{
		const TArscStringStyle& t_style = _input->styles[i];
		//为了在相同KEY的情况下，要插在最后一个，所以需要用upperBound找所有相同KEY后面那一个位置
		QMap<quint32, QString>::const_iterator t_iter = t_tags.upperBound(t_style.firstChar * 2);
		if (t_iter != t_tags.end())
			t_tags.insert_modify(t_iter, t_style.firstChar * 2, "<" + t_style.ref->string + ">");
		else
			t_tags.insert(t_style.firstChar * 2, "<" + t_style.ref->string + ">");
		//在相同KEY的情况下，需要插最前面，所以需要用upperBound找所有相同KEY的第一个
		t_iter = t_tags.lowerBound(t_style.lastChar * 2 + 1);
		if (t_iter != t_tags.end())
			t_tags.insert_modify(t_iter, t_style.lastChar * 2 + 1, "</" + t_style.ref->string + ">");
		else
			t_tags.insert(t_style.lastChar * 2 + 1, "</" + t_style.ref->string + ">");
	}
	QString t_result(_input->string);
	for (QMultiMap<quint32, QString>::iterator i = t_tags.end(); i != t_tags.begin(); --i)
	{
		t_result.insert(((i - 1).key() + 1) / 2, (i - 1).value());
	}
	return t_result;
}


QRegExp g_rBeing("<([a-zA-Z]+)>");
QRegExp g_rend("<(/[a-zA-Z]+)>");
void decodeRichText(QString& _input, QVector<TArscStringStyle>& _spans, int _endPos)
{
	if (_endPos == -1)
		_endPos = _input.length();

	int t_bPos;
	while ((t_bPos = g_rBeing.indexIn(_input)) != -1 && t_bPos < _endPos)
	{
		_spans.append(TArscStringStyle());
		int t_lastIdx = _spans.size() - 1;
		//不能这么写，因为下面如果执行到递归，插入新项后，这个地址可能就变成野指针了。
		//ResStringPool_span& t_span = _spans.spans.last();
		PArscRichString t_tag_tmp(new TArscRichString());
		t_tag_tmp->string = g_rBeing.capturedTexts()[1];
		Q_ASSERT(g_publicStrPool->getRefCount(t_tag_tmp) != 0);
		_spans[t_lastIdx].ref = g_publicStrPool->getRichString(t_tag_tmp);
		_spans[t_lastIdx].firstChar = t_bPos;

		int t_matchedLength = g_rBeing.matchedLength();
		_input.remove(t_bPos, t_matchedLength);
		_endPos -= t_matchedLength;

		QString t_endtag = "</" + t_tag_tmp->string + ">";
		int t_ePos = _input.indexOf(t_endtag);
		int t_subbpos = 0;
		while ((t_subbpos = g_rBeing.indexIn(_input)) != -1 && t_subbpos < t_ePos)		//中间还有
		{
			decodeRichText(_input, _spans, t_ePos);
			t_ePos = _input.indexOf(t_endtag);
		}
		_input.remove(t_ePos, t_endtag.length());
		_endPos -= t_matchedLength;
		_spans[t_lastIdx].lastChar = t_ePos - 1;
	}
}
TArscRichString* decodeRichText(const QString& _input)
{
	TArscRichString* t_richString = new TArscRichString();
	t_richString->guid = 0;
	t_richString->index = 0;
	t_richString->string = _input;
	decodeRichText(t_richString->string, t_richString->styles, -1);
	return t_richString;
}
