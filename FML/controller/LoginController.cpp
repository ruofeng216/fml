#include "LoginController.h"


CLoginController::CLoginController()
	: m_pCurLogin(NULL)
{
	init();
}
CLoginController::~CLoginController()
{
	for (int i = 0; i < m_logins.size(); i++)
	{
		if (m_logins[i])
		{
			delete m_logins[i];
			m_logins[i] = NULL;
		}
	}
	m_logins.clear();
	m_pCurLogin = NULL;
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
	for (QVector<CLogin *>::const_iterator itor = m_logins.begin();
		itor != m_logins.end(); itor++)
	{
		QString u = (*itor)->getUname();
		if (u == uname)
		{
			QString p = (*itor)->getPassword();
			if (p == pswd)
			{
				m_pCurLogin = *itor;
				return e_Success;
			}
			else
				return e_ErrPswd;
		}
	}
	return e_NoUser;
}
// ע��
eERR CLoginController::regLogin(const QString &uname, const QString &pswd)
{
	eERR e = chkLogin(uname, pswd);
	if (e == e_Success)
		return e_Success;
	if (e == e_NoUser)
	{
		CLogin *pLogin = new CLogin(uname, pswd);
		m_logins.append(pLogin);
		m_pCurLogin = pLogin;
		// TODO: update db

		return e_Success;
	}
	if (e == e_ErrPswd)
		return e_Exist;
	return e_RegErr;
}
// �޸�����
eERR CLoginController::modifyLogin(const QString &uname, const QString &pswd)
{
	return e_Success;
}

void CLoginController::init()
{
	// load from database
	CLogin *pLogin= new CLogin("root", "123456");

	m_logins.append(pLogin);
	m_pCurLogin = new CLogin();
}