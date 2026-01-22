#include "src/QResArscEditor.h"
#include <map>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QTranslator>
#include <QtWidgets/QApplication>
#include <set>

void myMessageHandler(QtMsgType _type, const QMessageLogContext& _context, const QString& _msg)
{
	static QFile t_file;
	if (!t_file.isOpen())
	{
		QString t_filePath = QCoreApplication::applicationDirPath() + "/log.txt";
		t_file.setFileName(t_filePath);
		if (!t_file.open(QIODevice::Append | QIODevice::Text))
		{
			qInstallMessageHandler(NULL);
			qWarning("Cannot open log file for writing.");
			return;
		}
	}

	QTextStream t_out(&t_file);
	t_out.setCodec("UTF-8");
	t_out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ");
	switch (_type) {
	case QtDebugMsg:
		t_out << "Debug: ";
		break;
	case QtInfoMsg:
		t_out << "Info: ";
		break;
	case QtWarningMsg:
		t_out << "Warning: ";
		break;
	case QtCriticalMsg:
		t_out << "Critical: ";
		break;
	case QtFatalMsg:
		t_out << "Fatal: ";
		break;
	}

	t_out << _msg << "\n";
}
int main(int argc, char* argv[])
{
	QApplication app(argc, argv);
#ifndef _DEBUG
	qInstallMessageHandler(myMessageHandler);
#endif // !_DEBUG
	QTranslator translator;
	if (translator.load(QLocale(), QFileInfo(QCoreApplication::applicationFilePath()).completeBaseName(),
		"_", ":/translation/tr"))
		QCoreApplication::installTranslator(&translator);

	QResArscEditor window;
	window.show();
	return app.exec();
}
