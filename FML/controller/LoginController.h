#pragma once
#include "inc\controller_interface.h"

class CLoginController :
	public ILogin
{
public:
	CLoginController();
	~CLoginController();

	// 获取登录用户名集合 最后登录的n名用户
	void getLoginUsers(QVector<QString> &users, int nCount = 5);
	// 登录验证
	eERR chkLogin(const QString &uname, const QString &pswd);
	// 注册
	eERR regLogin(const QString &uname, const QString &pswd);
	// 修改密码
	eERR modifyLogin(const QString &uname, const QString &pswd);

protected:
	void init();

private:
	QVector<CLogin*> m_logins;
	CLogin* m_pCurLogin;
};

