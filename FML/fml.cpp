#include "fml.h"





FML::FML(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	connect(VIEWSIGNAL, &ViewSignalManager::login, this, &FML::login);
	qDebug() << "main thred" << QThread::currentThreadId();
	QTimer *t = new QTimer(this);
	connect(t, &QTimer::timeout, [this]{
		Worker *worker = new Worker(this);
		worker->start();
	});
	t->start(5000);
}
void FML::login(const CMyBasePtr val)
{
	if (val->getClassName() == "CLogin")
	{
		const CLogin *cl = static_cast<const CLogin*>(val.constData());
		qDebug() << QString(cl->getUname()) << "/" << QString(cl->getPassword());
	}
}