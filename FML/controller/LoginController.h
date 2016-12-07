#pragma once
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
	eERR chkLogin(const QString &uname, const QString &pswd);
	// ע��
	eERR regLogin(const QString &uname, const QString &pswd);
	// �޸�����
	eERR modifyLogin(const QString &uname, const QString &pswd);

protected:
	void init();

private:
	QVector<CLogin*> m_logins;
	CLogin* m_pCurLogin;
};

