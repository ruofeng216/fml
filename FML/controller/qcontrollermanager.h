#pragma once

#include <QObject>
#include "inc/controller_interface.h"
#include "democrawler.h"

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

	IDemo *getDemoInst();

	void init();

private:
	static QControllerManager* s_instance;

	ILogin *m_pLoginCtrl;

	IDemo *m_pDemo;
	DemoCrawler m_DemoCrawler;
};
