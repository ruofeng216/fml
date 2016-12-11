#include "datatype.h"

////////////////////////////////////////////////////////////////////
//CMyBase
CMyBase::CMyBase(const QString& name)
{
	m_className = name;
	m_errorCode = e_Success;
}
CMyBase::CMyBase(const CMyBase &other)
	: QSharedData(other)
{
	m_className = other.getClassName();
	m_errorCode = other.getErrorCode();
}
CMyBase::~CMyBase()
{
}
const QString& CMyBase::getClassName() const
{
	return m_className;
}
void CMyBase::setErrorCode(eERR errorCode)
{
	m_errorCode = errorCode;
}
eERR CMyBase::getErrorCode() const
{
	return m_errorCode;
}
//////////////////////////////////////////////////////////////////////
//CMyField
CMyField::CMyField()
{
}

CMyField::~CMyField()
{

}
void CMyField::setKey(const QString &key)
{
	m_key = key;
}
const QString &CMyField::getKey() const
{
	return m_key;
}
void CMyField::setVal(const QVariant &val)
{
	m_val = val;
}
const QVariant &CMyField::getVal() const
{
	return m_val;
}
//////////////////////////////////////////////////////////////////////
// login
CLogin::CLogin(const QString &name, const QString &pswd)
	: CMyBase(CLASSNAME_CLOGIN)
{
	m_uname.setKey("uname");
	m_uname.setVal(name);
	m_password.setKey("password");
	m_password.setVal(pswd);
}
CLogin::~CLogin()
{
}

void CLogin::setUname(const QString &name)
{
	m_uname.setVal(name);
}
const QString &CLogin::getUname() const
{
	return m_uname.getVal().toString();
}
const QString &CLogin::getUnameKey() const
{
	return m_uname.getKey();
}

void CLogin::setPassword(const QString &pswd)
{
	m_password.setVal(pswd);
}
const QString &CLogin::getPassword() const
{
	return m_password.getVal().toString();
}
const QString &CLogin::getPasswordKey() const
{
	return m_password.getKey();
}

const QVariantMap &CLogin::toJson()
{
	QVariantMap mJson;
	mJson[getUnameKey()] = getUname();
	mJson[getPasswordKey()] = getPassword();
	return mJson;
}
void CLogin::fromJson(const QVariantMap &val)
{
	if (val.contains(getUnameKey()))
	{
		setUname(val[getUnameKey()].toString());
		setPassword(val[getPasswordKey()].toString());
	}
}
////////////////////////////////////////////////////////////////
