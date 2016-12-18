#include "democrawler.h"
#include "util/datatype.h"
#include <QUuid>
#include <QDateTime>
#include "viewsignalmanager.h"


DemoCrawler::DemoCrawler(QObject *parent)
	: QThread(parent)
{
}

DemoCrawler::~DemoCrawler()
{
}

void DemoCrawler::run()
{
	QStringList stt;
	stt << "000001.HK" << "602611.SH" << "021254.SZ";
	while (true)
	{
		CMyBasePtr s(new demoStruct());
		demoStruct *st = static_cast<demoStruct*>(s.data());
		st->setBondid(CMyField(stt[QDateTime::currentMSecsSinceEpoch() % 3]));
		st->setBid(CMyField(float(QDateTime::currentMSecsSinceEpoch()%100)/100.0 + 2.0));
		st->setVolBid(CMyField(QDateTime::currentMSecsSinceEpoch() / 1000 * 1000 % 10000));
		st->setOfr(CMyField(float(QDateTime::currentMSecsSinceEpoch() % 100) / 100.0 + 2.0));
		st->setVolOfr(CMyField(QDateTime::currentMSecsSinceEpoch() / 1000 * 1000 % 10000));
		static int a = 1;
		st->setSN(CMyField(QString::number(a)));
		a++;
		emit VIEWSIGNAL->callBackUI(s);
		msleep(50);
	}
}