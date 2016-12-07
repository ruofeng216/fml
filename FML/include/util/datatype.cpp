#include "datatype.h"

// login
CLogin::CLogin()
{
}
CLogin::CLogin(const QVariantMap &val)
{
	if (val.contains(""))
}
CLogin::~CLogin()
{
}

void CLogin::setUname(const QString &name)
{
	m_uname = name;
}
const QString &CLogin::getUname() const
{
	return m_uname;
}

void CLogin::setPassword(const QString &pswd)
{
	m_password = pswd;
}
const QString &CLogin::getPassword() const
{
	return m_password;
}

////////////////////////////////////////////////////////////////
