#include "qdbmgr.h"

QDBMgr::QDBMgr(const QString &dbname, QObject *parent)
	: QObject(parent)
	, m_db(dbname)
	, m_sqlQuery(NULL)
{
}

QDBMgr::~QDBMgr()
{
}

// tables data init.
void QDBMgr::initTables()
{

}

// �����ݿ�
bool QDBMgr::openDB()
{

}
// �������ݿ�
bool QDBMgr::updateDB()
{

}