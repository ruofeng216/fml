#pragma once

#include <QObject>
#include "inc/controller_interface.h"

class QControllerManager : public QObject
{
	Q_OBJECT

public:
	QControllerManager(QObject *parent = NULL);
	~QControllerManager();

	// »ñÈ¡ÊµÀý
	static QControllerManager *instance();
	// release
	void release();

	ILogin *getLoginInst();

private:
	static QControllerManager* s_instance;

	ILogin *m_pLoginCtrl;
};
