#include "DemoController.h"
#include <QMutexLocker>
#include "viewsignalmanager.h"

CDemoController::CDemoController()
{
}


CDemoController::~CDemoController()
{
}

bool CDemoController::setData(const QVariant &val)
{
	QMutexLocker lock(&m_lock);
	demoStruct *t = new demoStruct;
	t->fromJson(val.toMap());
	m_datas[t->getSN().getVal().toString()] = *t;
	emit VIEWSIGNAL->callBackUI(CMyBasePtr(t));
	return true;
}

void CDemoController::getData(QVariant &val, int count)
{
	QMutexLocker lock(&m_lock);
	QVariantList rt;
	QList<QString> &lst = m_datas.keys();
	for (int i = lst.size(), j = 0; i > 0 , j < count; i--,j++)
	{
		rt.append(m_datas[lst[i]].toJson());
	}
	val = QVariant(rt);
}