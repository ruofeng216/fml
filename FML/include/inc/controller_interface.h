#include "util\datatype.h"
#include <QVector>

class ILogin
{
public:
	virtual ~ILogin() = 0;
	// 获取登录用户名集合 最后登录的n名用户
	virtual void getLoginUsers(QVector<QString> &users, int nCount = 5) = 0;
	// 登录验证
	virtual eERR chkLogin(const QString &uname, const QString &pswd) = 0;
	// 注册
	virtual eERR regLogin(const QString &uname, const QString &pswd) = 0;
	// 修改密码
	virtual eERR modifyLogin(const QString &uname, const QString &pswd) = 0;
};
