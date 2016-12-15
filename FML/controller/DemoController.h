#pragma once
#include "inc\controller_interface.h"
#include <QMutex>
#include "util/datatype.h"

class CDemoController :
	public IDemo
{
public:
	CDemoController();
	~CDemoController();

	bool setData(const QVariant &val);

	void getData(QVariant &val, int count = 20);


private:
	QMutex m_lock;
	QMap<QString, demoStruct> m_datas;
};

