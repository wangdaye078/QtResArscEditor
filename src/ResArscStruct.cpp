#include "ResArscStruct.h"
#include "basicDefine.h"
const char* DIMENSION_UNIT_STRS[6] = { "px", "dip", "sp", "pt", "in", "mm" };
const char* FRACTION_UNIT_STRS[2] = { "%", "%p" };
float MANTISSA_MULT = 1.0f / (1 << 8);
float RADIX_MULTS[4] = { 1.0f * MANTISSA_MULT, 1.0f / (1 << 7) * MANTISSA_MULT,
		1.0f / (1 << 15) * MANTISSA_MULT, 1.0f / (1 << 23) * MANTISSA_MULT };


QMap<int, QString> DENSITY_DPI_VALUES;
const char* ORIENTATION_VALUES[4] = { "", "port", "land", "square" };
const char* KEYBOARD_VALUES[4] = { "", "nokeys", "qwerty", "12key" };
const char* KEYBOARDHIDDEN_VALUES[4] = { "", "keysexposed", "keyshidden", "keyssoft" };
const char* NAVIGATION_VALUES[5] = { "", "nonav", "dpad", "trackball", "wheel" };
const char* NAVIGATIONHIDDEN_VALUES[10] = { "","", "", "", "navexposed","", "", "", "navhidden", "" };
const char* GRAMMATICAL_GENDER_VALUES[4] = { "", "neuter", "feminine", "masculine" };

// screenLayout 0 1 2 3 |5 6|7 8
//SCREENLAYOUT_SIZE_VALUES | 4位未使用 |SCREENLAYOUT_LONG_VALUES | SCREENLAYOUT_LAYOUTDIR_VALUES
const char* SCREENLAYOUT_SIZE_VALUES[5] = { "", "small", "normal", "large", "xlarge" };
const char* SCREENLAYOUT_LONG_VALUES[3] = { "", "notlong", "long" };
const char* SCREENLAYOUT_LAYOUTDIR_VALUES[3] = { "", "ldltr" , "ldrtl" };

const char* SCREENLAYOUT_ROUND_VALUES[3] = { "", "notround" , "round" };
const char* TOUCHSCREEN_VALUES[4] = { "", "notouch" , "stylus", "finger" };
const char* UI_MODE_NIGHT_VALUES[3] = { "","notnight" ,"night" };
const char* UI_MODE_TYPE_VALUES[16] = { "", "", "desk" , "car" , "television" , "appliance" , "watch" ,"vrheadset", "", "", "", "godzillaui", "smallui", "mediumui", "largeui", "hugeui" };

// colorMode  0 1|2 3|5 6 7 8
//COLOR_WIDE_VALUES | COLOR_HDR_VALUES |高4位没用
const char* COLOR_WIDE_VALUES[3] = { "","nowidecg" ,"widecg" };
const char* COLOR_HDR_VALUES[3] = { "","lowdr" ,"highdr" };
void initTableConfig(void)
{
	DENSITY_DPI_VALUES.insert(DENSITY_DPI_UNDEFINED, "");
	DENSITY_DPI_VALUES.insert(DENSITY_DPI_LDPI, "ldpi");
	DENSITY_DPI_VALUES.insert(DENSITY_DPI_MDPI, "mdpi");
	DENSITY_DPI_VALUES.insert(DENSITY_DPI_TVDPI, "tvdpi");
	DENSITY_DPI_VALUES.insert(DENSITY_DPI_HDPI, "hdpi");
	DENSITY_DPI_VALUES.insert(DENSITY_DPI_XHDPI, "xhdpi");
	DENSITY_DPI_VALUES.insert(DENSITY_DPI_XXHDPI, "xxhdpi");
	DENSITY_DPI_VALUES.insert(DENSITY_DPI_XXXHDPI, "xxxhdpi");
	DENSITY_DPI_VALUES.insert(DENSITY_DPI_ANY, "anydpi");
	DENSITY_DPI_VALUES.insert(DENSITY_DPI_NONE, "nnndpi");
}
void packLanguageOrRegion(const char* _in, int _inlen, char* _out, int _base)
{
	Q_ASSERT(_inlen == 2 || _inlen == 3);
	if (_inlen == 2)
	{
		_out[0] = _in[0];
		_out[1] = _in[1];
	}
	else
	{
		_out[1] = (char)(_in[0] - _base);
		_out[1] |= ((char)(_in[1] - _base) << 5);
		_out[0] = ((char)(_in[1] - _base) >> 3);
		_out[0] |= ((char)(_in[2] - _base) << 2);
		_out[0] |= 0x80;
	}
}
QString unpackLanguageOrRegion(const char* _value, int _base)
{
	if ((_value[0] & 0x80) != 0)
	{
		char t_result[3];
		t_result[0] = (char)(_base + (_value[1] & 0x1F));
		t_result[1] = (char)(_base + ((_value[1] & 0xE0) >> 5) + ((_value[0] & 0x03) << 3));
		t_result[2] = (char)(_base + ((_value[0] & 0x7C) >> 2));
		return CHARToQStringN(t_result, 3);
	}
	return CHARToQStringN(_value, 2);
}
QString getLocaleString(const ResTable_config& _tableConfig)
{
	QString t_locale;
	if (_tableConfig.language[0] != '\0')
		t_locale = unpackLanguageOrRegion(_tableConfig.language, 0x61);
	if (t_locale.isEmpty())
		return t_locale;
	if (_tableConfig.localeScript[0] == '\0' && _tableConfig.localeVariant[0] == '\0')
	{
		if (_tableConfig.country[0] != '\0')
			return t_locale + "-r" + unpackLanguageOrRegion(_tableConfig.country, 0x30);
		else
			return t_locale;
	}
	t_locale = "b+" + t_locale;
	//复制下，在后面加个0
	char t_localeScript[5] = { 0,0,0,0,0 };
	char t_localeVariant[9] = { 0,0,0,0,0,0,0,0,0 };
	memcpy(t_localeScript, _tableConfig.localeScript, _countof(_tableConfig.localeScript));
	memcpy(t_localeVariant, _tableConfig.localeVariant, _countof(_tableConfig.localeVariant));

	if (t_localeScript[0] != '\0')
		t_locale += "+" + QString(t_localeScript);
	if (_tableConfig.country[0] != '\0')
		t_locale += "+" + unpackLanguageOrRegion(_tableConfig.country, 0x30);
	if (t_localeVariant[0] != '\0')
		t_locale += "+" + QString(t_localeVariant);
	return t_locale;
}
QString tableConfig2String(const QString& _prefix, const ResTable_config& _tableConfig)
{
	QStringList t_suffix;
	if (_tableConfig.mcc != 0)
		t_suffix.append("mcc" + QString("%1").arg(_tableConfig.mcc, 3, 10, QChar('0')));
	if (_tableConfig.mnc != 0 && _tableConfig.mnc != 0xFFFF)
		t_suffix.append("mnc" + (_tableConfig.mnc == 0xFFFF ? "00" : "0" + QString::number(_tableConfig.mnc)));
	if (_tableConfig.language[0] != '\0')
		t_suffix.append(getLocaleString(_tableConfig));
	if (_tableConfig.orientation != 0)
		t_suffix.append(ORIENTATION_VALUES[_tableConfig.orientation]);
	if (_tableConfig.touchscreen != 0)
		t_suffix.append(TOUCHSCREEN_VALUES[_tableConfig.touchscreen]);
	if (_tableConfig.density != 0)
		t_suffix.append(DENSITY_DPI_VALUES.contains(_tableConfig.density) ? DENSITY_DPI_VALUES[_tableConfig.density] : (QString::number(_tableConfig.mnc) + "dpi"));
	if (_tableConfig.keyboard != 0)
		t_suffix.append(KEYBOARD_VALUES[_tableConfig.keyboard]);
	if (_tableConfig.navigation != 0)
		t_suffix.append(NAVIGATION_VALUES[_tableConfig.navigation]);
	if ((_tableConfig.inputFlags & 0x0C) != 0)
		t_suffix.append(NAVIGATIONHIDDEN_VALUES[_tableConfig.inputFlags & 0x0C]);
	if ((_tableConfig.inputFlags & 0x03) != 0)
		t_suffix.append(KEYBOARDHIDDEN_VALUES[_tableConfig.inputFlags & 0x03]);
	if (_tableConfig.grammaticalInflection != 0)
		t_suffix.append(GRAMMATICAL_GENDER_VALUES[_tableConfig.grammaticalInflection]);
	if (_tableConfig.screenWidth != 0 && _tableConfig.screenHeight != 0)
		t_suffix.append(QString::number(_tableConfig.screenWidth) + "x" + QString::number(_tableConfig.screenHeight));
	if (_tableConfig.screenWidthDp != 0)
		t_suffix.append("w" + QString::number(_tableConfig.screenWidthDp) + "dp");
	if (_tableConfig.screenHeightDp != 0)
		t_suffix.append("h" + QString::number(_tableConfig.screenHeightDp) + "dp");
	if (_tableConfig.sdkVersion != 0)
		t_suffix.append("v" + QString::number(_tableConfig.sdkVersion));
	if ((_tableConfig.screenLayout & 0xF) != 0)
		t_suffix.append(SCREENLAYOUT_SIZE_VALUES[_tableConfig.screenLayout & 0xF]);
	if ((_tableConfig.screenLayout & 0x30) != 0)
		t_suffix.append(SCREENLAYOUT_LONG_VALUES[(_tableConfig.screenLayout & 0x30) >> 4]);
	if ((_tableConfig.screenLayout & 0xC0) != 0)
		t_suffix.append(SCREENLAYOUT_LAYOUTDIR_VALUES[(_tableConfig.screenLayout & 0xC0) >> 6]);
	if ((_tableConfig.uiMode & 0x0F) != 0)
		t_suffix.append(UI_MODE_TYPE_VALUES[_tableConfig.uiMode & 0x0F]);
	if (_tableConfig.uiMode >> 4 != 0)
		t_suffix.append(UI_MODE_NIGHT_VALUES[_tableConfig.uiMode >> 4]);
	if (_tableConfig.smallestScreenWidthDp != 0)
		t_suffix.append("sw" + QString::number(_tableConfig.smallestScreenWidthDp) + "dp");
	if ((_tableConfig.screenLayout2 & 0x03) != 0)
		t_suffix.append(SCREENLAYOUT_ROUND_VALUES[_tableConfig.screenLayout2 & 0x03]);
	if ((_tableConfig.colorMode & 0x03) != 0)
		t_suffix.append(COLOR_WIDE_VALUES[_tableConfig.screenLayout2 & 0x03]);
	if ((_tableConfig.colorMode & 0x0C) != 0)
		t_suffix.append(COLOR_HDR_VALUES[(_tableConfig.screenLayout2 & 0x0C) >> 2]);

	if (t_suffix.size() == 0)
		return "default";
	return _prefix + "-" + t_suffix.join("-");
}
uint32_t getTableConfigMask(const ResTable_config& _tableConfig)
{
	uint32_t t_mask = 0;
	if (_tableConfig.mcc != 0)
		t_mask |= ACONFIGURATION_MCC;
	if (_tableConfig.mnc != 0 && _tableConfig.mnc != 0xFFFF)
		t_mask |= ACONFIGURATION_MNC;
	if (_tableConfig.locale != 0)
		t_mask |= ACONFIGURATION_LOCALE;
	if (_tableConfig.orientation != 0)
		t_mask |= ACONFIGURATION_ORIENTATION;
	if (_tableConfig.touchscreen != 0)
		t_mask |= ACONFIGURATION_TOUCHSCREEN;
	if (_tableConfig.density != 0)
		t_mask |= ACONFIGURATION_DENSITY;
	if (_tableConfig.keyboard != 0)
		t_mask |= ACONFIGURATION_KEYBOARD;
	if (_tableConfig.navigation != 0)
		t_mask |= ACONFIGURATION_NAVIGATION;
	if ((_tableConfig.inputFlags) != 0)
		t_mask |= ACONFIGURATION_KEYBOARD_HIDDEN;
	if (_tableConfig.grammaticalInflection != 0)
		t_mask |= ACONFIGURATION_GRAMMATICAL_GENDER;
	if (_tableConfig.screenSize != 0)
		t_mask |= ACONFIGURATION_SCREEN_SIZE;
	if (_tableConfig.sdkVersion != 0)
		t_mask |= ACONFIGURATION_VERSION;
	if ((_tableConfig.screenLayout & 0x3F) != 0)
		t_mask |= ACONFIGURATION_SCREEN_LAYOUT;
	if ((_tableConfig.screenLayout & 0xC0) != 0)
		t_mask |= ACONFIGURATION_LAYOUTDIR;
	if ((_tableConfig.uiMode) != 0)
		t_mask |= ACONFIGURATION_UI_MODE;
	if (_tableConfig.smallestScreenWidthDp != 0)
		t_mask |= ACONFIGURATION_SMALLEST_SCREEN_SIZE;
	if ((_tableConfig.screenLayout2 & 0x03) != 0)
		t_mask |= ACONFIGURATION_SCREEN_ROUND;
	if (_tableConfig.colorMode != 0)
		t_mask |= ACONFIGURATION_COLOR_MODE;
	return t_mask;
}
void TStringPool::makeIndexs(void)
{
	strIndexs.clear();

	for (int i = 0; i < strings.size(); ++i)
	{
		TRichString t_richStr;
		if (i < styles.size())
			t_richStr = TRichString(strings[i], styles[i]);
		else
			t_richStr.str = strings[i];

		Q_ASSERT(!strIndexs.contains(t_richStr));
		strIndexs.insert(t_richStr, quint32(i));
	}
}
void TStringPool::reset(void)
{
	strings.resize(0);
	styles.resize(0);
	strIndexs.clear();
	referenceCount.resize(0);
}
