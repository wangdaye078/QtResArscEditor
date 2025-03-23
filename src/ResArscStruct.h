//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor\src\ResArscStruct.h
//	desc:		
//
//	created:	wangdaye 14:3:2025   20:54
//********************************************************************
#ifndef ResArscStruct_h__
#define ResArscStruct_h__
#include <QSharedPointer>
#include <QMap>
#pragma pack(push, 1)

enum class RES_TYPE : uint16_t
{
	RES_NULL_TYPE = 0x0000,
	RES_STRING_POOL_TYPE = 0x0001,
	RES_TABLE_TYPE = 0x0002,
	RES_XML_TYPE = 0x0003,
	// Chunk types in RES_XML_TYPE
	RES_XML_FIRST_CHUNK_TYPE = 0x0100,
	RES_XML_START_NAMESPACE_TYPE = 0x0100,
	RES_XML_END_NAMESPACE_TYPE = 0x0101,
	RES_XML_START_ELEMENT_TYPE = 0x0102,
	RES_XML_END_ELEMENT_TYPE = 0x0103,
	RES_XML_CDATA_TYPE = 0x0104,
	RES_XML_LAST_CHUNK_TYPE = 0x017f,
	// This contains a uint32_t array mapping strings in the string
	// pool back to resource identifiers.  It is optional.
	RES_XML_RESOURCE_MAP_TYPE = 0x0180,
	// Chunk types in RES_TABLE_TYPE
	RES_TABLE_PACKAGE_TYPE = 0x0200,
	RES_TABLE_TYPE_TYPE = 0x0201,
	RES_TABLE_TYPE_SPEC_TYPE = 0x0202,
	RES_TABLE_LIBRARY_TYPE = 0x0203
};

struct ResChunk_header
{
	RES_TYPE type;		//<comment = "Type identifier for this chunk">;
	uint16_t headerSize;	//<comment = "Size of the chunk header (in bytes)">;
	uint32_t size;			//<comment = "Total size of this chunk (in bytes)">;
};
struct ResTable_header
{
	ResChunk_header header;
	uint32_t packageCount;
};

struct ResStringPool_header
{
	ResChunk_header header;
	uint32_t stringCount;	// <comment = "Number of strings in this pool">;//  (number of uint32_t indices that follow in the data).
	uint32_t styleCount;	// <comment = "Number of style span arrays in the pool">;// (number of uint32_t indices follow the string indices).
	enum
	{
		SORTED_FLAG = 1 << 0,//If set, the string index is sorted by the string values (based on strcmp16()).
		UTF8_FLAG = 1 << 8// String pool is encoded in UTF-8
	};
	uint32_t flags;			// <comment = "Flags">;
	uint32_t stringsStart;	// <comment = "Index from header of the string data">;
	uint32_t stylesStart;	// <comment = "Index from header of the style data">;
};
struct ResStringPool_ref
{
	uint32_t index;		// <comment = "Index into the string pool table">;
	bool operator<(const ResStringPool_ref& _other) const
	{
		return index < _other.index;
	}
};

struct ResStringPool_span
{
	enum {
		END = 0xFFFFFFFF
	};
	ResStringPool_ref name;			//<comment = "This is the name of the span">;
	uint32_t firstChar;			//<comment = "The range of characters in the string that this span applies to">;
	uint32_t lastChar;
	bool operator<(const ResStringPool_span& _other) const
	{
		if (name.index < _other.name.index)
			return true;
		if (name.index > _other.name.index)
			return false;
		if (firstChar < _other.firstChar)
			return true;
		if (firstChar > _other.firstChar)
			return false;
		if (lastChar < _other.lastChar)
			return true;
		if (lastChar > _other.lastChar)
			return false;
		return false;	//相等
	}
	bool operator == (const ResStringPool_span& _other) const
	{
		if (name.index != _other.name.index)
			return false;
		if (firstChar != _other.firstChar)
			return false;
		if (lastChar != _other.lastChar)
			return false;
		return true;
	}
};

struct ResTable_package
{
	ResChunk_header header;
	uint32_t id;				//<comment = "Package IDs">;
	uint16_t name[128];		//<comment = "Actual name of this package">;
	uint32_t typeStrings;		//<comment = "Offset to a ResStringPool_header defining the resource type symbol table">;
	uint32_t lastPublicType;	//<comment = "Last index into typeStrings that is for public use by others">;
	uint32_t keyStrings;		//<comment = "Offset to a ResStringPool_header defining the resource key symbol table">;
	uint32_t lastPublicKey;		//<comment = "Last index into keyStrings that is for public use by others">;
	uint32_t typeIdOffset;	//为0，放弃
};

struct ResTable_typeSpec
{
	ResChunk_header header;
	uint8_t id;				// <comment = "The type identifier this chunk is holding">;
	uint8_t res0;				// <comment = "Must be 0">;
	uint16_t res1;			// 是子项的数量，比如有多少种本地化
	uint32_t entryCount;		// <comment = "Number of uint32_t entry configuration masks that follow">;
};

enum EDENSITY_DPI
{
	DENSITY_DPI_UNDEFINED = 0,
	DENSITY_DPI_LDPI = 120,
	DENSITY_DPI_MDPI = 160,
	DENSITY_DPI_TVDPI = 213,
	DENSITY_DPI_HDPI = 240,
	DENSITY_DPI_XHDPI = 320,
	DENSITY_DPI_XXHDPI = 480,
	DENSITY_DPI_XXXHDPI = 640,
	DENSITY_DPI_ANY = 0xFFFE,
	DENSITY_DPI_NONE = 0xFFFF
};
enum ACONFIGURATION {
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#MccQualifier">mcc</a>
	 * configuration.
	 */
	ACONFIGURATION_MCC = 0x0001,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#MccQualifier">mnc</a>
	 * configuration.
	 */
	ACONFIGURATION_MNC = 0x0002,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#LocaleQualifier">locale</a>
	 * configuration.
	 */
	ACONFIGURATION_LOCALE = 0x0004,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#TouchscreenQualifier">touchscreen</a>
	 * configuration.
	 */
	ACONFIGURATION_TOUCHSCREEN = 0x0008,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#ImeQualifier">keyboard</a>
	 * configuration.
	 */
	ACONFIGURATION_KEYBOARD = 0x0010,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#KeyboardAvailQualifier">keyboardHidden</a>
	 * configuration.
	 */
	ACONFIGURATION_KEYBOARD_HIDDEN = 0x0020,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#NavigationQualifier">navigation</a>
	 * configuration.
	 */
	ACONFIGURATION_NAVIGATION = 0x0040,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#OrientationQualifier">orientation</a>
	 * configuration.
	 */
	ACONFIGURATION_ORIENTATION = 0x0080,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#DensityQualifier">density</a>
	 * configuration.
	 */
	ACONFIGURATION_DENSITY = 0x0100,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#ScreenSizeQualifier">screen size</a>
	 * configuration.
	 */
	ACONFIGURATION_SCREEN_SIZE = 0x0200,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#VersionQualifier">platform version</a>
	 * configuration.
	 */
	ACONFIGURATION_VERSION = 0x0400,
	/**
	 * Bit mask for screen layout configuration.
	 */
	ACONFIGURATION_SCREEN_LAYOUT = 0x0800,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#UiModeQualifier">ui mode</a>
	 * configuration.
	 */
	ACONFIGURATION_UI_MODE = 0x1000,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#SmallestScreenWidthQualifier">smallest screen width</a>
	 * configuration.
	 */
	ACONFIGURATION_SMALLEST_SCREEN_SIZE = 0x2000,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#LayoutDirectionQualifier">layout direction</a>
	 * configuration.
	 */
	ACONFIGURATION_LAYOUTDIR = 0x4000,
	ACONFIGURATION_SCREEN_ROUND = 0x8000,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#WideColorGamutQualifier">wide color gamut</a>
	 * and <a href="/guide/topics/resources/providing-resources.html#HDRQualifier">HDR</a> configurations.
	 */
	ACONFIGURATION_COLOR_MODE = 0x10000,
	/**
	 * Bit mask for
	 * <a href="/guide/topics/resources/providing-resources.html#GrammaticalInflectionQualifier">grammatical gender</a>
	 * configuration.
	 */
	ACONFIGURATION_GRAMMATICAL_GENDER = 0x20000,
};

//https://developer.android.com/guide/topics/resources/providing-resources
struct ResTable_config
{
	uint32_t size;
	union {
		struct {
			uint16_t mcc;
			uint16_t mnc;
		};
		uint32_t imsi;
	};
	union {
		struct {
			char language[2];
			char country[2];
		};
		uint32_t locale;																					//4
	};
	union {
		struct {
			uint8_t orientation;			//ORIENTATION_VALUES											//128
			uint8_t touchscreen;			//TOUCHSCREEN_VALUES
			uint16_t density;																			//256
		};
		uint32_t screenType;
	};
	struct {
		union {
			struct {
				uint8_t keyboard;		//KEYBOARD_VALUES
				uint8_t navigation;		//NAVIGATION_VALUES
				uint8_t inputFlags;		//& 0x0C NAVIGATIONHIDDEN_VALUES ，& 0x03 KEYBOARDHIDDEN_VALUES
				uint8_t inputFieldPad0;
			};
			struct {
				uint32_t input : 24;
				uint32_t inputFullPad0 : 8;
			};
			struct {
				uint8_t grammaticalInflectionPad0[3];
				uint8_t grammaticalInflection;	//GRAMMATICAL_GENDER_VALUES
			};
			uint32_t inputInfo;
		};
	};
	union {
		struct {
			uint16_t screenWidth;
			uint16_t screenHeight;
		};
		uint32_t screenSize;
	};
	union {
		struct {
			uint16_t sdkVersion;					//显示成v??												//1024
			uint16_t minorVersion;
		};
		uint32_t version;
	};
	union {
		struct {
			uint8_t screenLayout;				//SCREENLAYOUT_SIZE_VALUES								//2048
			uint8_t uiMode;						//高4位UI_MODE_NIGHT_VALUES，低4位UI_MODE_TYPE_VALUES	//4096
			uint16_t smallestScreenWidthDp;		//显示成sw???dp											//8192
		};
		uint32_t screenConfig;
	};
	union {
		struct {
			uint16_t screenWidthDp;
			uint16_t screenHeightDp;
		};
		uint32_t screenSizeDp;
	};
	char localeScript[4];
	char localeVariant[8];
	union {
		struct {
			uint8_t screenLayout2;      // & 0x03 SCREENLAYOUT_ROUND_VALUES
			uint8_t colorMode;          // COLOR_WIDE_VALUES COLOR_HDR_VALUES
			uint16_t screenConfigPad2;  // Reserved padding.
		};
		uint32_t screenConfig2;
	};
	uint32_t localeScriptWasComputed;	//bool只有1字节，这个是4字节
	char localeNumberingSystem[8];
	bool operator == (const ResTable_config& _other)
	{
		if (imsi != _other.imsi)
			return false;
		if (locale != _other.locale)
			return false;
		if (screenType != _other.screenType)
			return false;
		if (inputInfo != _other.inputInfo)
			return false;
		if (screenSize != _other.screenSize)
			return false;
		if (version != _other.version)
			return false;
		if (screenConfig != _other.screenConfig)
			return false;
		if (screenSizeDp != _other.screenSizeDp)
			return false;
		if (*reinterpret_cast<const uint32_t*>(localeScript) != *reinterpret_cast<const uint32_t*>(_other.localeScript))
			return false;
		if (*reinterpret_cast<const uint32_t*>(localeVariant) != *reinterpret_cast<const uint32_t*>(_other.localeVariant))
			return false;
		if (screenConfig2 != _other.screenConfig2)
			return false;
		if (*reinterpret_cast<const uint32_t*>(localeNumberingSystem) != *reinterpret_cast<const uint32_t*>(_other.localeNumberingSystem))
			return false;
		return true;
	}
	ResTable_config()
	{
		memset(this, 0, sizeof(ResTable_config));
		size = sizeof(ResTable_config);
	}
};

struct ResTable_type
{
	ResChunk_header header;
	uint8_t id;				//<comment = "The type identifier this chunk is holding">;
	uint8_t res0;				//<comment = "Must be 0">;
	uint16_t res1;			//<comment = "Must be 0">;
	uint32_t entryCount;		//<comment = "Number of uint32_t entry indices that follow">;
	uint32_t entriesStart;		//<comment = "Offset from header where ResTable_entry data starts">;
	ResTable_config config;
};

struct ResTable_entry
{
	uint16_t size;			// <comment = "Number of bytes in this structure">;
	enum
	{
		FLAG_COMPLEX = 0x0001,
		FLAG_PUBLIC = 0x0002,
		FLAG_WEAK = 0x0004
	};
	uint16_t flags;
	ResStringPool_ref key;
};
struct Res_value
{
	uint16_t size;		//<comment = "Number of bytes in this structure">;
	uint8_t res0;			//<comment = "Always set to 0">;
	enum class _DataType : uint8_t
	{
		// The 'data' is either 0 or 1, specifying this resource is either
		// undefined or empty, respectively.
		TYPE_NULL = 0x00,
		// The 'data' holds a ResTable_ref, a reference to another resource
		// table entry.
		TYPE_REFERENCE = 0x01,
		// The 'data' holds an attribute resource identifier.
		TYPE_ATTRIBUTE = 0x02,
		// The 'data' holds an index into the containing resource table's
		// global value string pool.
		TYPE_STRING = 0x03,
		// The 'data' holds a single-precision floating point number.
		TYPE_FLOAT = 0x04,
		// The 'data' holds a complex number encoding a dimension value,
		// such as "100in".
		TYPE_DIMENSION = 0x05,
		// The 'data' holds a complex number encoding a fraction of a
		// container.
		TYPE_FRACTION = 0x06,
		// The 'data' holds a dynamic ResTable_ref, which needs to be
		// resolved before it can be used like a TYPE_REFERENCE.
		TYPE_DYNAMIC_REFERENCE = 0x07,
		// The 'data' holds an attribute resource identifier, which needs to be resolved
		// before it can be used like a TYPE_ATTRIBUTE.
		TYPE_DYNAMIC_ATTRIBUTE = 0x08,
		// Beginning of integer flavors...
		TYPE_FIRST_INT = 0x10,
		// The 'data' is a raw integer value of the form n..n.
		TYPE_INT_DEC = 0x10,
		// The 'data' is a raw integer value of the form 0xn..n.
		TYPE_INT_HEX = 0x11,
		// The 'data' is either 0 or 1, for input "false" or "true" respectively.
		TYPE_INT_BOOLEAN = 0x12,
		// Beginning of color integer flavors...
		TYPE_FIRST_COLOR_INT = 0x1c,
		// The 'data' is a raw integer value of the form #aarrggbb.
		TYPE_INT_COLOR_ARGB8 = 0x1c,
		// The 'data' is a raw integer value of the form #rrggbb.
		TYPE_INT_COLOR_RGB8 = 0x1d,
		// The 'data' is a raw integer value of the form #argb.
		TYPE_INT_COLOR_ARGB4 = 0x1e,
		// The 'data' is a raw integer value of the form #rgb.
		TYPE_INT_COLOR_RGB4 = 0x1f,
		// ...end of integer flavors.
		TYPE_LAST_COLOR_INT = 0x1f,
		// ...end of integer flavors.
		TYPE_LAST_INT = 0x1f
	};
	_DataType dataType;
	uint32_t data;
};
struct ResTable_ref
{
	uint32_t indent;
	bool operator<(const ResTable_ref& _other) const
	{
		return indent < _other.indent;
	}
};

struct TTableValueEntry : public ResTable_entry
{
	Res_value value;
};
struct ResTable_map_entry : public ResTable_entry
{
	ResTable_ref parent;
	uint32_t count;
};
struct ResTable_map
{
	ResTable_ref name;
	Res_value value;
};
#pragma pack(pop)

struct TStringPoolSpans
{
	QVector<ResStringPool_span> spans;
	bool operator == (const TStringPoolSpans& _other) const
	{
		//return spans == _other.spans;		//应该不是我的问题，是C++标准的变化，导致这个会编译错误。
		return std::equal(spans.cbegin(), spans.cend(), _other.spans.cbegin(), _other.spans.cend());
	}
};

struct TTableTypeSpecEx :public ResTable_typeSpec
{
	QVector<uint32_t> configmask;	//对应每一条具体数据，如果值是4的话，代表可以做翻译本地化	ACONFIGURATION
	TTableTypeSpecEx& operator=(const ResTable_typeSpec& _other)
	{
		*reinterpret_cast<ResTable_typeSpec*>(this) = _other;
		return *this;
	}
};
struct TTableMapEntryEx :public ResTable_map_entry
{
	QVector<ResTable_map> tablemap;
	TTableMapEntryEx& operator=(const ResTable_map_entry& _other)
	{
		*reinterpret_cast<ResTable_map_entry*>(this) = _other;
		return *this;
	}
};

struct TTableTypeEx :public ResTable_type
{
	QVector<QSharedPointer<ResTable_entry>> entryValue;
	template<typename T>
	T* createEntry(uint _idx)
	{
		QSharedPointer<ResTable_entry> t_p = QSharedPointer<ResTable_entry>(new T());
		entryValue[_idx] = t_p;
		return reinterpret_cast<T*>(t_p.data());
	}
	TTableTypeEx& operator=(const ResTable_type& _other)
	{
		*reinterpret_cast<ResTable_type*>(this) = _other;
		return *this;
	}
};

struct TRichString
{
	QString str;
	TStringPoolSpans span;
	bool operator<(const TRichString& _other) const
	{
		if (str < _other.str)
			return true;
		if (str > _other.str)
			return false;
		if (span.spans < _other.span.spans)
			return true;
		if (span.spans > _other.span.spans)
			return false;
		return false;	//相等
	}
	TRichString() {};
	TRichString(const QString& _str, const TStringPoolSpans& _span) :str(_str), span(_span) {};
};
struct TStringPool
{
	ResStringPool_header StringPoolHeader;
	QVector<QString> strings;
	QVector<TStringPoolSpans> styles;

	QMap<TRichString, uint32_t> strIndexs;
	QVector<int> referenceCount;
	void makeIndexs(void);
	void reset(void);
};

extern const char* DIMENSION_UNIT_STRS[8];
extern const char* FRACTION_UNIT_STRS[2];
extern float MANTISSA_MULT;
extern float RADIX_MULTS[4];
extern QMap<int, QString> DENSITY_DPI_VALUES;
extern const char* KEYBOARD_VALUES[4];
extern const char* KEYBOARDHIDDEN_VALUES[4];
extern const char* NAVIGATION_VALUES[5];
extern const char* NAVIGATIONHIDDEN_VALUES[10];
extern const char* ORIENTATION_VALUES[4];
extern const char* SCREENLAYOUT_SIZE_VALUES[5];
extern const char* SCREENLAYOUT_LONG_VALUES[3];
extern const char* SCREENLAYOUT_LAYOUTDIR_VALUES[3];
extern const char* SCREENLAYOUT_ROUND_VALUES[4];
extern const char* TOUCHSCREEN_VALUES[4];
extern const char* UI_MODE_NIGHT_VALUES[3];
extern const char* UI_MODE_TYPE_VALUES[16];


extern void initTableConfig(void);
extern QString tableConfig2String(const QString& _prefix, const ResTable_config& _tableConfig);
extern uint32_t getTableConfigMask(const ResTable_config& _tableConfig);
extern void packLanguageOrRegion(const char* _in, int _inlen, char* _out, int _base);

#endif // ResArscStruct_h__