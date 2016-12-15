#pragma once
#include "util\datatype.h"
#include <QVector>

class ILogin
{
public:
	virtual ~ILogin() = 0;
	// ��¼��֤
	virtual eERR chkLogin(const QString &uname, const QString &pswd) = 0;
	// ע��
	virtual eERR regLogin(const QString &uname, const QString &pswd) = 0;
	// �޸�����
	virtual eERR modifyLogin(const QString &uname, const QString &pswd) = 0;
};

class IDemo
{
public:
	virtual ~IDemo() = 0;

	virtual bool setData(const QVariant &val) = 0;

	virtual void getData(QVariant &val, int count = 20) = 0;
};