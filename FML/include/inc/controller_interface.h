#pragma once
#include "util\datatype.h"
#include <QVector>

class ILogin
{
public:
	virtual ~ILogin() = 0;
	// 登录验证
	virtual eERR chkLogin(const QString &uname, const QString &pswd) = 0;
	// 注册
	virtual eERR regLogin(const QString &uname, const QString &pswd) = 0;
	// 修改密码
	virtual eERR modifyLogin(const QString &uname, const QString &pswd) = 0;
};

class IGlobalSetting
{
public:
	virtual ~IGlobalSetting() = 0;
	// 获取功能根节点菜单
	virtual void getRootFunc(QStringList &rootVal) = 0;
	// 获取功能子节点菜单
	virtual void getChildrenFunc(const QString &funcid, QStringList &chVal) = 0;
	// 获取功能父节点菜单
	virtual void getParentFunc(const QString &funcid, QString &parval) = 0;
	// 获取功能info
	virtual void getFuncInfo(const QString &funcid, CFuncInfo &funcinfo) = 0;
	
};




class IDemo
{
public:
	virtual ~IDemo() = 0;

	virtual bool setData(const QVariant &val) = 0;

	virtual void getData(QVariant &val, int count = 20) = 0;
	virtual int getSN() = 0;
};