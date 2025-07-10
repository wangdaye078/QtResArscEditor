#include "src/QResArscEditor.h"
#include <QtWidgets/QApplication>
#include <map>
#include <QDebug>
#include <set>

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	QResArscEditor window;
	window.show();
	return app.exec();
}
