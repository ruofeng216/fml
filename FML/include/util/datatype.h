#include <QString>
#include <QVariantMap>


// login
class CLogin 
{
public:
	CLogin();
	CLogin(const QVariantMap &val);
	~CLogin();

	void setUname(const QString &name);
	const QString &getUname() const;

	void setPassword(const QString &pswd);
	const QString &getPassword() const;

	const QVariantMap &toJson();

private:
	QString m_uname;
	QString m_password;
};

