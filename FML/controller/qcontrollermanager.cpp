#include "qcontrollermanager.h"
#include "LoginController.h"


QControllerManager::QControllerManager(QObject *parent)
	: QObject(parent)
	, m_pLoginCtrl(NULL)
{
}

QControllerManager::~QControllerManager()
{
}

// »ñÈ¡ÊµÀý
QControllerManager *QControllerManager::instance()
{
	static QControllerManager inst;
	return &inst;
}
void QControllerManager::release()
{
	if (m_pLoginCtrl)
	{
		delete m_pLoginCtrl;
		m_pLoginCtrl = NULL;
	}
}

ILogin *QControllerManager::getLoginInst()
{
	if (m_pLoginCtrl == NULL)
	{
		m_pLoginCtrl = new CLoginController();
	}
	return m_pLoginCtrl;
}