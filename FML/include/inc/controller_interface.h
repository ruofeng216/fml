#include "..\util\datatype.h"
#include <QVector>

class ILogin
{
public:
	enum ETYPE {
		e_Success = 0, // �ɹ�
		e_NoUser,      // �û�������
		e_ErrPswd,     // �������
		e_Exist,       // �û�����
		e_RegErr,      // ע��ʧ��
		e_ModifyErr    // �޸�ʧ��
	};
	virtual ~ILogin() = 0;
	// ��ȡ��¼�û������� ����¼��n���û�
	virtual void getLoginUsers(QVector<QString> &users, int nCount = 5) = 0;
	// ��¼��֤
	virtual ETYPE chkLogin(const QString &uname, const QString &pswd) = 0;
	// ע��
	virtual ETYPE regLogin(const QString &uname, const QString &pswd) = 0;
	// �޸�����
	virtual ETYPE modifyLogin(const QString &uname, const QString &pswd) = 0;
};
