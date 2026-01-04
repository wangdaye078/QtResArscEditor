#include "common/basicDefine.h"
#include "common/zip.h"
#include "QResArscParser.h"
#include "ResArscStruct.h"
#include <QDebug>
#include <QFile>
#include <QFileInfo>

QResArscParser::QResArscParser(QObject* parent)
	: QObject(parent)
{
	m_publicFinal = new QPublicFinal(this);
	m_stringPool = new QStringPool("GlobalStringPool", true, this);
	g_publicStrPool = m_stringPool;
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
bool QResArscParser::readFile(const QString& _path)
{
	QFileInfo t_fileInfo(_path);
	if (!t_fileInfo.exists() || !t_fileInfo.isFile())
	{
		qDebug() << "file not exists or not a file: " << _path;
		return false;
	}
	if (t_fileInfo.suffix().toLower() == "apk")
		return readApkFile(_path);
	else if (t_fileInfo.suffix().toLower() == "arsc")
		return readArscFile(_path);
	else
		qDebug() << "file type not supported: " << _path;
	return false;
}
constexpr auto ARSC_FILE_NAME = ("resources.arsc");

bool QResArscParser::readApkFile(const QString& _path)
{
	QFile t_file(_path);
	bool t_readOk = false;
	if (t_file.exists() && t_file.open(QIODevice::ReadOnly))
	{
		m_fileBuff = t_file.readAll();
		t_readOk = readApkFile(m_fileBuff);
		t_file.close();
	}
	return t_readOk;
}
bool QResArscParser::readApkFile(const QByteArray& _buff)
{
	zip_error_t t_error;
	zip_error_init(&t_error);
	zip_source_t* t_source = zip_source_buffer_create(_buff.constData(), _buff.size(), 0, &t_error);
	if (t_source == NULL)
	{
		qDebug() << "zip source buffer create failed: " << zip_error_strerror(&t_error);
		zip_error_fini(&t_error);
		return false;
	}
	zip_t* t_archive = zip_open_from_source(t_source, 0, &t_error);
	if (t_archive == NULL)
	{
		qDebug() << "can't open zip from source: " << zip_error_strerror(&t_error);
		zip_source_free(t_source);
		zip_error_fini(&t_error);
		return false;
	}
	zip_file_t* t_file = zip_fopen(t_archive, ARSC_FILE_NAME, 0);
	if (t_file == NULL)
	{
		qDebug() << "open " << ARSC_FILE_NAME << " file failed !";
		zip_close(t_archive);
		return false;
	}
	zip_stat_t t_zstat;
	zip_stat(t_archive, ARSC_FILE_NAME, 0, &t_zstat);
	QByteArray t_buff(t_zstat.size, 0);
	zip_int64_t t_size = zip_fread(t_file, t_buff.data(), t_buff.size());
	bool t_readOk = false;
	if (t_size != t_zstat.size)
		qDebug() << "read " << ARSC_FILE_NAME << " file failed !";
	else
		t_readOk = readBuff(t_buff);
	zip_fclose(t_file);
	zip_close(t_archive);
	return t_readOk;
}
bool QResArscParser::readArscFile(const QString& _path)
{
	bool t_readOk = false;
	QFile t_file(_path);
	if (t_file.exists() && t_file.open(QIODevice::ReadOnly))
	{
		m_fileBuff = t_file.readAll();
		t_readOk = readBuff(m_fileBuff);
		t_file.close();
	}
	return t_readOk;
}
bool QResArscParser::readBuff(const QByteArray& _buff)
{
	reset();
	if (_buff.size() < sizeof(ResTable_header))
	{
		qDebug() << "read buffer too small, invalid data";
		return false;
	}
	const char* t_pBuff = _buff.constData();
	ResTable_header t_tableHeader = *reinterpret_cast<const ResTable_header*>(t_pBuff);
	if (t_tableHeader.header.type != RES_TYPE::RES_TABLE_TYPE)
	{
		qDebug() << "read buffer type error, invalid data";
		return false;
	}
	if (t_tableHeader.header.headerSize != sizeof(ResTable_header))
	{
		qDebug() << "read buffer header size error, invalid data";
		return false;
	}
	if (t_tableHeader.header.size != _buff.size())
	{
		qDebug() << "read buffer size error, invalid data";
		return false;
	}
	t_pBuff += sizeof(t_tableHeader);
	bool t_readOk = true;
	while (t_pBuff < _buff.constData() + _buff.size())
	{
		if (t_pBuff + sizeof(ResChunk_header) > _buff.constData() + _buff.size())
		{
			qDebug() << "read buffer overflow, invalid data";
			t_readOk = false;
			break;
		}
		const ResChunk_header* t_pHeader = reinterpret_cast<const ResChunk_header*>(t_pBuff);
		if (t_pBuff + t_pHeader->size > _buff.constData() + _buff.size())
		{
			qDebug() << "read buffer overflow, invalid chunk size";
			t_readOk = false;
			break;
		}
		switch (t_pHeader->type)
		{
		case RES_TYPE::RES_STRING_POOL_TYPE:
			m_stringPool->readBuff(t_pBuff);
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
			t_readOk = false;
			break;
		}
		t_pBuff += t_pHeader->size;
	}
	return t_readOk;
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
bool QResArscParser::updateApkFileBuff(void)
{
	zip_source_t* t_zsmem = zip_source_buffer_create(0, 0, 0, NULL);
	zip_source_keep(t_zsmem);	//保证t_zsmem在zip_close之后都不会被销毁
	zip_source_begin_write(t_zsmem);
	zip_source_write(t_zsmem, m_fileBuff.constData(), m_fileBuff.size());
	zip_source_commit_write(t_zsmem);

	zip_t* t_archive = zip_open_from_source(t_zsmem, 0, NULL);
	Q_ASSERT(t_archive != NULL); //没道理失败，否则从最开始打开的时候就应该失败了。
	zip_int64_t t_index = zip_name_locate(t_archive, ARSC_FILE_NAME, 0);
	Q_ASSERT(t_index >= 0);
	QByteArray t_buff;
	writeBuff(t_buff);
	zip_source_t* t_source = zip_source_buffer(t_archive, t_buff.constData(), t_buff.size(), 0);
	Q_ASSERT(t_source != NULL);
	if (zip_file_replace(t_archive, t_index, t_source, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8) != 0)
	{
		qDebug() << "replace file failed: " << zip_strerror(t_archive);
		zip_source_free(t_source);
		zip_close(t_archive);
		zip_source_free(t_zsmem);
		return false;
	}
	if (zip_set_file_compression(t_archive, t_index, ZIP_CM_STORE, 1) != 0)
	{
		zip_close(t_archive);
		zip_source_free(t_zsmem);
		return false;
	}
	zip_close(t_archive);

	zip_source_open(t_zsmem);
	zip_source_seek(t_zsmem, 0, SEEK_END);
	zip_int64_t t_size = zip_source_tell(t_zsmem);
	m_fileBuff.resize(t_size);
	zip_source_seek(t_zsmem, 0, SEEK_SET);
	zip_source_read(t_zsmem, m_fileBuff.data(), t_size);
	zip_source_close(t_zsmem);

	zip_source_free(t_zsmem);
	return true;
}
bool QResArscParser::writeApkFile(const QString& _path)
{
	//前面将resources.arsc写入内存文件，后面将内存文件写入到磁盘文件中。
	bool t_updateOk = updateApkFileBuff();
	if (!t_updateOk)
		return false;

	QFile t_file(_path);
	if (!t_file.open(QIODevice::WriteOnly))
		return false;
	t_file.write(m_fileBuff);
	t_file.close();
	return true;
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

	m_stringPool->writeBuff(_buff);
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

