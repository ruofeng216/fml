#pragma once

typedef enum e_ErrType
{
	e_Success = 0, // �ɹ�
	e_NoUser = 1,      // �û�������
	e_ErrPswd = 2,     // �������
	e_Exist = 3,       // �û�����
	e_RegErr = 4,      // ע��ʧ��
	e_ModifyErr = 5,    // �޸�ʧ��

} eERR;