#include "QUtf8.h"
#include "utf8.h"
#include <QVector>

QString utf8_to_QString(const char* _pBuff, quint32 _len)
{
	int t_needlen = (int)u8decodelen(_pBuff) + 1;
	QVector<ucs4_t> t_ucsBuff(t_needlen, 0);
	int t_decodelen = (int)u8decode(_pBuff, t_ucsBuff.data(), t_needlen, NULL);
	QVector<ushort> t_utf16;
	for (int i = 0, j = 0; i < t_decodelen; ++i)
	{
		uint32_t t_ucs4 = t_ucsBuff[i];
		if (t_ucs4 > 0x10FFFF)// || (0xD800 <= t_ucs4 && t_ucs4 <= 0xDFFF))
		{
			Q_ASSERT(false);
		}
		else if (t_ucsBuff[i] > 0xFFFF)
		{
			uint32_t t_ucs = t_ucs4 - 0x10000;
			t_utf16.push_back((ushort)((t_ucs >> 10) + 0xD800));
			t_utf16.push_back((ushort)((t_ucs & 0x3FF) + 0xDC00));
		}
		else if (0xD800 <= t_ucs4 && t_ucs4 <= 0xDFFF)
		{
			//utf-16的代理项范围，规范情况下是不允许直接使用的，但是某些不规范编码的情况下，也会出现这种情况
			t_utf16.push_back((ushort)t_ucs4);
		}
		else
			t_utf16.push_back((ushort)t_ucs4);
	}
	return QString::fromUtf16(t_utf16.data(), t_utf16.length());
}

QByteArray QString_to_utf8(const QString& _str)
{
	QVector<ucs4_t> t_ucsBuff;
	const ushort* t_pstr = _str.utf16();
	for (int i = 0; i < _str.length(); ++i)
	{
		uint32_t t_ucs4;
		ushort t_ch = *(t_pstr + i);
		if (t_ch >= 0xD800 && t_ch <= 0xDBFF && (i + 1) < _str.length())
		{
			ushort t_ch2 = *(t_pstr + i + 1);
			if (t_ch2 >= 0xDC00 && t_ch2 <= 0xDFFF)
			{
				t_ucs4 = ((uint32_t)(t_ch - 0xD800) << 10) + (uint32_t)(t_ch2 - 0xDC00) + 0x10000;
				i++;
			}
			else
			{
				Q_ASSERT(false);
				t_ucs4 = t_ch;
			}
		}
		else
		{
			t_ucs4 = t_ch;
		}
		t_ucsBuff.push_back(t_ucs4);
	}
	t_ucsBuff.push_back(0);
	int t_needlen2 = (int)u8encodelen(t_ucsBuff.data()) + 1;
	QByteArray t_arr(t_needlen2, char('\0'));
	/*int t_encodelen = (int)*/u8encode(t_ucsBuff.data(), t_arr.data(), t_needlen2, NULL);
	t_arr.resize(t_arr.size() - 1);
	return t_arr;
}
