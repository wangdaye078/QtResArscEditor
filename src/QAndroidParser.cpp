#include "common/basicDefine.h"
#include "common/zip.h"
#include "QAndroidParser.h"
#include <QDebug>
#include <QFile>
#include <QFileInfo>

QAndroidParser::QAndroidParser(QObject* _parent) :
	QObject(_parent)
{
	m_stringPool = new QStringPool("GlobalStringPool", true, true, this);
	g_publicStrPool = m_stringPool;
};
QAndroidParser::~QAndroidParser()
{
	delete m_stringPool;
	g_publicStrPool = NULL;
};
QStringPool* QAndroidParser::getStringPool(void) const
{
	return m_stringPool;
}
bool QAndroidParser::readFile(const QString& _path)
{
	QFileInfo t_fileInfo(_path);
	if (!t_fileInfo.exists() || !t_fileInfo.isFile())
	{
		qDebug() << "file not exists or not a file: " << _path;
		return false;
	}
	if (t_fileInfo.suffix().toLower() == "apk")
		return readApkFile(_path, getBinFileName());
	else if (t_fileInfo.suffix().toLower() == getBinFileSuffix())
		return readBinFile(_path);
	else
		qDebug() << "file type not supported: " << _path;
	return false;
}
bool QAndroidParser::readApkFile(const QString& _path, const char* _fileName)
{
	QFile t_file(_path);
	bool t_readOk = false;
	if (t_file.exists() && t_file.open(QIODevice::ReadOnly))
	{
		m_fileBuff = t_file.readAll();
		t_readOk = readApkFile(m_fileBuff, _fileName);
		t_file.close();
	}
	return t_readOk;
}
bool QAndroidParser::readApkFile(const QByteArray& _buff, const char* _fileName)
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
	zip_file_t* t_file = zip_fopen(t_archive, _fileName, 0);
	if (t_file == NULL)
	{
		qDebug() << "open " << _fileName << " file failed !";
		zip_close(t_archive);
		return false;
	}
	zip_stat_t t_zstat;
	zip_stat(t_archive, _fileName, 0, &t_zstat);
	QByteArray t_buff(t_zstat.size, 0);
	zip_int64_t t_size = zip_fread(t_file, t_buff.data(), t_buff.size());
	bool t_readOk = false;
	if (t_size != t_zstat.size)
		qDebug() << "read " << _fileName << " file failed !";
	else
		t_readOk = readBuff(t_buff);
	zip_fclose(t_file);
	zip_close(t_archive);
	return t_readOk;
}
bool QAndroidParser::readBinFile(const QString& _path)
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
bool QAndroidParser::writeFile(const QString& _path)
{
	QFileInfo t_fileInfo(_path);
	if (t_fileInfo.suffix().toLower() == "apk")
	{
		return writeApkFile(_path, getBinFileName());
	}
	else if (t_fileInfo.suffix().toLower() == getBinFileSuffix())
	{
		return writeBinFile(_path);
	}
	else
	{
		qDebug() << "file type not supported for writing: " << _path;
		return false;
	}
}
bool QAndroidParser::updateApkFileBuff(const QByteArray& _buff, const char* _fileName)
{
	zip_source_t* t_zsmem = zip_source_buffer_create(0, 0, 0, NULL);
	zip_source_keep(t_zsmem);	//保证t_zsmem在zip_close之后都不会被销毁
	zip_source_begin_write(t_zsmem);
	zip_source_write(t_zsmem, m_fileBuff.constData(), m_fileBuff.size());
	zip_source_commit_write(t_zsmem);

	zip_t* t_archive = zip_open_from_source(t_zsmem, 0, NULL);
	Q_ASSERT(t_archive != NULL); //没道理失败，否则从最开始打开的时候就应该失败了。
	zip_int64_t t_index = zip_name_locate(t_archive, _fileName, 0);
	Q_ASSERT(t_index >= 0);

	zip_source_t* t_source = zip_source_buffer(t_archive, _buff.constData(), _buff.size(), 0);
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
bool QAndroidParser::writeApkFile(const QString& _path, const char* _fileName)
{
	QByteArray t_buff;
	writeBuff(t_buff);
	//前面将resources.arsc写入内存文件，后面将内存文件写入到磁盘文件中。
	bool t_updateOk = updateApkFileBuff(t_buff, _fileName);
	if (!t_updateOk)
		return false;

	QFile t_file(_path);
	if (!t_file.open(QIODevice::WriteOnly))
		return false;
	t_file.write(m_fileBuff);
	t_file.close();
	return true;
}
bool QAndroidParser::writeBinFile(const QString& _path)
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