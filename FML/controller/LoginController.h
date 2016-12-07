#pragma once
#include <QMap>
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
	ETYPE chkLogin(const QString &uname, const QString &pswd);
	// 注册
	ETYPE regLogin(const QString &uname, const QString &pswd);
	// 修改密码
	ETYPE modifyLogin(const QString &uname, const QString &pswd);

protected:
	void initLoginUsers();

private:
	QMap<QString, CLogin> m_logins;

};

