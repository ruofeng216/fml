#include <QtWidgets/QApplication>
#include "qmpreload.h"
#include <QCoreApplication>
#include <QDebug>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

	// If this property is true, the applications quits when the last visible primary window (i.e. window with no parent) is closed.
	a.setQuitOnLastWindowClosed(false);

	CrashHook crashHook(new QmCrashEvents());
	// ≥Ã–Ú≥ı ºªØ
	QmPreload qmInit;
	if (!qmInit.init(argc, argv)) {
		return false;
	}
	qDebug() << QCoreApplication::applicationFilePath();
    return a.exec();
}
