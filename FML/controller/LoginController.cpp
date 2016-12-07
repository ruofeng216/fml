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
// 获取登录用户名集合 最后登录的n名用户
void CLoginController::getLoginUsers(QVector<QString> &users, int nCount)
{
	for (int i = 0, j = nCount; i < m_logins.size(), j>0; i++, j--)
	{
		users.push_back(m_logins[i]->getUname());
	}
}
// 登录验证
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
// 注册
eERR CLoginController::regLogin(const QString &uname, const QString &pswd)
{

	return e_Success;
}
// 修改密码
eERR CLoginController::modifyLogin(const QString &uname, const QString &pswd)
{
	return e_Success;
}

void CLoginController::init()
{
	// load from database

}