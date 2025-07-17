#include "common/basicDefine.h"
#include "QPublicFinal.h"
#include "QTablePackage.h"
#include "SimpleRichText.h"

QTablePackage::QTablePackage(QObject* _parent)
	: QObject(_parent), m_packageInfo(), m_typeStringPool(false), m_keyStringPool(false)
{
}

QTablePackage::~QTablePackage()
{
}

void QTablePackage::reset(void)
{
	m_typeStringPool.reset();
	m_keyStringPool.reset();
	m_tableType.clear();
}
void QTablePackage::readBuff(const char* _pBegin)
{
	m_packageInfo = *reinterpret_cast<const ResTable_package*>(_pBegin);
	Q_ASSERT(m_packageInfo.header.headerSize == sizeof(ResTable_package));
	Q_ASSERT(m_packageInfo.typeStrings == sizeof(ResTable_package));
	Q_ASSERT(m_packageInfo.typeIdOffset == 0);
	const char* t_pBuff = _pBegin + m_packageInfo.header.headerSize;
	while (t_pBuff < _pBegin + m_packageInfo.header.size)
	{
		const ResChunk_header* t_header = reinterpret_cast<const ResChunk_header*>(t_pBuff);
		switch (t_header->type)
		{
		case RES_TYPE::RES_STRING_POOL_TYPE:
			if (t_pBuff == _pBegin + m_packageInfo.keyStrings)
				m_keyStringPool.readBuff(t_pBuff);
			else if (t_pBuff == _pBegin + m_packageInfo.typeStrings)
				m_typeStringPool.readBuff(t_pBuff);
			else
				Q_ASSERT(false);
			break;
		case RES_TYPE::RES_TABLE_TYPE_SPEC_TYPE:
			{
				uint t_id = (*reinterpret_cast<const ResTable_typeSpec*>(t_pBuff)).id;//id代表什么资源，是string还array等
				if (!m_tableType.contains(t_id))
					m_tableType.insert(t_id, PTableType(new QTableType(this)));
				m_tableType[t_id]->readBuff_head(t_pBuff);
			}
			break;
		case RES_TYPE::RES_TABLE_TYPE_TYPE:
			{
				uint t_id = (*reinterpret_cast<const ResTable_type*>(t_pBuff)).id;		//id代表什么资源，是string还array等
				Q_ASSERT(m_tableType.contains(t_id));		//在有具体数据前，肯定已经有了typeSpec
				m_tableType[t_id]->readBuff_specData(t_pBuff);
			}
			break;
		default:
			Q_ASSERT(false);
			break;
		}
		t_pBuff += t_header->size;
	}
}
void QTablePackage::writeBuff(QByteArray& _buff)
{
	int t_tablePackageHeader_pos = _buff.size();
	_buff.append(reinterpret_cast<const char*>(&m_packageInfo), sizeof(m_packageInfo));
	reinterpret_cast<ResTable_package*>(_buff.data() + t_tablePackageHeader_pos)->typeStrings = _buff.size() - t_tablePackageHeader_pos;
	m_typeStringPool.writeBuff(_buff);
	reinterpret_cast<ResTable_package*>(_buff.data() + t_tablePackageHeader_pos)->keyStrings = _buff.size() - t_tablePackageHeader_pos;
	m_keyStringPool.writeBuff(_buff);
	for (QMap<uint32_t, PTableType>::const_iterator i = m_tableType.begin(); i != m_tableType.end(); ++i)
	{
		i.value()->writeBuff(this, _buff);
	}
	reinterpret_cast<ResTable_package*>(_buff.data() + t_tablePackageHeader_pos)->header.size = _buff.size() - t_tablePackageHeader_pos;
}
void QTablePackage::traversalData(TRAVERSE_PACKAGE_DATA_CALLBACK _callBack) const
{
	QString t_packageName = WCHARToQString(m_packageInfo.name);
	for (QMap<uint32_t, PTableType>::const_iterator i = m_tableType.begin(); i != m_tableType.end(); ++i)
	{
		QString t_typeName = getTypeString(i.key() - 1);
		_callBack(t_packageName, eTreeItemType_type, i.key(), t_typeName, QVariant::fromValue(i.value()));
		i.value()->traversalData(_callBack, t_packageName, i.key(), t_typeName);
	}
}
QString QTablePackage::getKeyString(const QString& _prefix, bool _addtype, uint32_t _index) const
{
	if ((_index >> 16) == 0)
		return m_keyStringPool.getGuidRef(_index)->string;
	else if ((_index >> 24) == 1 && (_index & 0xFF0000) != 0)
		return g_publicFinal->getDataName(_index)->string;
	else if ((_index >> 24) == 1 && (_index & 0xFF0000) == 0)
		return QString("0x%1").arg(_index, 8, 16, QChar('0'));
	else
	{
		uint32_t t_typeID = (_index & 0xFF0000) >> 16;
		uint32_t t_ID = _index & 0xFFFF;
		Q_ASSERT(m_tableType.contains(t_typeID));
		PTableType t_TableType = m_tableType[t_typeID];
		uint32_t t_keyIndex = t_TableType->getKeyIndex(t_ID);
		return _prefix + QString("%1%2").arg(_addtype ? (m_typeStringPool.getGuidRef(t_typeID - 1)->string + "/") : "").arg(getKeyString(QString(), false, t_keyIndex));
	}
}
QString QTablePackage::getTypeString(uint32_t _index) const
{
	return m_typeStringPool.getGuidRef(_index)->string;
}
void onTraversalSpec(TSpecificData** t_specData, const ResTable_config& _config, const QString& _packageName, ETreeItemType _type, uint32_t _typeId, const QString& _name, const QVariant& _v)
{
	PSpecificData t_pSpecificData = _v.value<PSpecificData>();
	ResTable_config t_config = t_pSpecificData->getTType().config;
	if (t_config == _config)
		*t_specData = t_pSpecificData.get();
}
QString QTablePackage::getReference(const ResTable_config& _config, qint32 _data) const
{
	if ((_data >> 24) == 1 && (_data & 0xFF0000) != 0)
		return g_publicFinal->getDataName(_data)->string;
	else if ((_data >> 24) == 1 && (_data & 0xFF0000) == 0)
		return QString("0x%1").arg(_data, 8, 16, QChar('0'));
	else
	{
		uint32_t t_typeID = (_data & 0xFF0000) >> 16;
		uint32_t t_ID = _data & 0xFFFF;

		PTableType t_pTableType = m_tableType[t_typeID];
		TSpecificData* t_specData = NULL;
		t_pTableType->traversalData(std::bind(&onTraversalSpec, &t_specData, _config, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5), "", 0, "");

		if (t_specData == NULL)
			t_specData = t_pTableType->getDefaultSpec().get();

		Q_ASSERT(t_ID < t_specData->getEntryCount());
		PResValue t_value = t_specData->getEntry(t_ID);
		if (t_value.isNull())
		{
			t_specData = t_pTableType->getDefaultSpec().get();
			t_value = t_specData->getEntry(t_ID);
		}

		//默认的那组也没有数据，这种情况是存在的，某些数据只存在特定的组，但是这种数据似乎不太会成为其他不同config组的引用
		if (t_value.isNull())
			return "not find!";
		EValueItemType t_valueItemType = t_value->getValueType();
		switch (t_valueItemType)
		{
		case eValueItemType_value:
			{
				TTableValueEntry* t_pValueEntry = reinterpret_cast<TTableValueEntry*>(t_value.get());
				if (t_pValueEntry->value.dataType == Res_value::_DataType::TYPE_DYNAMIC_REFERENCE || t_pValueEntry->value.dataType == Res_value::_DataType::TYPE_REFERENCE)
					return getReference(_config, t_pValueEntry->value.data);
				else if (t_pValueEntry->value.dataType == Res_value::_DataType::TYPE_STRING)
					return QString("@%1/%2").arg(m_typeStringPool.getGuidRef(t_typeID - 1)->string).arg(encodeRichText(t_pValueEntry->svalue.get()));
				else
					return resValue2String(t_pValueEntry->value, t_pValueEntry->svalue);
			}
			break;
		default:
			Q_ASSERT(false);
			return "error";
			break;

		}
	}
}
uint32_t QTablePackage::keyGuidToIndex(uint32_t _guid) const
{
	return m_keyStringPool.getRefIndex(m_keyStringPool.getGuidRef(_guid));
}
const ResTable_package& QTablePackage::packageInfo(void) const
{
	return m_packageInfo;
}
