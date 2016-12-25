#pragma once
#include "inc\controller_interface.h"
#include <QDomElement>

class GlobalSetController : public IGlobalSetting
{

public:
	GlobalSetController();
	~GlobalSetController();

	// ��ȡ���ܸ��ڵ�˵�
	void getRootFunc(QStringList &rootVal);
	// ��ȡ�����ӽڵ�˵�
	void getChildrenFunc(const QString &funcid, QStringList &chVal);
	// ��ȡ���ܸ��ڵ�˵�
	void getParentFunc(const QString &funcid, QString &parval);
	// ��ȡ����info�˵�
	void getFuncInfo(const QString &funcid, CFuncInfo &funcinfo);


	// ��ʼ��
	void init();

private:
	// ��ʼ�����ܲ˵�
	void initFunc();
	void getFunc(const QDomElement &el);

private:
	QMap<QString, CFuncInfo> m_AllFunc;
	QMap<QString, QStringList> m_FuncLevel;
};