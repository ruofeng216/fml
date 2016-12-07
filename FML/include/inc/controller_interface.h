#include "..\util\datatype.h"
#include <QVector>

class ILogin
{
public:
	enum ETYPE {
		e_Success = 0, // 成功
		e_NoUser,      // 用户不存在
		e_ErrPswd,     // 密码错误
		e_Exist,       // 用户存在
		e_RegErr,      // 注册失败
		e_ModifyErr    // 修改失败
	};
	virtual ~ILogin() = 0;
	// 获取登录用户名集合 最后登录的n名用户
	virtual void getLoginUsers(QVector<QString> &users, int nCount = 5) = 0;
	// 登录验证
	virtual ETYPE chkLogin(const QString &uname, const QString &pswd) = 0;
	// 注册
	virtual ETYPE regLogin(const QString &uname, const QString &pswd) = 0;
	// 修改密码
	virtual ETYPE modifyLogin(const QString &uname, const QString &pswd) = 0;
};
