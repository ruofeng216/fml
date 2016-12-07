#include "util\datatype.h"
#include <QVector>

class ILogin
{
public:
	virtual ~ILogin() = 0;
	// ��ȡ��¼�û������� ����¼��n���û�
	virtual void getLoginUsers(QVector<QString> &users, int nCount = 5) = 0;
	// ��¼��֤
	virtual eERR chkLogin(const QString &uname, const QString &pswd) = 0;
	// ע��
	virtual eERR regLogin(const QString &uname, const QString &pswd) = 0;
	// �޸�����
	virtual eERR modifyLogin(const QString &uname, const QString &pswd) = 0;
};
