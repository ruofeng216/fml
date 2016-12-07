#pragma once
#include <QMap>
#include "inc\controller_interface.h"

class CLoginController :
	public ILogin
{
public:
	CLoginController();
	~CLoginController();

	// ��ȡ��¼�û������� ����¼��n���û�
	void getLoginUsers(QVector<QString> &users, int nCount = 5);
	// ��¼��֤
	ETYPE chkLogin(const QString &uname, const QString &pswd);
	// ע��
	ETYPE regLogin(const QString &uname, const QString &pswd);
	// �޸�����
	ETYPE modifyLogin(const QString &uname, const QString &pswd);

protected:
	void initLoginUsers();

private:
	QMap<QString, CLogin> m_logins;

};

