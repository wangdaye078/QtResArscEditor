#include "QArscString.h"
#include "common/QUtf8.h"
#include "common/basicDefine.h"

uint32_t writeStringLen_16(char* _pBuff, const QString& _str)
{
	quint16* t_pBuff = reinterpret_cast<quint16*>(_pBuff);
	if (_str.size() < 0x8000)
	{
		t_pBuff[0] = _str.size();
		return sizeof(quint16);
	}
	else
	{
		t_pBuff[0] = (_str.size() >> 16) | 0x8000;
		t_pBuff[1] = _str.size() & 0xffff;
		return sizeof(quint16) * 2;
	}
}
uint32_t writeStringLen_8(char* _pBuff, const QString& _str, QByteArray& _utf8Array)
{
	qint8* t_pBuff = reinterpret_cast<qint8*>(_pBuff);
	uint32_t t_pos = 0;
	if (_str.size() < 0x80)
		t_pBuff[t_pos++] = _str.size();
	else
	{
		t_pBuff[t_pos++] = (_str.size() >> 8) | 0x80;
		t_pBuff[t_pos++] = _str.size() & 0xff;
	}
	_utf8Array = QString_to_utf8(_str);
	if (_utf8Array.size() < 0x80)
		t_pBuff[t_pos++] = _utf8Array.size();
	else
	{
		t_pBuff[t_pos++] = (_utf8Array.size() >> 8) | 0x80;
		t_pBuff[t_pos++] = _utf8Array.size() & 0xff;
	}
	return t_pos;
}
uint32_t readStringLen_16(const char*& _pBuff)
{
	//看逻辑，UTF16字符串最长可以有0x7FFF FFFF长
	quint16 t_u16len = readValue<quint16>(_pBuff);
	if ((t_u16len & 0x8000) == 0)
		return t_u16len;
	else
		return ((t_u16len & 0x7FFF) << 16) + readValue<ushort>(_pBuff);
}
uint readStringLen_8(const char*& _pBuff)
{
	//看逻辑，UTF8字符串最长可以有0x7F FF长
	uchar t_u16len = readValue<uchar>(_pBuff);
	if ((t_u16len & 0x80) != 0)
		/*uchar t_u16len_fix = */readValue<uchar>(_pBuff);

	uchar t_u8len = readValue<uchar>(_pBuff);
	if ((t_u8len & 0x80) == 0)
		return t_u8len;
	else
		return ((t_u8len & 0x7F) << 8) + readValue<uchar>(_pBuff);
}
QString readString(const char* _pBuff, bool _isUTF8)
{
	QString t_str;
	if (!_isUTF8)
	{
		uint t_len = readStringLen_16(_pBuff);
		t_str = WCHARToQStringN(_pBuff, t_len);
		Q_ASSERT(*(reinterpret_cast<const ushort*>(_pBuff) + t_len) == 0);
	}
	else
	{
		uint t_len = readStringLen_8(_pBuff);
		t_str = utf8_to_QString(_pBuff, t_len);
		QByteArray t_arr = t_str.toUtf8();
		Q_ASSERT(_pBuff[t_len] == 0);
	}
	return t_str;
}
void writeString(QByteArray& _buff, const QString& _string, bool _isUTF8)
{
	char t_len[4];
	if (!_isUTF8)
	{
		uint32_t t_lenlen = writeStringLen_16(t_len, _string);
		_buff.append(t_len, t_lenlen);
		_buff.append(reinterpret_cast<const char*>(_string.utf16()), (_string.length() + 1) * sizeof(ushort));
	}
	else
	{
		QByteArray t_utf8Array;
		uint32_t t_lenlen = writeStringLen_8(t_len, _string, t_utf8Array);
		_buff.append(t_len, t_lenlen);
		_buff.append(t_utf8Array.data(), t_utf8Array.size() + 1);
	}
}