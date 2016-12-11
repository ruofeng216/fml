#pragma once
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QSharedDataPointer>
#include "pro.h"

class CMyBase : public QSharedData
{
public:
	CMyBase(const QString& name);
	CMyBase(const CMyBase &other);
	virtual ~CMyBase();
	const QString &getClassName() const;

	virtual void setErrorCode(eERR errorCode);
	virtual eERR getErrorCode() const;

protected:
	QString m_className;               // 派生类的名称
	eERR m_errorCode;               // 错误码，0表示成功
};
Q_DECLARE_TYPEINFO(CMyBase, Q_MOVABLE_TYPE);

typedef QSharedDataPointer<CMyBase> CMyBasePtr;


class CMyField
{
public:
	CMyField();
	~CMyField();
	void setKey(const QString &key);
	const QString &getKey() const;

	void setVal(const QVariant &val);
	const QVariant &getVal() const;

private:
	QString m_key;
	QVariant m_val;
};


// login
#define CLASSNAME_CLOGIN "CLogin"
class CLogin : public CMyBase
{
public:
	CLogin(const QString &name = "", const QString &pswd = "");
	~CLogin();
	void setUname(const QString &name);
	const QString &getUname() const;
	const QString &getUnameKey() const;

	void setPassword(const QString &pswd);
	const QString &getPassword() const;
	const QString &getPasswordKey() const;

	const QVariantMap &toJson();
	void fromJson(const QVariantMap &val);

private:
	CMyField m_uname;
	CMyField m_password;
};

