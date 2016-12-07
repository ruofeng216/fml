#pragma once

#include <QObject>

class ILogin;
class QControllerManager : public QObject
{
	Q_OBJECT

public:
	QControllerManager(QObject *parent = NULL);
	~QControllerManager();

	// ��ȡʵ��
	static QControllerManager *instance();
	// release
	void release();

	ILogin *getLoginInst();

private:
	ILogin *m_pLoginCtrl;
};
