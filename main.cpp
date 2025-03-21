#include <QtWidgets/QApplication>
#include "src/QResArscEditor.h"
#include "src/ResArscStruct.h"
#include "src/SimpleRichText.h"

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	initTableConfig();
	QResArscEditor w;
	w.show();
	return a.exec();
}
