#include "common/basicDefine.h"
#include "common/zip.h"
#include "QResArscParser.h"
#include "QtGui/private/qzipreader_p.h"
#include "QtGui/private/qzipwriter_p.h"
#include "ResArscStruct.h"
#include <QDebug>
#include <QFile>
#include <QFileInfo>

QResArscParser::QResArscParser(QObject* parent)
	: QObject(parent)
{
	m_publicFinal = new QPublicFinal(this);
	m_StringPool = new QStringPool(true, this);
	g_publicStrPool = m_StringPool;
	g_publicFinal = m_publicFinal;
}

QResArscParser::~QResArscParser()
{
	g_publicFinal = NULL;
	g_publicStrPool = NULL;
}

void QResArscParser::reset(void)
{
	m_tablePackages.clear();
}
void QResArscParser::readFile(const QString& _path)
{
	QFileInfo t_fileInfo(_path);
	if (!t_fileInfo.exists() || !t_fileInfo.isFile())
	{
		qDebug() << "file not exists or not a file: " << _path;
		return;
	}
	if (t_fileInfo.suffix().toLower() == "apk")
		readApkFile(_path);
	else if (t_fileInfo.suffix().toLower() == "arsc")
		readArscFile(_path);
	else
		qDebug() << "file type not supported: " << _path;
}
constexpr auto ARSC_FILE_NAME = ("resources.arsc");

void QResArscParser::readApkFile(const QString& _path)
{
	QZipReader t_zipReader(_path);
	if (!t_zipReader.isReadable())
	{
		qDebug() << "apk file not readable: " << _path;
		return;
	}

	QByteArray t_buff = t_zipReader.fileData(QString(ARSC_FILE_NAME));
	if (!t_buff.isEmpty())
		readBuff(t_buff);
	else
		qDebug() << QString("read %1 file failed: %2").arg(QString(ARSC_FILE_NAME)).arg(_path);
	t_zipReader.close();

	/*
	QuaZip t_zipReader(_path);
	if (!t_zipReader.open(QuaZip::mdUnzip))
	{
		qDebug() << "apk file not readable: " << _path;
		return;
	}
	if (t_zipReader.setCurrentFile(ARSC_FILE_NAME, QuaZip::csInsensitive))
	{
		QuaZipFile t_archived(&t_zipReader);
		if (t_archived.open(QIODevice::ReadOnly))
		{
			QByteArray t_archivedData = t_archived.readAll();
			if (!t_archivedData.isEmpty())
			{
				readBuff(t_archivedData);
			}
			else
			{
				qDebug() << "read resources.arsc file failed: " << _path;
			}
			t_zipReader.close();
			return;
		}
	}
	qDebug() << "open resources.arsc file failed: " << _path;
	t_zipReader.close();
	return;
	*/
}
void QResArscParser::readArscFile(const QString& _path)
{
	QFile t_file(_path);
	if (t_file.exists() && t_file.open(QIODevice::ReadOnly))
	{
		QByteArray t_buff = t_file.readAll();
		readBuff(t_buff);
		t_file.close();
	}
}
void QResArscParser::readBuff(const QByteArray& _buff)
{
	reset();
	const char* t_pBuff = _buff.constData();
	ResTable_header t_tableHeader = *reinterpret_cast<const ResTable_header*>(t_pBuff);
	Q_ASSERT(t_tableHeader.header.type == RES_TYPE::RES_TABLE_TYPE);
	Q_ASSERT(t_tableHeader.header.headerSize == sizeof(ResTable_header));
	Q_ASSERT(t_tableHeader.header.size == _buff.size());
	t_pBuff += sizeof(t_tableHeader);

	while (t_pBuff < _buff.constData() + _buff.size())
	{
		if (t_pBuff + sizeof(ResChunk_header) > _buff.constData() + _buff.size())
		{
			qDebug() << "read buffer overflow, invalid data";
			break;
		}
		const ResChunk_header* t_pHeader = reinterpret_cast<const ResChunk_header*>(t_pBuff);
		if (t_pBuff + t_pHeader->size > _buff.constData() + _buff.size())
		{
			qDebug() << "read buffer overflow, invalid chunk size";
			break;
		}
		switch (t_pHeader->type)
		{
		case RES_TYPE::RES_STRING_POOL_TYPE:
			m_StringPool->readBuff(t_pBuff);
			break;
		case RES_TYPE::RES_TABLE_PACKAGE_TYPE:
			{
				QString t_packageName = WCHARToQString(reinterpret_cast<const ResTable_package*>(t_pBuff)->name);
				PTablePackage t_pTablePackage(new QTablePackage(this));
				t_pTablePackage->readBuff(t_pBuff);
				m_tablePackages[t_packageName] = t_pTablePackage;
			}
			break;
		default:
			Q_ASSERT(false);
			break;
		}
		t_pBuff += t_pHeader->size;
	}
}
bool QResArscParser::writeFile(const QString& _path)
{
	if (_path.endsWith(".apk", Qt::CaseInsensitive))
	{
		return writeApkFile(_path);
	}
	else if (_path.endsWith(".arsc", Qt::CaseInsensitive))
	{
		return writeArscFile(_path);
	}
	else
	{
		qDebug() << "file type not supported for writing: " << _path;
		return false;
	}
}
bool QResArscParser::writeApkFile(const QString& _path)
{
	//大部份库都只支持ZIP文件的添加，只有libzip支持直接修改ZIP文件中的资源。
	zip_t* t_archive = NULL;
	QByteArray t_buff;
	while (true)
	{
		t_archive = zip_open(QStringToUTF8(_path), 0, NULL);
		if (t_archive == NULL)
			break;
		zip_int64_t t_index = zip_name_locate(t_archive, ARSC_FILE_NAME, 0);
		if (t_index < 0)
			break;
		writeBuff(t_buff);
		//第四个参数如果非0，会自动托管申请的资源，直到zip_close之前自动销毁。
		//因为我们用的是QByteArray的constData()，所以不需要托管。
		zip_source_t* t_source = zip_source_buffer(t_archive, t_buff.constData(), t_buff.size(), 0);
		if (t_source == NULL)
			break;
		if (zip_file_replace(t_archive, t_index, t_source, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8) != 0)
		{
			zip_source_free(t_source);
			break;
		}
		if (zip_set_file_compression(t_archive, t_index, ZIP_CM_STORE, 1) != 0)
			break;
		zip_close(t_archive);
		//libzip的逻辑似乎是这样的：zip_source_buffer创建的source，如果调用zip_add或者zip_file_replace这种函数成功，
		//那么source会加入到zip的数据队列，在zip_close时被自动销毁，所以不需要手动调用zip_source_free。
		//如果加入失败，则需要手动调用zip_source_free。
		//而zip_source_buffer输入的BUFF，如果第四个参数不为0，则在source销毁的时候会自动销毁，否则需要手动销毁。
		//实际就是source的销毁和BUFF的销毁是两件事情，不要混淆了。
		return true;
	}
	zip_close(t_archive);
	return false;
}
bool QResArscParser::writeArscFile(const QString& _path)
{
	QFile t_file(_path);
	if (t_file.open(QIODevice::WriteOnly))
	{
		QByteArray t_buff;
		writeBuff(t_buff);
		t_file.write(t_buff);
		t_file.close();
		return true;
	}
	return false;
}
void QResArscParser::writeBuff(QByteArray& _buff)
{
	ResTable_header t_tableHeader;
	t_tableHeader.header.type = RES_TYPE::RES_TABLE_TYPE;
	t_tableHeader.header.headerSize = sizeof(ResTable_header);
	t_tableHeader.packageCount = m_tablePackages.size();
	_buff.append(reinterpret_cast<const char*>(&t_tableHeader), sizeof(t_tableHeader));

	m_StringPool->writeBuff(_buff);
	for (QMap<QString, PTablePackage>::iterator i = m_tablePackages.begin(); i != m_tablePackages.end(); ++i)
	{
		i.value()->writeBuff(_buff);
	}
	ResChunk_header* t_head = reinterpret_cast<ResChunk_header*>(_buff.data());
	t_head->size = static_cast<uint32_t>(_buff.size());
}
void QResArscParser::traversalAllTablePackage(TTRAVERSAL_ALL_TABLEPACKAGE _callBack)
{
	for (QMap<QString, PTablePackage>::iterator i = m_tablePackages.begin(); i != m_tablePackages.end(); ++i)
		_callBack(i.key(), i.value());
}

