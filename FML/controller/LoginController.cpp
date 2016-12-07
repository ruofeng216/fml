#include "LoginController.h"


CLoginController::CLoginController()
	: m_pCurLogin(NULL)
{
	init();
}
CLoginController::~CLoginController()
{
	m_logins.clear();
}
// ��ȡ��¼�û������� ����¼��n���û�
void CLoginController::getLoginUsers(QVector<QString> &users, int nCount)
{
	for (int i = 0, j = nCount; i < m_logins.size(), j>0; i++, j--)
	{
		users.push_back(m_logins[i]->getUname());
	}
}
// ��¼��֤
eERR CLoginController::chkLogin(const QString &uname, const QString &pswd)
{
	for (QVector<CLogin *>::iterator itor = m_logins.begin();
		itor != m_logins.end(); itor++)
	{
		if ((*itor)->getUname() == uname)
		{
			if ((*itor)->getPassword() == pswd)
				return e_Success;
			else
				return e_ErrPswd;
		}
	}
	return e_NoUser;
}
// ע��
eERR CLoginController::regLogin(const QString &uname, const QString &pswd)
{

	return e_Success;
}
// �޸�����
eERR CLoginController::modifyLogin(const QString &uname, const QString &pswd)
{
	return e_Success;
}

void CLoginController::init()
{
	// load from database

}